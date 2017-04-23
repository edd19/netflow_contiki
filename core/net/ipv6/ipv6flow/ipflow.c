/**
 * \file
 *    Header file for the Contiki netflow engine
 *
 * \author
 *         Ndizera Eddy <eddy.ndizera@student.uclouvain.be>
 *         Ivan Ahad <ivan.abdelahad@student.uclouvain.be>
 */
/*---------------------------------------------------------------------------*/

#include "contiki.h"
#include "contiki-lib.h"
#include "net/ipv6/ipv6flow/ipflow.h"
#include "net/ipv6/tinyipfix/tipfix.h"

#include <stdlib.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */
/*---------------------------------------------------------------------------*/
#define LIST_FLOWS_NAME flow_table
#define MEMB_FLOWS_NAME flow_memb


static int status = IDLE;
/*---------------------------------------------------------------------------*/
void
launch_ipflow()
{
  PRINTF("Launch ipflow process\n");
  status = RUNNING;
  process_start(&flow_process, NULL);
}
/*---------------------------------------------------------------------------*/
static void
initialize()
{
  PRINTF("Initialize ipflow\n");
  LIST(LIST_FLOWS_NAME);
  list_init(LIST_FLOWS_NAME);

  MEMB(MEMB_FLOWS_NAME, flow_t, MAX_FLOWS);
  memb_init(&MEMB_FLOWS_NAME);

  ipflow_p = PROCESS_CURRENT();
  netflow_event = process_alloc_event();
}
/*---------------------------------------------------------------------------*/
int
get_process_status()
{
  return status;
}
/*---------------------------------------------------------------------------*/
PROCESS(flow_process, "Ip flows");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(flow_process, ev, data)
{

  PROCESS_BEGIN();

  initialize();

  while(1){
    PROCESS_YIELD();
    if(ev == netflow_event){
    	PRINTF("Netflow event to treat\n");
    	if (data != NULL){
    		struct ipflow_event_data *event_data = data;
        free(event_data);
    	}
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
