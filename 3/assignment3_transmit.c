/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "board-peripherals.h"
#include "sys/etimer.h"
#include <stdio.h>
#include "net/rime/rime.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/

static struct etimer timer_etimer;
static char message[50] = {48};
static void send(char message[], int size);
static void messageInc();

PROCESS(transmit_process, "unicasting...");
AUTOSTART_PROCESSES(&transmit_process);

static const struct unicast_callbacks unicast_callbacks = {};
static struct unicast_conn uc;

static void send(char message[], int size) {
	linkaddr_t addr;
	printf("%s\n",message);
   packetbuf_copyfrom(message, strlen(message));

   // COMPUTE THE ADDRESS OF THE RECEIVER FROM ITS NODE ID, FOR EXAMPLE NODEID 0xBA04 MAPS TO 0xBA AND 0x04 RESPECTIVELY
   // In decimal, if node ID is 47620, this maps to 186 (higher byte) AND 4 (lower byte)
   //22528 => (5800) 0: 88, 1 : 0
   addr.u8[0] = 88; // HIGH BYTE or 186 in decimal
   addr.u8[1] = 0;// LOW BYTE or 4 in decimal
   if(!linkaddr_cmp(&addr, &linkaddr_node_addr)) {
         unicast_send(&uc, &addr);
   }

}

static int pointer = 0;
static void messageInc() {
      message[pointer]+= 1;
      if(message[pointer] > 57) {
         if(pointer == 49) {
            pointer=0;
            message[pointer] = 48;
         }        
      else {
         pointer += 1;
         message[pointer] = 48;
      }
   }
}
// ROund 2
PROCESS_THREAD(transmit_process, ev, data) {
	PROCESS_EXITHANDLER(unicast_close(&uc);)
	PROCESS_BEGIN();
	unicast_open(&uc, 146, &unicast_callbacks);
   while (1) {
      send(message, 50);
      messageInc();
      send(message, 50);
      messageInc();
      send(message, 50);
      messageInc();
      send(message, 50);
      messageInc();

      etimer_set(&timer_etimer, CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
   }
   PROCESS_END();
}

/*---------------------------------------------------------------------------*/
