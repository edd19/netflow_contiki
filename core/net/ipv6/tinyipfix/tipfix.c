/**
 * \file
 *    Tiny IPFIX library.
 *    Functions to easily create Tiny IPFIX or IPFIX messages.
 *
 * \author
 *         Ndizera Eddy <eddy.ndizera@student.uclouvain.be>
 *         Ivan Ahad <ivan.abdelahad@student.uclouvain.be>
 */
/*---------------------------------------------------------------------------*/
#include "lib/list.h"
#include "lib/memb.h"
#include "net/ipv6/tinyipfix/tipfix.h"
#include "sys/clock.h"

#include <string.h>

/********* Memory blocks for structures **************/
#define MEMB_IPFIX_NAME ipfix_memb
#define MEMB_TEMPLATES_NAME template_memb
#define MEMB_INFO_ELEM_NAME info_elem_memb

MEMB(MEMB_IPFIX_NAME, ipfix_t, MAX_IPFIX);
MEMB(MEMB_TEMPLATES_NAME, template_t, MAX_TEMPLATES);
MEMB(MEMB_INFO_ELEM_NAME, information_element_t, MAX_INFORMATION_ELEMENTS);

/********* IPFIX context variables **************/
static uint32_t sequence_number = IPFIX_SEQUENCE;
static int initialized;
/*---------------------------------------------------------------------------*/
void 
initialize_tipfix()
{
  if(initialized != 0){
    memb_init(&MEMB_IPFIX_NAME);
    memb_init(&MEMB_TEMPLATES_NAME);
    memb_init(&MEMB_INFO_ELEM_NAME);
    initialized = 1;
  }
}
/*---------------------------------------------------------------------------*/
information_element_t *
create_ipfix_information_element(uint16_t id, uint16_t size, uint32_t eid, uint8_t* (*f)())
{
  information_element_t * new_element = memb_alloc(&MEMB_INFO_ELEM_NAME);
  new_element -> id = id;
  new_element -> size = size;
  new_element -> eid = eid;
  new_element -> f = f;
  new_element -> next = NULL;

  return new_element;
}
/*---------------------------------------------------------------------------*/
void
free_information_element(information_element_t * element)
{
  memb_free(&MEMB_INFO_ELEM_NAME, element);
}
/*---------------------------------------------------------------------------*/
template_t *
create_ipfix_template(int id, int (*compute_number_records)())
{
  template_t *new_template = memb_alloc(&MEMB_TEMPLATES_NAME);
  new_template -> id = id;
  new_template -> next = NULL;
  new_template -> compute_number_records = compute_number_records;
  list_init(new_template -> elements);

  return new_template;
}
/*---------------------------------------------------------------------------*/
void
add_element_to_template(template_t *template, information_element_t *element)
{
  list_add(template -> elements, element);
}
/*---------------------------------------------------------------------------*/
void
free_template(template_t *template)
{
  // Free all elements
  information_element_t *current_element;
  for(current_element = list_pop(template -> elements);
      current_element != NULL;
      current_element = list_pop(template -> elements)) {
    free_information_element(current_element);
  }
  // Free template
  memb_free(&MEMB_TEMPLATES_NAME, template);
}
/*---------------------------------------------------------------------------*/
int
add_ipfix_header(uint8_t *ipfix_message, ipfix_t *ipfix)
{
  uint32_t ipfix_export_time = clock_seconds();
  memcpy(ipfix_message, &(ipfix -> version), sizeof(uint16_t));
  memcpy(&ipfix_message[4], &ipfix_export_time, sizeof(uint32_t));
  memcpy(&ipfix_message[8], &sequence_number, sizeof(uint32_t));
  memcpy(&ipfix_message[12], &(ipfix -> domain_id), sizeof(uint32_t));

  sequence_number++;

  return IPFIX_HEADER_LENGTH;
}
/*---------------------------------------------------------------------------*/
int
add_ipfix_records_or_template(uint8_t *ipfix_message, template_t *template, int offset, int type)
{
  int length_data = IPFIX_SET_HEADER_LENGTH;

  //Set data records
  int i = 0;
  int number_records = 1;
  if (type != IPFIX_TEMPLATE){
    number_records = (*(template -> compute_number_records))();
  }

  for(i = 0; i < number_records; i++){
    information_element_t *current_element;
    for(current_element = list_head(template -> elements);
    current_element != NULL;
    current_element = list_item_next(template -> elements)) {
      if(type == IPFIX_TEMPLATE){
        memcpy(&ipfix_message[offset+length_data], &(current_element -> id), sizeof(uint16_t));
        memcpy(&ipfix_message[offset+length_data+2], &(current_element -> size), sizeof(uint16_t));
        length_data = length_data + 4;

        if(current_element -> eid != 0){
          memcpy(&ipfix_message[offset+length_data+4], &(current_element -> eid), sizeof(uint32_t));
          length_data = length_data + 4;
        }
      }
      else{
        uint8_t *value = (*(current_element -> f))();
        memcpy(&ipfix_message[offset + length_data], value, sizeof(uint8_t) * (current_element -> size));
        length_data = length_data + (current_element -> size);
      }
    }
  }

  // Set header
  memcpy(&ipfix_message[offset], &(template -> id), sizeof(uint16_t));
  memcpy(&ipfix_message[offset+2], &length_data, sizeof(uint16_t));

  return offset + length_data;
}
/*---------------------------------------------------------------------------*/
ipfix_t *
create_ipfix()
{
  ipfix_t *new_ipfix = memb_alloc(&MEMB_IPFIX_NAME);
  new_ipfix -> version = IPFIX_VERSION;
  new_ipfix -> domain_id = IPFIX_DOMAIN_ID;
  list_init(new_ipfix -> templates);

  return new_ipfix;
}
/*---------------------------------------------------------------------------*/
void
add_templates_to_ipfix(ipfix_t *ipfix, template_t *template)
{
  list_add(ipfix -> templates, template);
}
/*---------------------------------------------------------------------------*/
void
free_ipfix(ipfix_t *ipfix)
{
  template_t *current_template;
  for(current_template = list_pop(ipfix -> templates);
      current_template != NULL;
      current_template = list_pop(ipfix -> templates)) {
    free_template(current_template);
  }

  memb_free(&MEMB_IPFIX_NAME, ipfix);

}
/*---------------------------------------------------------------------------*/
int
generate_ipfix_message(uint8_t *ipfix_message, ipfix_t *ipfix, int type)
{
  int offset = 0;
  offset = add_ipfix_header(ipfix_message, ipfix);

  template_t *current_template;
  for(current_template = list_head(ipfix -> templates);
      current_template != NULL;
      current_template = list_item_next(current_template)) {
    offset = add_ipfix_records_or_template(ipfix_message, current_template, offset, type);
  }

  memcpy(&ipfix_message[2], (uint16_t *)&offset, sizeof(uint16_t));

  return offset;
}
/*---------------------------------------------------------------------------*/
