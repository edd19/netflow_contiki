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

#define IPFIX_HEADER_LENGTH 20

#define MAX_TEMPLATES 3
#define MAX_INFORMATION_ELEMENTS 10

/** Structures definitions **/

typedef struct {
  information_element_t *next;
  uint16_t id;
  uint16_t size;
  uint32_t entreprise_id;
  void *f;
} information_element_t;

typedef struct {
  uint16_t id;
  uint8_t n;
  list_t elements;
} template_t;

/*---------------------------------------------------------------------------*/

/** Methods definitions **/

// Methods to create the structure
information_element_t *create_ipfix_information_element(uint16_t id, uint16_t size, uint32_t eid, void* f);
void free_information_element(information_element_t * element);

template_t *create_ipfix_template(int id);
void add_element_to_template(template_t *template, information_element_t *element);
void free_template(template_t *template);

// Methods to create ipfix or tipifx message
int add_ipfix_header(uint8_t *ipfix_message);
int add_tipfix_header(uint8_t *ipfix_message);
int add_ipfix_template(uint8_t *ipfix_message, template_t *template, int offset);

//Methods to convert tiny ipfix to ipfix
int tipifx_to_ipfix(uint8_t *tipfix_message, uint8_t *ipfix_message);

/*---------------------------------------------------------------------------*/
