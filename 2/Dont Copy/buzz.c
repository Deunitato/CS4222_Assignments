/*
 * Copyright (C) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "contiki.h"
#include "sys/etimer.h"
#include "buzzer.h"
#include "board-peripherals.h"

#include <stdint.h>

PROCESS(process_buzz, "buzz");
AUTOSTART_PROCESSES(&process_buzz);

static int counter;
static struct etimer timer_etimer;
static int prevValue = 0;
static bool isIdle = true;
static bool isBuzzing = false;
/*---------------------------------------------------------------------------*/
static void init_opt_reading(void);
static void get_light_reading(void);

/*---------------------------------------------------------------------------*/
void
do_timer_timeout()
{
  /* Re-arm rtimer. Starting up the sensor takes around 125ms */
  /* rtimer period 2s */
  int s, ms1,ms2,ms3;
  s = clock_time() / CLOCK_SECOND;
  ms1 = (clock_time()% CLOCK_SECOND)*10/CLOCK_SECOND;
  ms2 = ((clock_time()% CLOCK_SECOND)*100/CLOCK_SECOND)%10;
  ms3 = ((clock_time()% CLOCK_SECOND)*1000/CLOCK_SECOND)%10;
  
  counter++;
  printf(": %d (cnt) %d (ticks) %d.%d%d%d (sec) \n",counter,clock_time(), s, ms1,ms2,ms3); 
  get_light_reading();

}

static void
get_light_reading()
{
  int value;

  value = opt_3001_sensor.value(0);
  int diff = prevValue - value / 100;
  if(value != CC26XX_SENSOR_READING_ERROR) {
    printf("OPT: Light=%d.%02d lux, diff=%d lux\n", value / 100, value % 100, diff);
  } else {
    printf("OPT: Light Sensor's Warming Up\n\n");
  }
  if (diff > 300 || diff < -300) {
    isIdle = false;
    if (isBuzzing) {
      buzzer_stop();
      isBuzzing = false;
    } else {
      buzzer_start(2093);
      isBuzzing = true;
    }
  } else {
    isIdle = true;
  }
  prevValue = value / 100;
  init_opt_reading();
}

static void
init_opt_reading(void)
{
  SENSORS_ACTIVATE(opt_3001_sensor);
}

PROCESS_THREAD(process_buzz, ev, data)
{
  PROCESS_BEGIN();
  init_opt_reading();

  while(1) {
    if (isBuzzing) {
      etimer_set(&timer_etimer, CLOCK_SECOND / 4);
    } else {
      etimer_set(&timer_etimer, CLOCK_SECOND * 3);
    }
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
    do_timer_timeout();
  }

  PROCESS_END();
}
