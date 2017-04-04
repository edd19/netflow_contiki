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
#define FLOW_BYTES 6
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
  uint16_t destination;
  uint16_t size;
  uint16_t packets;
} ipflow_record_t;

typedef struct {
  ipflow_hdr_t hdr;
  ipflow_record_t *records;
} ipflow_t;


/*---------------------------------------------------------------------------*/

/** Method definition **/


void initialize();

int update(uip_ipaddr_t *ripaddr, int size);

ipflow_t * create_message();

void free_message(ipflow_t * message);

void flush();
/*---------------------------------------------------------------------------*/
