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
#include "sys/node-id.h"
#include "net/ipv6/ipv6flow/ipflow.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

#define LIST_NAME flow_list
#define MEMB_NAME flow_memb
#define MAX_LENGTH 10

#define EXPORT_INTERVAL 5*60*CLOCK_SECOND // 5 minutes

static uint16_t seqno = 1;
process_event_t netflow_event;
static short initialized = 0;

struct flow_node {
  struct flow_node *next;
  ipflow_record_t record;
};

LIST(LIST_NAME);

MEMB(MEMB_NAME, struct flow_node, MAX_LENGTH);

/*---------------------------------------------------------------------------*/
PROCESS(flow_process, "Ip flows");
/*---------------------------------------------------------------------------*/
void 
initialize_ipflow()
{
  PRINTF("Initialize ipflow\n");
  list_init(LIST_NAME);
  memb_init(&MEMB_NAME);
  process_start(&flow_process, NULL);
  initialized = 1;
}
/*---------------------------------------------------------------------------*/
int
is_launched()
{
	return initialized;
}
/*---------------------------------------------------------------------------*/
int 
flow_update(uip_ipaddr_t *ripaddr, int size)
{
	if(initialized == 0){
		PRINTF("Not initialized\n");
		return 0;
	}	
  // Update flow if already existent
  struct flow_node *current_node; 
  for(current_node = list_head(LIST_NAME); 
      current_node != NULL;
      current_node = list_item_next(current_node)) {
    ipflow_record_t *record = &(current_node -> record);
    if (ripaddr -> u8[0] == record -> destination){
      PRINTF("Update existent flow record\n");
      record -> size = size + (record -> size);
      record -> packets = (record -> packets) + 1;
      return 1;
    }
  }

  // Cannot add anymore flow
  if (list_length(LIST_NAME) >= MAX_LENGTH){
    PRINTF("Cannot add new flow record. Flow table is full.\n");
    return 0;
  }

  // Create new flow
  PRINTF("Create new flow record\n");
  ipflow_record_t new_record = {ripaddr -> u8[0], size, 1};
  
  struct flow_node *new_node;
  new_node = memb_alloc(&MEMB_NAME);
  new_node -> record = new_record;
  list_push(LIST_NAME, new_node);
  return 1;
}
/*---------------------------------------------------------------------------*/
ipflow_t *
create_message()
{
	if(initialized == 0){
		PRINTF("Not initialized\n");
		return 0;
	}
  PRINTF("Create message\n");
  // Create header 
  // TODO get real battery value, get rpl parent
  int length = HDR_BYTES + FLOW_BYTES * list_length(LIST_NAME);
  ipflow_hdr_t header = {node_id, seqno, 80, length, 1};

  // Create flow records
  ipflow_record_t records[list_length(LIST_NAME)];
  struct flow_node *current_node;
  int n = 0; 
  for(current_node = (struct flow_node*)list_head(LIST_NAME); 
      current_node != NULL;
      current_node = list_item_next(current_node)) {
    records[n] = current_node->record;
    n++;
  }
  // TODO malloc memory for message
  ipflow_t message = {header, records};

  seqno ++;

  return &message;

}
/*---------------------------------------------------------------------------*/
void
flush(){
	if(initialized == 0){
		PRINTF("Not initialized\n");
		return ;
	}
  PRINTF("Flush records list\n");
  struct flow_node *current_node;
  for(current_node = list_pop(LIST_NAME); 
      current_node != NULL;
      current_node = list_pop(LIST_NAME)) {
    memb_free(&MEMB_NAME, current_node);
  }
}
/*---------------------------------------------------------------------------*/
void
print_table(){
  PRINTF("Flow table \n");
  struct flow_node *current_node;
  for(current_node = list_pop(LIST_NAME); 
      current_node != NULL;
      current_node = list_pop(LIST_NAME)) {
  	ipflow_record_t record = current_node -> record;
    PRINTF("Dest: %d - Size: %d - Packets: %d \n", record.destination, record.size, record.packets);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(flow_process, ev, data)
{
  static struct etimer periodic;

  PROCESS_BEGIN();

  ipflow_p = PROCESS_CURRENT();
  netflow_event = process_alloc_event();

  etimer_set(&periodic, EXPORT_INTERVAL);
  while(1){
    PROCESS_YIELD();
    if(ev == netflow_event){
    	PRINTF("Netflow event to treat\n");
    	if (data != NULL){
    		struct ipflow_event_data *event_data = data;
    		flow_update(event_data -> ripaddr, event_data -> size);
    	}
    }
    if(etimer_expired(&periodic)) {
      etimer_reset(&periodic);
      print_table();
      flush();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
