/*
 *  Lighter version of powertrace.
 */

#include "contiki.h"
#include "benchmark.h"
#include "sys/ctimer.h"
#include "sys/energest.h"
#include <stdio.h>
#include <string.h>

#define PERIOD 60
#define ENERGEST_INTERVAL		(PERIOD * CLOCK_SECOND)

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

static unsigned long last_rx, last_tx, last_cpu, last_lpm;
/*---------------------------------------------------------------------------*/
PROCESS(energest_process, "Energest process");
/*---------------------------------------------------------------------------*/
void
launch_energest()
{
  process_start(&energest_process, NULL);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(energest_process, ev, data)
{
    static struct etimer periodic;

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  etimer_set(&periodic, ENERGEST_INTERVAL);
  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&periodic)) {
      unsigned long all_rx, all_tx, all_cpu, all_lpm;
      all_rx = energest_type_time(ENERGEST_TYPE_LISTEN);
      all_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);
      all_cpu = energest_type_time(ENERGEST_TYPE_CPU);
      all_lpm = energest_type_time(ENERGEST_TYPE_LPM);

      PRINTF("ENERGY rx %lu tx %lu lpm %lu cpu %lu\n",
          ((all_rx - last_rx)*1000)/RTIMER_SECOND,
          ((all_tx - last_tx)*1000)/RTIMER_SECOND,
          ((all_lpm - last_lpm)*1000)/RTIMER_SECOND,
          ((all_cpu - last_cpu)*1000)/RTIMER_SECOND);

      last_rx = energest_type_time(ENERGEST_TYPE_LISTEN);
      last_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);
      last_lpm = energest_type_time(ENERGEST_TYPE_LPM);
      last_cpu = energest_type_time(ENERGEST_TYPE_CPU);

      etimer_reset(&periodic);
    }
  }

  PROCESS_END();
}
