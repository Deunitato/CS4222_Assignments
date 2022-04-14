#include "contiki.h"
#include "dev/leds.h"
#include <stdio.h>
#include "core/net/rime/rime.h"
#include "dev/serial-line.h"
#include "dev/uart1.h"
#include "node-id.h"
#include "defs_and_types.h"
#include "net/netstack.h"
#include "random.h"
#ifdef TMOTE_SKY
#include "powertrace.h"
#endif
/*---------------------------------------------------------------------------*/
//#define WAKE_TIME RTIMER_SECOND/20    // 10 HZ, 0.1s
/*---------------------------------------------------------------------------*/
//#define SLEEP_CYCLE 4         	      // 0 for never sleep
//#define SLEEP_SLOT RTIMER_SECOND/5   // sleep slot should not be too large to prevent overflow
/*---------------------------------------------------------------------------*/
// duty cycle = WAKE_TIME / (WAKE_TIME + SLEEP_SLOT * SLEEP_CYCLE)
/*---------------------------------------------------------------------------*/
#define TIME_SLOT RTIMER_SECOND/16
#define N_SIZE 10

#define THRESHOLD -65

static int row;
static int col;
static int currRow = 0;
static int currCol = 0;
/*---------------------------------------------------------------------------*/
typedef struct {
  unsigned long node_id;
  unsigned long timestamp;
  unsigned int leaveCounter;
} node;
// nodes store
static node nodes[10];
/*---------------------------------------------------------------------------*/
// sender timer
static struct rtimer rt;
static struct pt pt;
/*---------------------------------------------------------------------------*/
static data_packet_struct received_packet;
static data_packet_struct data_packet;
unsigned long curr_timestamp;
/*---------------------------------------------------------------------------*/
PROCESS(cc2650_nbr_discovery_process, "cc2650 neighbour discovery process");
AUTOSTART_PROCESSES(&cc2650_nbr_discovery_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  leds_on(LEDS_GREEN);
  memcpy(&received_packet, packetbuf_dataptr(), sizeof(data_packet_struct));
  int rssi = (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI);
  int i;
  if (rssi < THRESHOLD) {
    // outside 3m
    for (i = 0; i < 10; i++) {
      if (nodes[i].node_id == received_packet.src_id) {
        nodes[i].leaveCounter += 1; 
        if (nodes[i].leaveCounter >= 3) {
          printf("%8lu LEAVE %lu\n",
            curr_timestamp / CLOCK_SECOND,
            received_packet.src_id);
          nodes[i].node_id = 0;
          nodes[i].timestamp = 0;
          nodes[i].leaveCounter = 0;
        }
        break;
      }
    }
  } else {
    // within 3m
    bool hasFound = false;
    for (i = 0; i < 10; i++) {
      // if found in array, reset counter and refresh timestamp
      if (nodes[i].node_id == received_packet.src_id) {
        nodes[i].timestamp = curr_timestamp / CLOCK_SECOND;
        nodes[i].leaveCounter = 0;
        hasFound = true;
        break;
      }
    }
    if (!hasFound) {
      for (i = 0; i < 10; i++) {
        // if found empty slot in array, add
        if (nodes[i].node_id == 0) {
          printf("%8lu DETECT %lu\n",
            curr_timestamp / CLOCK_SECOND,
            received_packet.src_id);
          nodes[i].node_id = received_packet.src_id;
          nodes[i].timestamp = curr_timestamp / CLOCK_SECOND;
          nodes[i].leaveCounter = 0;
          break;
        }
      }
    }
  }
  printf("Received packet from node %lu with sequence number %lu and timestamp %3lu.%03lu RSSI: %d\n", 
    received_packet.src_id, 
    received_packet.seq, 
    received_packet.timestamp / CLOCK_SECOND, 
    ((received_packet.timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND,
    rssi);
  
  leds_off(LEDS_GREEN);
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
char sender_scheduler(struct rtimer *t, void *ptr) {
  static uint16_t i = 0;
  PT_BEGIN(&pt);

  curr_timestamp = clock_time(); 
  printf("Start clock %lu ticks, timestamp %3lu.%03lu\n", 
    curr_timestamp, 
    curr_timestamp / CLOCK_SECOND, 
    ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);

  while(1) {
    // check for expired detected nodes
    for (i = 0; i < 10; i++) {
      if (nodes[i].node_id != 0) {
        long old_time = nodes[i].timestamp;
        long new_time = curr_timestamp / CLOCK_SECOND;
        if (new_time - old_time >= 30) {
          printf("%8lu LEAVE %lu\n",
            new_time,
            nodes[i].node_id);
          nodes[i].node_id = 0;
          nodes[i].timestamp = 0;
          nodes[i].leaveCounter = 0;
        }
      }
    }
    if (currRow == row || currCol == col) {
      // radio on    
      NETSTACK_RADIO.on();
      leds_off(LEDS_BLUE);
      leds_on(LEDS_RED);  
      for(i = 0; i < NUM_SEND; i++) {
        data_packet.seq++;
        curr_timestamp = clock_time();
        data_packet.timestamp = curr_timestamp;
        /*
        printf("Send seq# %lu  @ %8lu ticks   %3lu.%03lu currRow: %d currCol: %d\n", 
          data_packet.seq, 
          curr_timestamp, 
          curr_timestamp / CLOCK_SECOND, 
          ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND,
          currRow,
          currCol);
        */
        packetbuf_copyfrom(&data_packet, (int)sizeof(data_packet_struct));
        broadcast_send(&broadcast);

        if(i != (NUM_SEND - 1)) {
          rtimer_set(t, RTIMER_TIME(t) + TIME_SLOT, 1, (rtimer_callback_t)sender_scheduler, ptr);
          PT_YIELD(&pt);
        }
      }  
    }
    else {
      leds_off(LEDS_RED);
      leds_on(LEDS_BLUE);
      // radio off
      NETSTACK_RADIO.off();
      rtimer_set(t, RTIMER_TIME(t) + TIME_SLOT, 1, (rtimer_callback_t)sender_scheduler, ptr);
      PT_YIELD(&pt);
    }
    if (currCol == N_SIZE - 1 && currRow == N_SIZE - 1) {
      currCol = 0;
      currRow = 0;
    } else if (currCol == N_SIZE - 1) {
      currCol = 0;
      currRow += 1;
    } else {
      currCol += 1;
    }
  }
  
  PT_END(&pt);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc2650_nbr_discovery_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  random_init(54222);

  int i;
  for (i = 0; i < 10; i++) {
    nodes[i].node_id = 0;
    nodes[i].timestamp = 0;
    nodes[i].leaveCounter = 0;
  }

  row = random_rand() % N_SIZE;
  col = random_rand() % N_SIZE;

  #ifdef TMOTE_SKY
  powertrace_start(CLOCK_SECOND * 5);
  #endif

  broadcast_open(&broadcast, 129, &broadcast_call);

  // for serial port
  #if !WITH_UIP && !WITH_UIP6
  uart1_set_input(serial_line_input_byte);
  serial_line_init();
  #endif

  printf("CC2650 neighbour discovery\n");
  printf("Node %d will be sending packet of size %d Bytes\n", 
    node_id, (int)sizeof(data_packet_struct));
  printf("N_SIZE: %d row: %d col: %d\n", N_SIZE, row, col);
  // radio off
  NETSTACK_RADIO.off();

  // initialize data packet
  data_packet.src_id = node_id;
  data_packet.seq = 0;

  // Start sender in one millisecond.
  rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
