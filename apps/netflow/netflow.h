/**
 * \file
 *         Send netflow data to server.
 * \author
 *         Ndizera Eddy <eddy.ndizera@student.uclouvain.be>
 *         Ivan Ahad <ivan.abdelahad@student.uclouvain.be>
 */

#ifndef NETFLOW_H_
#define NETFLOW_H_

// IPFIX FIELDS
#define OCTET_DELTA_COUNT 1
#define OCTET_DELTA_COUNT_BYTES 2

#define PACKET_DELTA_COUNT 2
#define PACKET_DELTA_COUNT_BYTES 2

// IPFIX Header values
#define TEMPLATE_ID 256
#define VERSION 10
#define DOMAIN_ID 1

#define NETFLOW_HDR_BYTES 16
#define SET_HDR_BYTES 4
#define IANA_FIELD 4
#define ENTREPRISE_FIELD 4
#define TEMPLATE_BYTES (2 * IANA_FIELD)
#define RECORD_BYTES  OCTET_DELTA_COUNT_BYTES + PACKET_DELTA_COUNT_BYTES

void initialize_netflow();

void send_template();



#endif /* NETFLOW_H_ */