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
static void convert_to_big_endian(uint8_t *src, uint8_t *dst, int size);
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
  new_template -> n = 0;
  new_template -> element_head = NULL;

  return new_template;
}
/*---------------------------------------------------------------------------*/
void
add_element_to_template(template_t *template, information_element_t *element)
{
  if(template -> element_head == NULL){
    template -> element_head = element;
  }
  else{
    information_element_t *current_element; 
    for(current_element = template -> element_head;
      current_element -> next != NULL; 
      current_element = current_element -> next){}
    current_element -> next = element;
  }
  template -> n = (template -> n) + 1;
}
/*---------------------------------------------------------------------------*/
void
free_template(template_t *template)
{
  // Free all elements
  information_element_t *current_element;
  for(current_element = template -> element_head;
      current_element != NULL;
      current_element = current_element -> next) {
    free_information_element(current_element);
  }
  template -> element_head = NULL;
  template -> n = 0;
  // Free template
  memb_free(&MEMB_TEMPLATES_NAME, template);
}
/*---------------------------------------------------------------------------*/
int
add_ipfix_header(uint8_t *ipfix_message, ipfix_t *ipfix)
{
  uint32_t ipfix_export_time = clock_seconds();
  uint8_t big_endian_version[2];
  convert_to_big_endian((uint8_t *)&(ipfix -> version), big_endian_version, 2);
  uint8_t big_endian_export_time[4];
  convert_to_big_endian((uint8_t *)&ipfix_export_time, big_endian_export_time, 4);
  uint8_t big_endian_sequence_number[4];
  convert_to_big_endian((uint8_t *)&sequence_number, big_endian_sequence_number, 4);
  uint8_t big_endian_domain_id[4];
  convert_to_big_endian((uint8_t *)&(ipfix->domain_id), big_endian_domain_id, 4);

  memcpy(ipfix_message, big_endian_version, sizeof(uint16_t));
  memcpy(&ipfix_message[4], big_endian_export_time, sizeof(uint32_t));
  memcpy(&ipfix_message[8], big_endian_sequence_number, sizeof(uint32_t));
  memcpy(&ipfix_message[12], big_endian_domain_id, sizeof(uint32_t));

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
    number_records = (template -> compute_number_records)();
  }

  for(i = 0; i < number_records; i++){
    information_element_t *current_element;
    for(current_element = template -> element_head;
        current_element != NULL;
        current_element = current_element -> next) {
      if(type == IPFIX_TEMPLATE){
        uint8_t big_endian_id[2];
        convert_to_big_endian((uint8_t *)&(current_element -> id), big_endian_id, 2);
        uint8_t big_endian_size[2];
        convert_to_big_endian((uint8_t *)&(current_element -> size), big_endian_size, 2);
        memcpy(&ipfix_message[offset+length_data], big_endian_id, sizeof(uint16_t));
        memcpy(&ipfix_message[offset+length_data+2], big_endian_size, sizeof(uint16_t));
        length_data = length_data + 4;

        if(current_element -> eid != 0){
          uint8_t big_endian_eid[4];
          convert_to_big_endian((uint8_t *)&(current_element -> eid), big_endian_eid, 4);
          memcpy(&ipfix_message[offset+length_data+4], big_endian_eid, sizeof(uint32_t));
          length_data = length_data + 4;
        }
      }
      else{
        uint8_t *value = (current_element -> f)();
        uint8_t big_endian_value[current_element -> size];
        convert_to_big_endian(value, big_endian_value, current_element -> size);
        memcpy(&ipfix_message[offset + length_data], big_endian_value, sizeof(uint8_t) * (current_element -> size));
        length_data = length_data + (current_element -> size);
      }
    }
  }

  //Set header
  uint8_t big_endian_template_id[2];
  convert_to_big_endian((uint8_t *)&(template -> id), big_endian_template_id, 2);
  memcpy(&ipfix_message[offset], big_endian_template_id, sizeof(uint16_t));
  if(type == IPFIX_TEMPLATE){
    uint8_t big_endian_count[2];
    convert_to_big_endian((uint8_t *)&(template -> n), big_endian_count, sizeof(uint16_t));
    memcpy(&ipfix_message[offset+2], big_endian_count, sizeof(uint16_t));
  }
  else{
    uint8_t big_endian_length_data[2];
    convert_to_big_endian((uint8_t *)&length_data, big_endian_length_data, sizeof(uint16_t));
    memcpy(&ipfix_message[offset+2], big_endian_length_data, sizeof(uint16_t));
  }

  return offset + length_data;
}
/*---------------------------------------------------------------------------*/
ipfix_t *
create_ipfix()
{
  ipfix_t *new_ipfix = memb_alloc(&MEMB_IPFIX_NAME);
  new_ipfix -> version = IPFIX_VERSION;
  new_ipfix -> domain_id = IPFIX_DOMAIN_ID;
  new_ipfix -> n = 0;
  new_ipfix -> template_head = NULL;

  return new_ipfix;
}
/*---------------------------------------------------------------------------*/
void
add_templates_to_ipfix(ipfix_t *ipfix, template_t *template)
{
  if(ipfix -> template_head == NULL){
    ipfix -> template_head = template;
  }
  else{
    template_t *current_template;
    for(current_template = ipfix -> template_head;
      current_template -> next != NULL; 
      current_template = current_template -> next){}
    current_template -> next = template;
  }
  ipfix -> n = (ipfix -> n) + 1;
}
/*---------------------------------------------------------------------------*/
void
free_ipfix(ipfix_t *ipfix)
{
  template_t *current_template;
  for(current_template = ipfix -> template_head;
      current_template != NULL;
      current_template = current_template -> next) {
    free_template(current_template);
  }
  ipfix -> template_head = NULL;
  ipfix -> n = 0;

  memb_free(&MEMB_IPFIX_NAME, ipfix);

}
/*---------------------------------------------------------------------------*/
int
generate_ipfix_message(uint8_t *ipfix_message, ipfix_t *ipfix, int type)
{
  int offset = 0;
  offset = add_ipfix_header(ipfix_message, ipfix);
  if(type == IPFIX_TEMPLATE){
    offset = offset + 4;
  }

  template_t *current_template;
  for(current_template = ipfix -> template_head;
      current_template != NULL;
      current_template = current_template -> next) {
    offset = add_ipfix_records_or_template(ipfix_message, current_template, offset, type);
  }

  if(type == IPFIX_TEMPLATE){
    uint16_t template_id = 2;
    uint8_t big_endian_template_id[2];
    convert_to_big_endian((uint8_t *)&template_id, big_endian_template_id, 2);
    memcpy(&ipfix_message[IPFIX_HEADER_LENGTH], big_endian_template_id, sizeof(uint16_t));

    uint8_t big_endian_length_data[2];
    uint16_t length_set = offset - IPFIX_HEADER_LENGTH;
    convert_to_big_endian((uint8_t *)&length_set, big_endian_length_data, 2);
    memcpy(&ipfix_message[IPFIX_HEADER_LENGTH+2], big_endian_length_data, sizeof(uint16_t));
  }

  uint8_t big_endian_size[2];
  convert_to_big_endian((uint8_t *)(uint16_t *)&offset, big_endian_size, 2);
  memcpy(&ipfix_message[2], big_endian_size, sizeof(uint16_t));

  return offset;
}
/*---------------------------------------------------------------------------*/
static void
convert_to_big_endian(uint8_t *src, uint8_t *dst, int size)
{
  int i = 0;
  for(i = 0; i < size; i++){
    dst[size-i-1] = src[i];
  }
}
/*---------------------------------------------------------------------------*/
