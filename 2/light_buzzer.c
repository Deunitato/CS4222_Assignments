#include <stdio.h>

#include "contiki.h"
#include "sys/rtimer.h"

#include "board-peripherals.h"

#include <stdint.h>

#include "sys/etimer.h"
#include "buzzer.h"

PROCESS(process_rtimer, "RTimer");
PROCESS(process_etimer, "ETimer");
AUTOSTART_PROCESSES(&process_rtimer, &process_etimer);

static int counter_rtimer;
static struct rtimer timer_rtimer;
static struct etimer timer_etimer;
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / 4;  
/*---------------------------------------------------------------------------*/
static void init_opt_reading(void);
static void get_light_reading(void);

/*---------------------------------------------------------------------------*/

int buzzerSound = 2637;
static int prevLUX = 0;
static int int f = 0;
static int value = 0;


void
do_rtimer_timeout(struct rtimer *timer, void *ptr)
{
  /* Re-arm rtimer. Starting up the sensor takes around 125ms */
  /* rtimer period 2s */
  clock_time_t t;

  rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
  get_light_reading();

}


static void
get_light_reading()
{

  value = opt_3001_sensor.value(0);
  if(value != CC26XX_SENSOR_READING_ERROR) {
    printf("OPT: Light=%d.%02d lux\n", value / 100, value % 100);
  } else {
    printf("OPT: Light Sensor's Warming Up\n\n");
  }
  init_opt_reading();
}

static void
init_opt_reading(void)
{
  SENSORS_ACTIVATE(opt_3001_sensor);
}



static void
buzzer()
{
  printf("\n\n\nBUZZER CALLED\n\n\n\n");
  if (f == 0 && buzzer_running()) 
	buzzer_stop(); 
  else if (f == 1 && !buzzer_running())
	buzzer_start(buzzerSound);

}

PROCESS_THREAD(process_etimer, ev, data)
{
  
  PROCESS_BEGIN();
  buzzer_init();
  
  //process_exit(&process_rtimer);
  //printf(" The value of CLOCK_SECOND is %d \n",CLOCK_SECOND);
  //etimer_set(&timer_etimer, CLOCK_SECOND);  //1s timer
  
  //process_start(&process_rtimer, NULL);
  while(1) {
  	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_etimer));
	buzzer();
        process_poll(&process_rtimer);
        PROCESS_YIELD();
  }
   
   

  PROCESS_END();
}



PROCESS_THREAD(process_rtimer, ev, data)
{
  PROCESS_BEGIN();
  init_opt_reading();
  

  printf(" The value of RTIMER_SECOND is %d \n",RTIMER_SECOND);
  printf(" The value of timeout_rtimer is %d \n",timeout_rtimer);

  while(1) {
    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0,  do_rtimer_timeout, NULL);
    //do a check
    int diff = value/100 - prevLUX/100;
    printf("DIFF =%d lux\n", diff);
    prevLUX = value;
    
    if(diff >= 300 || diff <= -300){
      if (f == 1){
         f = 0;
      }
      else {
         f = 1;
      }
      etimer_set(&timer_etimer, 4 * CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_etimer));
     }
    PROCESS_YIELD();
  }

  PROCESS_END();
}
