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
static int
cmp_ipaddr(uip_ipaddr_t *in, uip_ipaddr_t *out)
{
  int i = 0;
  for(i = 0; i < 16; i++){
    if ((in -> u8[i]) != (out -> u8[i])){
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static flow_t *
create_flow(uip_ipaddr_t *destination, uint16_t size, uint16_t packets)
{
  flow_t *new_flow;
  new_flow = memb_alloc(&MEMB_FLOWS_NAME);
  memcpy(&(new_flow -> destination), destination, 16*sizeof(uint8_t));
  new_flow -> size = size;
  new_flow -> packets = packets

  return new_flow;
}
/*---------------------------------------------------------------------------*/
int
update_flow_table(uip_ipaddr_t *destination, uint16_t size, uint16_t packets)
{
  // Try to update existent flow
  flow_t *current_flow;
  for(current_flow = list_head(LIST_FLOWS_NAME);
      current_flow != NULL;
      current_flow = list_item_next(current_flow)) {
    if (cmp_ipaddr(destination, &(current_flow -> destination)) == 1){
      PRINTF("Update existent flow record\n");
      current_flow -> size = size + (current_flow -> size);
      current_flow -> packets = (current_flow -> packets) + 1;
      return 1;
    }
  }

  // Check if reached maximum size table
  if (list_length(LIST_FLOWS_NAME) >= MAX_FLOWS){
    PRINTF("Cannot add new flow record. Flow table is full.\n");
    return 0;
  }

  flow_t *new_flow = create_flow(destination, size, packets);
  list_push(LIST_FLOWS_NAME, new_flow);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
get_number_flows()
{
  return list_length(LIST_FLOWS_NAME);
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
