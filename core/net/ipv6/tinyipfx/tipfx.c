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

#define MEMB_TEMPLATES_NAME template_memb
#define MEMB_INFO_ELEM_NAME info_elem_memb

MEMB(MEMB_TEMPLATES_NAME, template_t, MAX_TEMPLATES);
MEMB(MEMB_INFO_ELEM_NAME, information_element_t, MAX_INFORMATION_ELEMENTS);
/*---------------------------------------------------------------------------*/
information_element_t *
create_ipfix_information_element(uint16_t id, uint16_t size, uint32_t eid, void* f)
{
  PRINTF("Create new information: id %d, size %d, eid %d\n",
         id, size, eid);
  information_element_t * new_element;
  new_element = memb_alloc(&MEMB_NAME);
  new_element -> id = id;
  new_element -> size = size;
  new_element -> eid = eid;
  new_element -> f = f;

  return new_element;
}
/*---------------------------------------------------------------------------*/
void
free_information_element(information_element_t * element)
{
  memb_free(&MEMB_INFO_ELEM_NAME, element);
}
/*---------------------------------------------------------------------------*/
void
free_template(template_t *template)
{
  // TODO free also the informations elements
  memb_free(&MEMB_TEMPLATES_NAME, template);
}
