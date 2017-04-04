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

#include "netflow.h"

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

#define LIST_NAME flow_list
#define MAX_LENGTH 10

struct flow_node {
  struct flow_list *next;
  ipflow_record_t record;
};

LIST(LIST_NAME);

/*---------------------------------------------------------------------------*/
void 
initialize()
{
  PRINTF("Initialize records list\n");
  list_init(LIST_NAME);
}

int 
update(uip_ipaddr_t *ripaddr, int size)
{
  // Update flow if already existent
  struct flow_node *current_node; 
  for(*current_node = list_head(LIST_NAME); 
      *current_node != NULL;
      current_node = list_item_next(current_node)) {
    ipflow_record_t record = current_node -> record;
    if (ripaddr -> u8[0] == record.destination){
      PRINTF("Update existent flow record\n");
      record.size = record.size + size;
      record.packets++;
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
  struct flow_node new_node;
  new_node.record = new_record;
  list_push(LIST_NAME, &new_node);
  return 1;
}

ipflow_t *
create_message()
{
  PRINTF("Create message")
  // Create header 
  int length = HDR_BYTES + FLOW_BYTES * list_length(LIST_NAME);
  ipflow_hdr_t header = {0, 1, 80, length, 1};

  // Create flow records
  ipflow_record_t records[list_length(LIST_NAME)];
  struct flow_node *current_node;
  int n = 0; 
  for(*current_node = list_head(LIST_NAME); 
      *current_node != NULL;
      current_node = list_item_next(current_node)) {
    records[n] = current_node.record;
    n++;
  }
  // TODO malloc memory for message
  ipflow_t message = {header, records};
  return &message;

}

void
flush(){
  PRINTF("Flush records list\n");
  while(list_pop(LIST_NAME) != NULL){}
}