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
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include "net/ipv6/ipv6flow/ipflow.h"

#include <stdlib.h>

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
#define UDP_PORT 1230

static uint16_t seqno = 1;
process_event_t netflow_event;
static short initialized = 0;
static struct uip_udp_conn *client_connection;
static uip_ipaddr_t server_addr;

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
  uip_ip6addr(&server_addr, 0xaaaa, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
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
int 
flow_update(uip_ipaddr_t *ripaddr, int size)
{
  // TODO size is incorrect
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
    if (cmp_ipaddr(ripaddr, &(record -> destination)) == 1){
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
  ipflow_record_t new_record;
  memcpy(&(new_record.destination), ripaddr, 16 * sizeof(uint8_t));
  new_record.size = size;
  new_record.packets = 1;
  
  struct flow_node *new_node;
  new_node = memb_alloc(&MEMB_NAME);
  new_node -> record = new_record;
  list_push(LIST_NAME, new_node);
  return 1;
}
/*---------------------------------------------------------------------------*/
void
send_message()
{
	if(initialized == 0){
		PRINTF("Not initialized\n");
		return ;
	}
  PRINTF("Create message\n");
  // Create header 
  // TODO get real battery value, get rpl parent
  int length = HDR_BYTES + (FLOW_BYTES * list_length(LIST_NAME));
  uint8_t message[length]; 
  memcpy(message, &node_id, sizeof(uint16_t));
  memcpy(&message[2], &seqno, sizeof(uint16_t));
  message[4] = 0;
  message[5] = length;
  message[6] = 0;

  // Create flow records
  struct flow_node *current_node;
  int n = 0;
  for(current_node = (struct flow_node*)list_head(LIST_NAME); 
      current_node != NULL;
      current_node = list_item_next(current_node)) {
    int offset = HDR_BYTES + (n*FLOW_BYTES);
    int j = 0;
    for(j = 0; j < 16; j++){
      message[offset+j] = (current_node -> record).destination.u8[j]; 
    }
    memcpy(&message[offset+16], &(current_node -> record).size, sizeof(uint16_t));
    memcpy(&message[offset+18], &(current_node -> record).packets, sizeof(uint16_t));
    n++;
  }

  print_message(message);

  seqno ++;

  PRINTF("Send message of len: %d \n", length);
  uip_udp_packet_sendto(client_connection, &message, length * sizeof(uint8_t),
                        &server_addr, UIP_HTONS(UDP_PORT));

}
/*---------------------------------------------------------------------------*/
void
print_message(uint8_t *message)
{
  PRINTF("**Header**\n");
  uint16_t *id = (uint16_t *)&message[0];
  uint16_t *seq = (uint16_t *)&message[2];
  PRINTF("Node_id : %d - Seq no: %d - Battery: %d - Length: %d - Parent id: %d \n",
         *id, *seq, message[4], message[5], message[6]);

  PRINTF("**Records**\n");
  int length = message[5];
  int number_records = (length - HDR_BYTES) / FLOW_BYTES;
  int i = 0;
  for(i = 0; i < number_records; i++){
    int offset = HDR_BYTES + (i*FLOW_BYTES);
    uint16_t *size = (uint16_t *)&message[offset+16];
    uint16_t *packets = (uint16_t *)&message[offset+18];
    PRINTF("No.%d - Size:%d -Packets:%d  - Destination: ",
           i+1, *size, *packets);
    int j = 0;
    for(j = 0; j < 16; j++){
      PRINTF("%x ", message[offset+j]);
    }
    PRINTF("\n");
  }
}
/*---------------------------------------------------------------------------*/
void
flush()
{
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
PROCESS_THREAD(flow_process, ev, data)
{
  static struct etimer periodic;

  PROCESS_BEGIN();

  ipflow_p = PROCESS_CURRENT();
  netflow_event = process_alloc_event();

  client_connection = udp_new(NULL, UIP_HTONS(UDP_PORT), NULL); 
  if(client_connection == NULL) {
    PRINTF("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(client_connection, UIP_HTONS(UDP_PORT)); 

  etimer_set(&periodic, EXPORT_INTERVAL);
  while(1){
    PROCESS_YIELD();
    if(ev == netflow_event){
    	PRINTF("Netflow event to treat\n");
    	if (data != NULL){
    		struct ipflow_event_data *event_data = data;
    		flow_update(&(event_data -> ripaddr), event_data -> size);
        free(event_data);
    	}
    }
    if(etimer_expired(&periodic)) {
      etimer_reset(&periodic);
      send_message();
      flush();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
