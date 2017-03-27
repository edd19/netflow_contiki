/**
 * \file
 *         Send netflow data to server.
 * \author
 *         Ndizera Eddy <eddy.ndizera@student.uclouvain.be>
 *         Ivan Ahad <ivan.abdelahad@student.uclouvain.be>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl.h"
#include "net/ip/uip-debug.h"

#include "netflow.h"
#include "simple-udp.h"
#include "servreg-hack.h"

#include <stdio.h>
#include <stdlib.h>

#define EXPORT_INTERVAL 5*60*CLOCK_SECOND // 5 minutes

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define UDP_PORT 1234
#define SERVICE_ID 190

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

static struct nf_v10_data_record records[10];
static int length = 0;
static int sequence = 1;

static struct simple_udp_connection unicast_connection;
static struct uip_udp_conn *client_conn;

/*---------------------------------------------------------------------------*/
PROCESS(exporter, "Netflow collector");
AUTOSTART_PROCESSES(&exporter);
/*---------------------------------------------------------------------------*/
static void
collect(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("Collect from: ");
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
  int i = 0;
  int is_updated = 0;
  for(i = 0; i < length && i < 10; i++){
    if(records[i].source_rime ==  sender_addr -> u16[6]){
      printf("Update records\n");
      records[i].octet_delta_count = records[i].octet_delta_count + datalen;
      is_updated = 1;
    }
  }
  if(is_updated == 0 && length < 10){
    printf("Create new records\n");
    records[length].octet_delta_count = datalen;
    records[length].source_rime = sender_addr -> u16[6];
    records[length].destination_rime = receiver_addr -> u16[6];
    length = length + 1;
  }
}
/*---------------------------------------------------------------------------*/
static void
flush()
{
  memset(records, 0, sizeof(struct nf_v10_data_record)*10);
}
/*---------------------------------------------------------------------------*/
void
export(){
  struct nf_v10_data_record data[length];
  memcpy(data, records, sizeof(struct nf_v10_data_record)*length); 
  struct nf_v10_data_set data_set = {TEMPLATE_ID, length, data};
  struct nf_v10_hdr header = {VERSION, 16+4+(sizeof(struct nf_v10_data_record)*length), 0, sequence, 0};
  struct nf_v10_packet packet = {header, data_set};
  sequence = sequence + 1;

  uip_udp_packet_sendto(client_conn, &packet, 16+4+(sizeof(struct nf_v10_data_record)*length),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
  flush();
  printf("Exported\n");
}
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t *
set_global_address(void)
{
  static uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);

  printf("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }

  return &ipaddr;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(exporter, ev, data)
{   
  static struct etimer periodic;
  uip_ipaddr_t *ipaddr;

  PROCESS_BEGIN();

  servreg_hack_init();

  ipaddr = set_global_address();

  servreg_hack_register(SERVICE_ID, ipaddr);

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, collect);

  /* new connection with remote host */
  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));

  etimer_set(&periodic, EXPORT_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    if(etimer_expired(&periodic)) {
      etimer_reset(&periodic);
      export();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
