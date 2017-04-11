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
#define HDR_BYTES 7
#define FLOW_BYTES 20
/*---------------------------------------------------------------------------*/
CCIF extern process_event_t netflow_event;

static struct process *ipflow_p;
/*---------------------------------------------------------------------------*/

/** Structure definition **/

typedef struct {
  uint16_t node_id;
  uint16_t no_seq;
  uint8_t battery;
  uint8_t length;
  uint8_t parent_id;
} ipflow_hdr_t;

typedef struct {
  uip_ipaddr_t destination;
  uint16_t size;
  uint16_t packets;
} ipflow_record_t;

typedef struct {
  ipflow_hdr_t hdr;
  ipflow_record_t *records;
} ipflow_t;

struct ipflow_event_data {
  uip_ipaddr_t ripaddr;
  uint16_t size;
};


/*---------------------------------------------------------------------------*/

/** Method definition **/


void initialize_ipflow();

int is_launched();

int flow_update(uip_ipaddr_t *ripaddr, int size);

void send_message();

void print_message(uint8_t *message);

void flush();
/*---------------------------------------------------------------------------*/
#endif /* IPFLOW_H_ */