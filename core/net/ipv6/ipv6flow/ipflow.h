/**
 * \file
 *    Header file for the Contiki netflow engine
 *
 * \author
 *         Ndizera Eddy <eddy.ndizera@student.uclouvain.be>
 *         Ivan Ahad <ivan.abdelahad@student.uclouvain.be>
 */
/*---------------------------------------------------------------------------*/
#ifndef IPFLOW_H_
#define IPFLOW_H_
/*---------------------------------------------------------------------------*/
#include "net/ip/uip.h"
#include "net/ipv6/tinyipfix/tipfix.h"
/*---------------------------------------------------------------------------*/
#define MAX_FLOWS 10
#define IPFLOW_EXPORT_INTERVAL 1 // minute
#define COLLECTOR_UDP_PORT 9995
/*---------------------------------------------------------------------------*/

/** Structures definition **/
typedef struct flow{
  struct flow *next;
  uip_ipaddr_t destination;
  uint16_t size;
  uint16_t packets;
} flow_t;
/*---------------------------------------------------------------------------*/

/** Method definition **/
void launch_ipflow();
int get_process_status();
int update_flow_table(uip_ipaddr_t *destination, uint16_t size, uint16_t packets);
int get_number_flows();
void flush_flow_table();

uint8_t * get_octet_delta_count();
uint8_t * get_packet_delta_count();
uint8_t * get_destination_node_id();
uint8_t * get_source_node_id();

/*---------------------------------------------------------------------------*/

/** INFORMATION ELEMENTS FIELDS **/
#define OCTET_DELTA_COUNT create_ipfix_information_element(1, 2, 0, &get_octet_delta_count)
#define PACKET_DELTA_COUNT create_ipfix_information_element(2, 2, 0, &get_packet_delta_count)
#define DESTINATION_NODE_ID create_ipfix_information_element(32770, 2, 20763, &get_destination_node_id)
#define SOURCE_NODE_ID create_ipfix_information_element(32771, 2, 20763, &get_source_node_id)

#endif /* IPFLOW_H_ */
