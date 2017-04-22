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
#include "contiki.h"
#include "contiki-lib.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

/********* Memory blocks for templates and information elements **************/
#define MEMB_IPFIX_NAME ipfix_memb
#define MEMB_TEMPLATES_NAME template_memb
#define MEMB_INFO_ELEM_NAME info_elem_memb

MEMB(MEMB_IPFIX_NAME, ipfix_t, MAX_IPFIX);
MEMB(MEMB_TEMPLATES_NAME, template_t, MAX_TEMPLATES);
MEMB(MEMB_INFO_ELEM_NAME, information_element_t, MAX_INFORMATION_ELEMENTS);

static uint32_t sequence_number = IPFIX_SEQUENCE;
static uint32_t domain_id = IPFIX_DOMAIN_ID;
/*---------------------------------------------------------------------------*/
information_element_t *
create_ipfix_information_element(uint16_t id, uint16_t size, uint32_t eid, void* f)
{
  PRINTF("Create new information: id %d, size %d, eid %d\n",
         id, size, eid);
  information_element_t * new_element;
  new_element = memb_alloc(&MEMB_INFO_ELEM_NAME);
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
create_ipfix_template(int id)
{
  PRINTF("Create new template id.%d\n", id);
  template_t *new_template;
  new_template = memb_alloc(&MEMB_TEMPLATES_NAME);
  new_template -> id = id;
  new_template -> n = 0;
  new_template -> next = NULL;
  list_init(new_template -> elements);
}
/*---------------------------------------------------------------------------*/
void
add_element_to_template(template_t *template, information_element_t *element)
{
  PRINTF("Add new information element to template no %d\n", template -> id);
  list_add(template -> elements, element);
  template -> n = list_length(template -> elements);
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
add_ipfix_header(uint8_t *ipfix_message, ipfix_t *ipfix, int type)
{
  PRINTF("Add ipfix header\n");
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
add_ipfix_template(uint8_t *ipfix_message, template_t *template, int offset)
{
  PRINTF("Add ipfix template %d\n", template -> id);
  int length_template = IPFIX_SET_HEADER_LENGTH;

  //Set information elements
  information_element_t *current_element;
  for(current_element = list_head(template -> elements);
      current_element != NULL;
      current_element = list_item_next(template -> elements)) {
    memcpy(&ipfix_message[offset+length_template], &(current_element -> id), sizeo(uint16_t));
    memcpy(&ipfix_message[offset+length_template+2], &(current_element -> size), sizeo(uint16_t));
    length_template = length_template + 4;

    if(entreprise_id != 0){
      memcpy(&ipfix_message[offset+length_template+4], &(current_element -> entreprise_id), sizeo(uint32_t));
      length_template = length_template + 4;
    }
  }

  // Set header
  memcpy(&ipfix_message[offset], &(template -> id), sizeof(uint16_t));
  memcpy(&ipfix_message[offset+2], &length_template, sizeof(uint16_t));

  return offset + length_template;
}
/*---------------------------------------------------------------------------*/
int
add_ipfix_records(uint8_t *ipfix_message, template_t *template, int offset)
{
  PRINTF("Add ipfix records for template %d\n", template -> id);
  int length_template = IPFIX_SET_HEADER_LENGTH;

  //Set data records
  int i = 0;
  int number_records = (template -> compute_number_records)()
  for(i = 0; i < number_records; i++){
    information_element_t *current_element;
    for(current_element = list_head(template -> elements);
    current_element != NULL;
    current_element = list_item_next(template -> elements)) {
      uint8_t *value = (current_element -> f)(); // TODO free value
      memcpy(&ipfix_message[offset + length_data], value, sizeof(uint8_t) * (current_element -> size));
      length_data = length_data + (current_element -> size);
    }
  }

  // Set header
  memcpy(&ipfix_message[offset], &(template -> id), sizeof(uint16_t));
  memcpy(&ipfix_message[offset+2], &length_template, sizeof(uint16_t));

  return offset + length_data;
}
/*---------------------------------------------------------------------------*/
ipfix_t *
create_ipfix()
{
  ipfix_t *new_ipfix;
  new_ipfix = memb_alloc(&MEMB_IPFIX_NAME);
  new_ipfix -> version = IPFIX_VERSION;
  new_ipfix -> length = IPFIX_HEADER_LENGTH;
  new_ipfix -> domain_id = IPFIX_DOMAIN_ID;
  new_ipfix -> n = 0;
  list_init(new_ipfix -> templates);

  return new_ipfix;
}
/*---------------------------------------------------------------------------*/
void
add_templates_to_ipfix(ipfix_t *ipfix, template_t *template)
{
  list_add(ipfix -> templates, template);
  ipfix -> n = list_length(template);
  ipfix -> length_template = (ipfix -> length_template) + (template -> size_template);
  ipfix -> length_data = (ipfix -> length_data) + (template -> size_data)
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
}
/*---------------------------------------------------------------------------*/
void
set_ipfix_length(uint8_t *ipfix_message, uint16_t length)
{
    memcpy(&ipfix_message[2], &length, sizeof(uint16_t));
}
/*---------------------------------------------------------------------------*/
int
generate_ipfix_message(uint8_t *ipfix_message, ipfix_t *ipfix, int type)
{
  PRINTF("Generate ipfix message\n");
  int offset = 0;
  offset = add_ipfix_header(ipfix_message, ipfix);

  if(type == IPFIX_TEMPLATE){
    template_t *current_template;
    for(current_template = list_pop(ipfix -> templates);
        current_template != NULL;
        current_template = list_pop(ipfix -> templates)) {
      offset = add_ipfix_template(ipfix_message, current_template, offset);
    }
  }
  else{
    template_t *current_template;
    for(current_template = list_pop(ipfix -> templates);
        current_template != NULL;
        current_template = list_pop(ipfix -> templates)) {
      offset = add_ipfix_records(ipfix_message, current_template, offset);
    }
  }

  set_ipfix_length(ipfix_message, (uint16_t)offset);

  return offset;
}
/*---------------------------------------------------------------------------*/
