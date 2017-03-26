/**
 * \file
 *         Send netflow data to server.
 * \author
 *         Ndizera Eddy <eddy.ndizera@student.uclouvain.be>
 *         Ivan Ahad <ivan.abdelahad@student.uclouvain.be>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/resolv.h"
#include "sys/etimer.h"

#include <string.h>
#include <stdio.h>

#define INTERVAL_TIME 1*60*CLOCK_SECOND // 5 minutes


/*---------------------------------------------------------------------------*/
PROCESS(netflow_client_process, "Netflow Client process");
AUTOSTART_PROCESSES(&netflow_client_process);
/*---------------------------------------------------------------------------*/
void
send_records()
{
  printf("Send records\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(netflow_client_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();
  
  while(1){
    etimer_set(&et, INTERVAL_TIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    send_records();
  }
 
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/