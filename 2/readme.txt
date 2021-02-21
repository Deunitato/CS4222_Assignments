From the output of etimer-buzzer.c, note down the value of CLOCK_SECOND. Find out
how many clock ticks corresponds to 1s in real time.




The value of CLOCK_SECOND is 128 


Time(E): 1 (cnt) 152 (ticks) 1.187 (sec) => 128
Time(E): 2 (cnt) 281 (ticks) 2.195 (sec) => 129
Time(E): 3 (cnt) 410 (ticks) 3.203 (sec) => 128
Time(E): 4 (cnt) 539 (ticks) 4.210 (sec) => 128
Time(E): 5 (cnt) 668 (ticks) 5.218 (sec) => 128

Estimated clock ticks per 1 sec is 128



From the output of rtimer-lightSensor.c. note down the value of RTIMER_SECOND. Find
out how many clock ticks corresponds to 1s in real time


The value of RTIMER_SECOND is 65536 
The value of timeout_rtimer is 16384 

: 1 (cnt) 28 (ticks) 0.218 (sec) => 128.4
OPT: Light Sensor's Warming Up
: 2 (cnt) 60 (ticks) 0.468 (sec) => 128.2
OPT: Light=291.68 lux
: 3 (cnt) 92 (ticks) 0.718 (sec) =>128.12
OPT: Light=269.28 lux
: 4 (cnt) 124 (ticks) 0.968 (sec) =>128.1 
OPT: Light=225.28 lux
: 5 (cnt) 156 (ticks) 1.218 (sec) => 128.1
OPT: Light=110.24 lux
: 6 (cnt) 188 (ticks) 1.468 (sec) => 128.06
OPT: Light=103.52 lux


Estimated to be around 128 ticks per second as well although the timing is not that accurate compared to etimer due to the use of real time.


INSTRUCTIONS TO RUN:
- Build the bin using the makefile given
	make TARGET=srf06-cc26xx BOARD=sensortag/cc2650 buzz.bin CPU_FAMILY=cc26xx




