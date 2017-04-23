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
/*---------------------------------------------------------------------------*/
#define MAX_FLOWS 10

#define IPFLOW_EXPORT_INTERVAL 1 // minute

/*---------------------------------------------------------------------------*/

/** Structures definition **/
typedef struct flow{
  uip_ipaddr_t destination;
  uint16_t size;
  uint16_t packets;
  struct flow *next;
} flow_t;
/*---------------------------------------------------------------------------*/

/** Method definition **/
void launch_ipflow();
int get_process_status();
int update_flow_table(uip_ipaddr_t *destination, uint16_t size, uint16_t packets);
int get_number_flows();
void flush_flow_table();

#endif /* IPFLOW_H_ */
