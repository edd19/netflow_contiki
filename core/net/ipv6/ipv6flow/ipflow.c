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
#include "lib/list.h"
#include "lib/memb.h"
#include "net/ip/uip-udp-packet.h"
#include "net/ipv6/ipv6flow/ipflow.h"
#include "net/ipv6/tinyipfix/tipfix.h"
#include "sys/node-id.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
#define LIST_FLOWS_NAME flow_table
#define MEMB_FLOWS_NAME flow_memb
MEMB(MEMB_FLOWS_NAME, flow_t, MAX_FLOWS);

LIST(LIST_FLOWS_NAME);

static int status = 0;
static ipfix_t *ipflow_ipfix = NULL;

static struct uip_udp_conn *exporter_connection;
static uip_ipaddr_t collector_addr;
static flow_t * temp_flow;
static int compression = NO_COMPRESSION;
/*---------------------------------------------------------------------------*/
static void initialize();
static int cmp_ipaddr(uip_ipaddr_t *in, uip_ipaddr_t *out);
static flow_t * create_flow(uip_ipaddr_t *destination, uint16_t size, uint16_t packets);
static ipfix_t * ipfix_for_ipflow();
static void send_ipfix_message(int type, int compression);
/*---------------------------------------------------------------------------*/
PROCESS(standard_process, "Standard Ip flows");
PROCESS(aggregator_process, "Ip flows for aggretor nodes");
PROCESS(gateway_process, "Ip flows for gateway nodes");
/*---------------------------------------------------------------------------*/
void
launch_ipflow(int compression_mode, int role)
{
  status = 1;
  compression = compression_mode;
  if(role == STANDARD){
    process_start(&standard_process, NULL);
  }
  else if(role == AGGREGATOR){
    // AGGRETOR process
  }
  else{
    //GATEWAY process
  }
}
/*---------------------------------------------------------------------------*/
static void
initialize()
{
  list_init(LIST_FLOWS_NAME);
  memb_init(&MEMB_FLOWS_NAME);

  uip_ip6addr(&collector_addr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);

  temp_flow = NULL;

  ipflow_ipfix = ipfix_for_ipflow();

  initialize_tipfix();
}
/*---------------------------------------------------------------------------*/
void
set_collector_addr(uip_ipaddr_t *addr)
{
  collector_addr = *addr;
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
  flow_t *new_flow = memb_alloc(&MEMB_FLOWS_NAME);
  memcpy(&(new_flow -> destination), destination, 16*sizeof(uint8_t));
  new_flow -> size = size;
  new_flow -> packets = packets;

  return new_flow;
}
/*---------------------------------------------------------------------------*/
int
update_flow_table(uip_ipaddr_t *destination, uint16_t size, uint16_t packets)
{
  if(get_process_status() != 1){
    return 0;
  }

  // Try to update existent flow
  flow_t *current_flow;
  for(current_flow = list_head(LIST_FLOWS_NAME);
      current_flow != NULL;
      current_flow = list_item_next(current_flow)) {
    if (cmp_ipaddr(destination, &(current_flow -> destination)) == 1){
      current_flow -> size = size + (current_flow -> size);
      current_flow -> packets = (current_flow -> packets) + 1;
      return 1;
    }
  }

  // Check if reached maximum size table
  if (get_number_flows() >= MAX_FLOWS){
    printf("Reached maximum flows\n");
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
  if(get_process_status() != 1){
    return 0;
  }
  return list_length(LIST_FLOWS_NAME);
}
/*---------------------------------------------------------------------------*/
int
get_process_status()
{
  return status;
}
/*---------------------------------------------------------------------------*/
void
flush_flow_table()
{
  if(get_process_status() != 1){
    return;
  }

  flow_t *current_flow;
  for(current_flow = list_pop(LIST_FLOWS_NAME);
     current_flow != NULL;
     current_flow = list_pop(LIST_FLOWS_NAME)) {
    memb_free(&MEMB_FLOWS_NAME, current_flow);
  }
}
/*---------------------------------------------------------------------------*/
uint8_t *
get_octet_delta_count()
{
  return (uint8_t *)&(temp_flow -> size) ;
}
/*---------------------------------------------------------------------------*/
uint8_t *
get_packet_delta_count()
{
  return (uint8_t *)&(temp_flow -> packets);
}
/*---------------------------------------------------------------------------*/
uint8_t *
get_source_node_id()
{
  return (uint8_t *)&node_id;
}
/*---------------------------------------------------------------------------*/
uint8_t *
get_destination_node_id()
{
  flow_t *flow = temp_flow;
  temp_flow = temp_flow -> next;
  static uint16_t temp = 0;
  temp = (flow -> destination).u16[7];
  temp = UIP_HTONS(temp);
  return (uint8_t *)&temp;
}
/*---------------------------------------------------------------------------*/
static ipfix_t *
ipfix_for_ipflow()
{
  template_t *template = create_ipfix_template(256, &get_number_flows);

  add_element_to_template(template, OCTET_DELTA_COUNT);
  add_element_to_template(template, PACKET_DELTA_COUNT);
  add_element_to_template(template, SOURCE_NODE_ID);
  add_element_to_template(template, DESTINATION_NODE_ID);

  ipfix_t *ipfix = create_ipfix();
  add_templates_to_ipfix(ipfix, template);

  return ipfix;
}
/*---------------------------------------------------------------------------*/
static void
send_ipfix_message(int type, int compression)
{
  uint8_t message[200];
  int length;
  if(compression == NO_COMPRESSION){
    length = generate_ipfix_message(message, ipflow_ipfix, type);
  }
  else{
    length = generate_tipfix_message(message, ipflow_ipfix, type);
  }

  uip_udp_packet_sendto(exporter_connection, &message, length * sizeof(uint8_t),
                        &collector_addr, UIP_HTONS(COLLECTOR_UDP_PORT));
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(standard_process, ev, data)
{
  static struct etimer periodic;

  PROCESS_BEGIN();

  initialize();

  exporter_connection = udp_new(NULL, UIP_HTONS(COLLECTOR_UDP_PORT), NULL);
  if(exporter_connection == NULL) {
    PROCESS_EXIT();
  }
  udp_bind(exporter_connection, UIP_HTONS(COLLECTOR_UDP_PORT));

  PROCESS_PAUSE();

  // Send template
  etimer_set(&periodic, IPFLOW_EXPORT_INTERVAL*20*CLOCK_SECOND);
  PROCESS_YIELD_UNTIL(etimer_expired(&periodic));
  send_ipfix_message(IPFIX_TEMPLATE, compression);

  // Send data
  etimer_set(&periodic, IPFLOW_EXPORT_INTERVAL*60*CLOCK_SECOND);
  while(1){
    PROCESS_YIELD_UNTIL(etimer_expired(&periodic));
    etimer_reset(&periodic);

    temp_flow = list_head(LIST_FLOWS_NAME);
    send_ipfix_message(IPFIX_DATA, compression);
    flush_flow_table();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static char aggrega[500];
static int length_aggrega = 0;
static int received = 0;
/*---------------------------------------------------------------------------*/
static void
update_aggregate_message(char *msg, int length) {
  if(length_aggrega == 0){
    memcpy(aggrega, msg, sizeof(char)*length);
    length_aggrega = length_aggrega + length;
  }
  else{
    char cpy[500];
    memcpy(cpy, aggrega, sizeof(char)*length_aggrega);
    length_aggrega = aggregate_message((uint8_t *)cpy, (uint8_t *)msg,
     (uint8_t *)aggrega);
  }
  received++;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(aggregator_process, ev, data)
{
  static struct etimer periodic;

  PROCESS_BEGIN();

  initialize();

  exporter_connection = udp_new(NULL, UIP_HTONS(COLLECTOR_UDP_PORT), NULL);
  if(exporter_connection == NULL) {
    PROCESS_EXIT();
  }
  udp_bind(exporter_connection, UIP_HTONS(COLLECTOR_UDP_PORT));

  PROCESS_PAUSE();

  // Send template
  etimer_set(&periodic, IPFLOW_EXPORT_INTERVAL*30*CLOCK_SECOND);
  PROCESS_YIELD_UNTIL(etimer_expired(&periodic));
  send_ipfix_message(IPFIX_TEMPLATE, compression);

  // Send data
  etimer_set(&periodic, IPFLOW_EXPORT_INTERVAL*60*CLOCK_SECOND);
  while(1){
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      if(uip_newdata()) {
        update_aggregate_message(uip_appdata, uip_datalen());
      }
    }
    if(etimer_expired(&periodic)) {
      temp_flow = list_head(LIST_FLOWS_NAME);

      uint8_t message[200];
      int length = generate_tipfix_message(message, ipflow_ipfix, type);
      update_aggregate_message(message, length);

      uip_udp_packet_sendto(exporter_connection, &aggrega, length_aggrega * sizeof(uint8_t),
                            &collector_addr, UIP_HTONS(COLLECTOR_UDP_PORT));

      flush_flow_table();
      etimer_reset(&periodic);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(gateway_process, ev, data)
{
  PROCESS_BEGIN();

  initialize();

  exporter_connection = udp_new(NULL, UIP_HTONS(COLLECTOR_UDP_PORT), NULL);
  if(exporter_connection == NULL) {
    PROCESS_EXIT();
  }
  udp_bind(exporter_connection, UIP_HTONS(COLLECTOR_UDP_PORT));

  while(1){
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      if(uip_newdata()) {
        uint16_t sender_node_id = 0;
        sender_node_id = (UIP_IP_BUF->srcipaddr).u16[7];
        sender_node_id = UIP_HTONS(sender_node_id);
        char msg[300];
        int length = tipifx_to_ipfix(uip_appdata, sender_node_id, msg);

        uip_udp_packet_sendto(exporter_connection, &msg, length * sizeof(uint8_t),
                              &collector_addr, UIP_HTONS(COLLECTOR_UDP_PORT));
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
