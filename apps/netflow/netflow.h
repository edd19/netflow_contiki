/**
 * \file
 *         Send netflow data to server.
 * \author
 *         Ndizera Eddy <eddy.ndizera@student.uclouvain.be>
 *         Ivan Ahad <ivan.abdelahad@student.uclouvain.be>
 */

#ifndef NETFLOW_H_
#define NETFLOW_H_

#define MAX_FLOW_RECORDS 10
#define TEMPLATE_ID 256
#define VERSION 0x00a
#define DOMAIN_ID 1

struct nf_v10_hdr { 
  u_int16_t version;    
  u_int16_t length;
  u_int32_t export_time;
  u_int32_t seqence;
  u_int32_t domain_id;
};

struct nf_v10_data_record { 
  u_int64_t source_rime;
  u_int64_t destination_rime;
};

struct nf_v10_set_hdr { 
  u_int16_t set_id;
  u_int16_t length;
  struct nf_v10_data_record records[MAX_FLOW_RECORDS];
};

struct nf_v10_packet{
	struct nf_v10_hdr hdr;
	struct nf_v10_data_record data;
};

void initialize_netflow();

#endif /* NETFLOW_H_ */