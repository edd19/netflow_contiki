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
#define MEMB_TEMPLATES_NAME template_memb
#define MEMB_INFO_ELEM_NAME info_elem_memb

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
add_ipfix_header(uint8_t *ipfix_message)
{
  PRINTF("Add ipfix header\n");
  uint16_t ipfix_version = IPFIX_VERSION;
  uint16_t ipfix_length = IPFIX_HEADER_LENGTH;
  uint32_t ipfix_export_time = clock_seconds();
  memcpy(ipfix_message, (uint16_t *)ipfix_version, sizeof(uint16_t));
  memcpy(ipfix_message, (uint16_t *)ipfix_length, sizeof(uint16_t));
  memcpy(ipfix_message, (uint32_t *)ipfix_export_time, sizeof(uint32_t));
  memcpy(ipfix_message, (uint32_t *)sequence_number, sizeof(uint32_t));
  memcpy(ipfix_message, (uint32_t *)domain_id, sizeof(uint32_t));

  sequence_number++;
  
  return IPFIX_HEADER_LENGTH;
}
