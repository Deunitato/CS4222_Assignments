#include <stdio.h>
#include <stdlib.h>

#include "contiki.h"
#include "sys/rtimer.h"

#include "board-peripherals.h"

#include <stdint.h>

#include "sys/etimer.h"
#include "buzzer.h"

PROCESS(process_rtimer, "RTimer");
AUTOSTART_PROCESSES(&process_rtimer);

static struct rtimer timer_rtimer;
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / 4;  
static rtimer_clock_t timeout_rtimer_buzzer_off = RTIMER_SECOND * 3;
/*---------------------------------------------------------------------------*/
static void init_opt_reading(void);
static void buzzer(void);
static void get_light_reading(void);

/*---------------------------------------------------------------------------*/

int buzzerSound = 2637;
static int prevLUX = 0;
static int f = 0;
static int value = 0;
static int diff = 0;


void
do_rtimer_timeout(struct rtimer *timer, void *ptr)
{
  //Re-arm rtimer. Starting up the sensor takes around 125ms 
  //rtimer period 2s 

  //rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
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


PROCESS_THREAD(process_rtimer, ev, data)
{
  static struct etimer timer_etimer;
  static struct etimer timer_etimer_buzzer;
  PROCESS_BEGIN();
  init_opt_reading();
  buzzer_init();
  
  /*
  printf(" The value of RTIMER_SECOND is %d \n",RTIMER_SECOND);
  printf(" The value of timeout_rtimer_buzzer_off is %d \n",timeout_rtimer_buzzer_off);
  printf(" The value of timeout_rtimer is %d \n",timeout_rtimer);*/

  while(1) {
    /*if (f == 1){ //if buzzer is on, ill poll 3 seconds
       printf("\n Timer starting with %d \n",CLOCK_SECOND * 3);
       etimer_set(&timer_etimer, 3 * CLOCK_SECOND);
    }
    else { */
    //rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0,  do_rtimer_timeout, NULL);
     printf("\n Timer starting with %d \n",CLOCK_SECOND);
     etimer_set(&timer_etimer, CLOCK_SECOND);
    //}
    //do a check 
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_etimer));
    get_light_reading();
    printf("DIFF = %d lux ,  prevLux = %d\n", diff, prevLUX/100);
    printf("Current f: %d\n\n", f);
    diff = abs(value - prevLUX)/100;
    prevLUX = value;
    if(diff >= 200){
      if (f == 1){ //Change buzzer state to off
	 diff = 0;

         printf("\n\n\nBUZZER STOPED\n\n\n\n");
	 buzzer_stop(); 

         printf("\n (OFFING STATE) BUZZER_Timer starting with %d \n",CLOCK_SECOND * 3);
         etimer_set(&timer_etimer_buzzer, 3 * CLOCK_SECOND); 
         PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_etimer_buzzer));  
         printf("\n BUZZER_Timer END \n");

         f = 0;     
      }
      else if (f == 0){ //change buzzer state to on
         diff = 0;

         printf("\n (ONING STATE) BUZZER_Timer starting with %d \n",CLOCK_SECOND * 3);
         etimer_set(&timer_etimer_buzzer, 3 * CLOCK_SECOND); 
         PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_etimer_buzzer));
         printf("\n BUZZER_Timer END \n");

         printf("\n\n\nBUZZER ON\n\n\n\n"); 
         buzzer_start(buzzerSound); 
         f = 1;  
      }
    }
    //PROCESS_YIELD();
  }

  PROCESS_END();
}
