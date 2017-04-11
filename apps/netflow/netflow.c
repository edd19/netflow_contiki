/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "sys/clock.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/ipv6flow/ipflow.h"

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#include <stdio.h>
#include <string.h>

#include "netflow.h"

#define UDP_PORT 1230

static struct uip_udp_conn *server_connection;

static struct uip_udp_conn *client_connection;
static uip_ipaddr_t server_addr;

static uint32_t seqno = 1;

/*---------------------------------------------------------------------------*/
PROCESS(ipflow_receiver_process, "Ipflow receiver process");
/*---------------------------------------------------------------------------*/
void
initialize_netflow()
{
  process_start(&ipflow_receiver_process, NULL);
}
/*---------------------------------------------------------------------------*/
void 
print_template(uint8_t *message)
{
  PRINTF("IPFIX header:\n");
  uint16_t *version = (uint16_t *)&message[0];
  uint16_t *length = (uint16_t*)&message[2];
  uint32_t *export_time = (uint32_t *)&message[4];
  uint32_t *seq_no = (uint32_t *)&message[8];
  uint32_t *domain = (uint32_t *)&message[12];

  PRINTF("Version: %d - Length: %d Export time: %lu - Sequence No: %lu - Domain: %lu \n",
         *version, *length, *export_time, *seq_no, *domain);

  PRINTF("Template : \n");
  uint16_t *template_id = (uint16_t *)&message[16];
  uint16_t *length_template = (uint16_t *)&message[18];

  PRINTF("HDR \t Id: %d - Length: %d\n", *template_id, *length_template);

  uint16_t *octet_delta_count = (uint16_t *)&message[20];
  uint16_t *octet_delta_count_bytes = (uint16_t *)&message[22];
  uint16_t *packet_delta_count = (uint16_t *)&message[24];
  uint16_t *packet_delta_count_bytes = (uint16_t *)&message[26];
  PRINTF("Fields \t  Octet: %d (%d) - Packet:%d (%d)\n",
         *octet_delta_count, *octet_delta_count_bytes, *packet_delta_count,
         *packet_delta_count_bytes);

}
/*---------------------------------------------------------------------------*/
void
send_template()
{
  PRINTF("Sending template\n");
  // TODO check if HTONS
  // Create template
  uint16_t version = VERSION;
  uint16_t length = NETFLOW_HDR_BYTES + SET_HDR_BYTES + TEMPLATE_BYTES;
  uint32_t export_time = (uint32_t)clock_seconds();
  uint32_t domain = DOMAIN_ID;
  uint8_t message[length];
  memcpy(message, &version, sizeof(uint16_t));
  memcpy(&message[2], &length, sizeof(uint16_t));
  memcpy(&message[4], &export_time, sizeof(uint32_t));
  memcpy(&message[8], &seqno, sizeof(uint32_t));
  memcpy(&message[12], &domain, sizeof(uint32_t));


  //Create template
  uint16_t template_id = TEMPLATE_ID;
  uint16_t length_template = SET_HDR_BYTES + TEMPLATE_BYTES;
  memcpy(&message[16], &template_id, sizeof(uint16_t));
  memcpy(&message[18], &length_template, sizeof(uint16_t));

  uint16_t octet_delta_count = OCTET_DELTA_COUNT;
  uint16_t octet_delta_count_bytes = OCTET_DELTA_COUNT_BYTES;
  uint16_t packet_delta_count = PACKET_DELTA_COUNT;
  uint16_t packet_delta_count_bytes = PACKET_DELTA_COUNT_BYTES;
  memcpy(&message[20], &octet_delta_count, sizeof(uint16_t));
  memcpy(&message[22], &octet_delta_count_bytes, sizeof(uint16_t));
  memcpy(&message[24], &packet_delta_count, sizeof(uint16_t));
  memcpy(&message[26], &packet_delta_count_bytes, sizeof(uint16_t));

  print_template(message);
  PRINTF("Send message of len: %d \n", length);
  uip_udp_packet_sendto(client_connection, &message, length * sizeof(uint8_t),
                        &server_addr, UIP_HTONS(UDP_PORT));

  seqno++;

}
/*---------------------------------------------------------------------------*/
void
send_record(uint8_t *ipflow_message)
{
  uint16_t *id = (uint16_t *)&ipflow_message[0];
  uint16_t *seq = (uint16_t *)&ipflow_message[2];
  uint8_t *battery = &ipflow_message[3];
  uint8_t *length = &ipflow_message[4];
  uint8_t *parent = &ipflow_message[5];

  int number_records = (*length - HDR_BYTES) / FLOW_BYTES;

  uint16_t version = VERSION;
  uint16_t length_message = NETFLOW_HDR_BYTES + SET_HDR_BYTES + (RECORD_BYTES * number_records);
  uint32_t domain = DOMAIN_ID;
  uint8_t message[(int)length];
  memcpy(message, &version, sizeof(uint16_t));
  memcpy(&message[2], &length_message, sizeof(uint16_t));
  message[4] = 0;
  message[5] = 0;
  message[6] = 0;
  message[7] = 0;
  memcpy(&message[8], &seqno, sizeof(uint32_t));
  memcpy(&message[12], &domain, sizeof(uint32_t));

  uint16_t template_id = TEMPLATE_ID;
  uint16_t length_data = SET_HDR_BYTES + (RECORD_BYTES * number_records);
  memcpy(&message[16], &template_id, sizeof(uint16_t));
  memcpy(&message[18], &length_data, sizeof(uint16_t));

  int offset = 20;
  int i = 0;
  for(i = 0; i < number_records; i++){
    int ipflow_offset = HDR_BYTES + (i*FLOW_BYTES);
    uint16_t *size = (uint16_t *)&message[offset+1];
    uint16_t *packets = (uint16_t *)&message[offset+3];

    memcpy(&message[offset], size, sizeof(uint16_t));
    memcpy(&message[offset+2], packets, sizeof(uint16_t));
    offset = offset + 4;
  }

  PRINTF("Send message of len: %d \n", length_message);
  uip_udp_packet_sendto(client_connection, &message, length_message * sizeof(uint8_t),
                        &server_addr, UIP_HTONS(UDP_PORT));
}
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  uint8_t *message;
  if(uip_newdata()) {
    PRINTF("Received new ipflow data\n");
    message = (uint8_t *)uip_appdata;
    print_message(message);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ipflow_receiver_process, ev, data)
{

  PROCESS_BEGIN();

  PRINTF("Begin ipflow receiver process\n");

  server_connection = udp_new(NULL, UIP_HTONS(UDP_PORT), NULL);
  if(server_connection == NULL) {
    PRINTF("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(server_connection, UIP_HTONS(UDP_PORT));

  client_connection = udp_new(NULL, UIP_HTONS(UDP_PORT), NULL); 
  if(client_connection == NULL) {
    PRINTF("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(client_connection, UIP_HTONS(UDP_PORT)); 

  PRINTF("Ready to listen for incoming ipflow message\n");

  send_template();

  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
