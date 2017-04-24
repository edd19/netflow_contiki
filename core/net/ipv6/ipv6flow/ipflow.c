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

/*---------------------------------------------------------------------------*/
#define LIST_FLOWS_NAME flow_table
#define MEMB_FLOWS_NAME flow_memb
MEMB(MEMB_FLOWS_NAME, flow_t, MAX_FLOWS);

LIST(LIST_FLOWS_NAME);

static int status = 0;
static ipfix_t *ipflow_ipfix = NULL;

static struct uip_udp_conn *exporter_connection;
static uip_ipaddr_t collector_addr;
/*---------------------------------------------------------------------------*/
static void initialize();
static int cmp_ipaddr(uip_ipaddr_t *in, uip_ipaddr_t *out);
static flow_t * create_flow(uip_ipaddr_t *destination, uint16_t size, uint16_t packets);
static ipfix_t * ipfix_for_ipflow();
static void send_ipfix_message(int type);
/*---------------------------------------------------------------------------*/
PROCESS(flow_process, "Ip flows");
/*---------------------------------------------------------------------------*/
void
launch_ipflow()
{
  status = 1;
  process_start(&flow_process, NULL);
}
/*---------------------------------------------------------------------------*/
static void
initialize()
{  
  list_init(LIST_FLOWS_NAME);
  memb_init(&MEMB_FLOWS_NAME);

  uip_ip6addr(&collector_addr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);

  ipflow_ipfix = ipfix_for_ipflow();
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
  flow_t *flow = list_head(LIST_FLOWS_NAME);
  return (uint8_t *)&(flow -> size) ;
}
/*---------------------------------------------------------------------------*/
uint8_t *
get_packet_delta_count()
{
  flow_t *flow = list_head(LIST_FLOWS_NAME);
  return (uint8_t *)&(flow -> packets);
}
/*---------------------------------------------------------------------------*/
// uint8_t *
// get_source_address()
// {
//   // TODO get host address
// }
/*---------------------------------------------------------------------------*/
uint8_t *
get_destination_address()
{
  flow_t *flow =list_pop(LIST_FLOWS_NAME);
  return (uint8_t *)&(flow -> destination);
}
/*---------------------------------------------------------------------------*/
static ipfix_t *
ipfix_for_ipflow()
{
  template_t *template = create_ipfix_template(256, &get_number_flows);

  add_element_to_template(template, OCTET_DELTA_COUNT);
  add_element_to_template(template, PACKET_DELTA_COUNT);
  add_element_to_template(template, DESTINATION_IPV6_ADDRESS);

  ipfix_t *ipfix = create_ipfix();
  add_templates_to_ipfix(ipfix, template);

  return ipfix;
}
/*---------------------------------------------------------------------------*/
static void
send_ipfix_message(int type)
{
  uint8_t message[200];
  int length = generate_ipfix_message(message, ipflow_ipfix, type);

  uip_udp_packet_sendto(exporter_connection, &message, length * sizeof(uint8_t), 
                        &collector_addr, UIP_HTONS(COLLECTOR_UDP_PORT));
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(flow_process, ev, data)
{
  static struct etimer periodic;

  PROCESS_BEGIN();

  initialize();
  initialize_tipfix();

  exporter_connection = udp_new(NULL, UIP_HTONS(COLLECTOR_UDP_PORT), NULL); 
  if(exporter_connection == NULL) {
    PROCESS_EXIT();
  }
  udp_bind(exporter_connection, UIP_HTONS(COLLECTOR_UDP_PORT));

  send_ipfix_message(IPFIX_TEMPLATE);

  etimer_set(&periodic, IPFLOW_EXPORT_INTERVAL*60*CLOCK_SECOND);
  while(1){
    PROCESS_YIELD();
    if(etimer_expired(&periodic)) {
      etimer_reset(&periodic);
      send_ipfix_message(IPFIX_DATA);
      flush_flow_table();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
