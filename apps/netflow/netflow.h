/**
 * \file
 *         Send netflow data to server.
 * \author
 *         Ndizera Eddy <eddy.ndizera@student.uclouvain.be>
 *         Ivan Ahad <ivan.abdelahad@student.uclouvain.be>
 */

#ifndef NETFLOW_H_
#define NETFLOW_H_

#define TEMPLATE_ID 256
#define VERSION 10
#define DOMAIN_ID 1

#define NETFLOW_HDR_BYTES 16
#define SET_HDR_BYTES 4
#define IANA_FIELD 4
#define ENTREPRISE_FIELD 4
#define TEMPLATE_BYTES (2 * IANA_FIELD)

// IPFIX FIELDS
#define OCTET_DELTA_COUNT 1
#define OCTET_DELTA_COUNT_BYTES 2

#define PACKET_DELTA_COUNT 2
#define PACKET_DELTA_COUNT_BYTES 2


struct nf_v10_hdr { 
  u_int16_t version;    
  u_int16_t length;
  u_int32_t export_time;
  u_int32_t seqence;
  u_int32_t domain_id;
};

struct nf_v10_set_hdr { 
  u_int16_t set_id;
  u_int16_t length;
};

struct nf_v10_data_record { 
  u_int64_t source_rime;
  u_int64_t destination_rime;
};


void initialize_netflow();

void send_template();



#endif /* NETFLOW_H_ */