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
#include "net/ip/resolv.h"
#include "sys/etimer.h"
#include "netflow.h"

#include <string.h>
#include <stdio.h>

#define EXPORT_TIME 5*60*CLOCK_SECOND // 5 minutes
#define COLLECT_TIME 1*60*CLOCK_SECOND // 5 minutes

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

/*---------------------------------------------------------------------------*/
PROCESS(netflow_exporter, "Netflow Exporter");
PROCESS(netflow_collector, "Netflow Collector");
AUTOSTART_PROCESSES(&netflow_exporter, &netflow_collector);
/*---------------------------------------------------------------------------*/
void
collect()
{
  printf("Collect records\n");
}
/*---------------------------------------------------------------------------*/
void
export()
{
  printf("Send records\n");
  uip_udp_packet_sendto(client_conn, &packet, sizeof(packet),
                          &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
}
/*---------------------------------------------------------------------------*/
void flush()
{
  print("Flush\n");
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  /* set server address */
  uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(netflow_collector, ev, data)
{

  static struct etimer collect_timer;

  PROCESS_BEGIN();
  
  while(1){
    etimer_set(&collect_timer, COLLECT_TIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&collect_timer));
    collect();
  }
 
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(netflow_exporter, ev, data)
{
  static struct etimer export_timer;

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  set_global_address();

  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));
  
  while(1){
    etimer_set(&export_timer, EXPORT_TIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&export_timer));
    export();
  }
 
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/