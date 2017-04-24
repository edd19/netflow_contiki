/**
 * \file
 *    Header file for the Tiny IPFIX.
 *
 * \author
 *         Ndizera Eddy <eddy.ndizera@student.uclouvain.be>
 *         Ivan Ahad <ivan.abdelahad@student.uclouvain.be>
 */
/*---------------------------------------------------------------------------*/
#include "lib/list.h"
/*---------------------------------------------------------------------------*/
#ifndef TIPFIX_H_
#define TIPFIX_H_
/*---------------------------------------------------------------------------*/

#define IPFIX_VERSION 10
#define IPFIX_SEQUENCE 1
#define IPFIX_DOMAIN_ID 1

#define IPFIX_TEMPLATE_ID 256

#define IPFIX_HEADER_LENGTH 16
#define IPFIX_SET_HEADER_LENGTH 4

#define MAX_IPFIX 3
#define MAX_TEMPLATES 3
#define MAX_INFORMATION_ELEMENTS 10

#define IPFIX_TEMPLATE 1
#define IPFIX_DATA 2

/** Structures definitions **/

 typedef struct information_element{
  struct information_element *next;
  uint16_t id;
  uint16_t size;
  uint32_t eid;
  uint8_t* (*f)();         // function that compute element value
} information_element_t;

typedef struct template{
  struct template *next;
  uint16_t id;
  int (*compute_number_records)();
  list_t elements;
}template_t;

typedef struct ipfix{
  uint16_t version;
  uint32_t domain_id;
  list_t templates;
}ipfix_t;

/*---------------------------------------------------------------------------*/

/** Methods definitions **/

void initialize_tipfix();

// Methods to create the structure
information_element_t *create_ipfix_information_element(uint16_t id, uint16_t size, uint32_t eid, uint8_t* (*f)());
void free_information_element(information_element_t * element);

template_t *create_ipfix_template(int id, int (*compute_number_records)());
void add_element_to_template(template_t *template, information_element_t *element);
void free_template(template_t *template);

ipfix_t *create_ipfix();
void add_templates_to_ipfix(ipfix_t *ipfix, template_t *template);
void free_ipfix(ipfix_t *ipfix);
int generate_ipfix_message(uint8_t *ipfix_message, ipfix_t *ipfix, int type);

// Methods to create ipfix or tipifx message
int add_ipfix_header(uint8_t *ipfix_message, ipfix_t *ipfix);
int add_tipfix_header(uint8_t *ipfix_message, ipfix_t *ipfix);
int add_ipfix_records_or_template(uint8_t *ipfix_message, template_t *template, int offset, int type);


//Methods to convert tiny ipfix to ipfix
int tipifx_to_ipfix(uint8_t *tipfix_message, uint8_t *ipfix_message);

/*---------------------------------------------------------------------------*/

#endif
