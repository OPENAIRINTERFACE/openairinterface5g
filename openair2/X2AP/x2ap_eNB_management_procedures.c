/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file x2ap_eNB_management_procedures.c
 * \brief x2ap tasks for eNB
 * \author Konstantinos Alexandris <Konstantinos.Alexandris@eurecom.fr>, Cedric Roux <Cedric.Roux@eurecom.fr>, Navid Nikaein <Navid.Nikaein@eurecom.fr>
 * \date 2018
 * \version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "intertask_interface.h"

#include "assertions.h"
#include "conversions.h"

#include "x2ap_common.h"
#include "x2ap_eNB_defs.h"
#include "x2ap_eNB.h"


#define X2AP_DEBUG_LIST
#ifdef X2AP_DEBUG_LIST
#  define X2AP_eNB_LIST_OUT(x, args...) X2AP_DEBUG("[eNB]%*s"x"\n", 4*indent, "", ##args)
#else
#  define X2AP_eNB_LIST_OUT(x, args...)
#endif

static int                  indent = 0;


x2ap_eNB_internal_data_t x2ap_eNB_internal_data;

RB_GENERATE(x2ap_enb_map, x2ap_eNB_data_s, entry, x2ap_eNB_compare_assoc_id);

int x2ap_eNB_compare_assoc_id(
  struct x2ap_eNB_data_s *p1, struct x2ap_eNB_data_s *p2)
{
  if (p1->assoc_id == -1) {
    if (p1->cnx_id < p2->cnx_id) {
      return -1;
    }

    if (p1->cnx_id > p2->cnx_id) {
      return 1;
    }
  } else {
    if (p1->assoc_id < p2->assoc_id) {
      return -1;
    }

    if (p1->assoc_id > p2->assoc_id) {
      return 1;
    }
  }

  /* Matching reference */
  return 0;
}

uint16_t x2ap_eNB_fetch_add_global_cnx_id(void)
{
  return ++x2ap_eNB_internal_data.global_cnx_id;
}

void x2ap_eNB_prepare_internal_data(void)
{
  memset(&x2ap_eNB_internal_data, 0, sizeof(x2ap_eNB_internal_data));
  STAILQ_INIT(&x2ap_eNB_internal_data.x2ap_eNB_instances_head);
}

void x2ap_eNB_insert_new_instance(x2ap_eNB_instance_t *new_instance_p)
{
  DevAssert(new_instance_p != NULL);

  STAILQ_INSERT_TAIL(&x2ap_eNB_internal_data.x2ap_eNB_instances_head,
                     new_instance_p, x2ap_eNB_entries);
}

void dump_tree(x2ap_eNB_data_t *t)
{
  if (t == NULL) return;
  printf("-----------------------\n");
  printf("eNB id %d %s\n", t->eNB_id, t->eNB_name);
  printf("state %d\n", t->state);
  printf("nextstream %d\n", t->nextstream);
  printf("in_streams %d out_streams %d\n", t->in_streams, t->out_streams);
  printf("cnx_id %d assoc_id %d\n", t->cnx_id, t->assoc_id);
  dump_tree(t->entry.rbe_left);
  dump_tree(t->entry.rbe_right);
}

void dump_trees(void)
{
x2ap_eNB_instance_t *zz;
STAILQ_FOREACH(zz, &x2ap_eNB_internal_data.x2ap_eNB_instances_head,
               x2ap_eNB_entries) {
printf("here comes the tree (instance %ld):\n---------------------------------------------\n", zz->instance);
dump_tree(zz->x2ap_enb_head.rbh_root);
printf("---------------------------------------------\n");
}
}

struct x2ap_eNB_data_s *x2ap_get_eNB(x2ap_eNB_instance_t *instance_p, sctp_assoc_t assoc_id, uint16_t cnx_id)
{
  struct x2ap_eNB_data_s  temp;
  struct x2ap_eNB_data_s *found;

//printf("x2ap_get_eNB at 1 (looking for assoc_id %d cnx_id %d)\n", assoc_id, cnx_id);
//dump_trees();

  memset(&temp, 0, sizeof(struct x2ap_eNB_data_s));

  temp.assoc_id = assoc_id;
  temp.cnx_id   = cnx_id;

  if (instance_p == NULL) {
    STAILQ_FOREACH(instance_p, &x2ap_eNB_internal_data.x2ap_eNB_instances_head,
                   x2ap_eNB_entries) {
      found = RB_FIND(x2ap_enb_map, &instance_p->x2ap_enb_head, &temp);

      if (found != NULL) {
        return found;
      }
    }
  } else {
    return RB_FIND(x2ap_enb_map, &instance_p->x2ap_enb_head, &temp);
  }

  return NULL;
}


x2ap_eNB_instance_t *x2ap_eNB_get_instance(instance_t instance)
{
  x2ap_eNB_instance_t *temp = NULL;

  STAILQ_FOREACH(temp, &x2ap_eNB_internal_data.x2ap_eNB_instances_head,
                 x2ap_eNB_entries) {
    if (temp->instance == instance) {
      /* Matching occurence */
      return temp;
    }
  }

  return NULL;
}

/// utility functions

void x2ap_dump_eNB (x2ap_eNB_data_t  * eNB_ref);

void
x2ap_dump_eNB_list (void) {
   x2ap_eNB_instance_t *inst = NULL;
   struct x2ap_eNB_data_s *found = NULL;
   struct x2ap_eNB_data_s temp;

   memset(&temp, 0, sizeof(struct x2ap_eNB_data_s));

  STAILQ_FOREACH (inst, &x2ap_eNB_internal_data.x2ap_eNB_instances_head,  x2ap_eNB_entries) {
    found = RB_FIND(x2ap_enb_map, &inst->x2ap_enb_head, &temp);
    x2ap_dump_eNB (found);
  }
}

void x2ap_dump_eNB (x2ap_eNB_data_t  * eNB_ref) {

  if (eNB_ref == NULL) {
    return;
  }

  X2AP_eNB_LIST_OUT ("");
  X2AP_eNB_LIST_OUT ("eNB name:          %s", eNB_ref->eNB_name == NULL ? "not present" : eNB_ref->eNB_name);
  X2AP_eNB_LIST_OUT ("eNB STATE:         %07x", eNB_ref->state);
  X2AP_eNB_LIST_OUT ("eNB ID:            %07x", eNB_ref->eNB_id);
  indent++;
  X2AP_eNB_LIST_OUT ("SCTP cnx id:     %d", eNB_ref->cnx_id);
  X2AP_eNB_LIST_OUT ("SCTP assoc id:     %d", eNB_ref->assoc_id);
  X2AP_eNB_LIST_OUT ("SCTP instreams:    %d", eNB_ref->in_streams);
  X2AP_eNB_LIST_OUT ("SCTP outstreams:   %d", eNB_ref->out_streams);
  indent--;
}

x2ap_eNB_data_t  * x2ap_is_eNB_pci_in_list (const uint32_t pci)
{
  x2ap_eNB_instance_t    *inst;
  struct x2ap_eNB_data_s *elm;

  STAILQ_FOREACH(inst, &x2ap_eNB_internal_data.x2ap_eNB_instances_head, x2ap_eNB_entries) {
    RB_FOREACH(elm, x2ap_enb_map, &inst->x2ap_enb_head) {
      for (int i = 0; i<elm->num_cc; i++) {
        if (elm->Nid_cell[i] == pci) {
          return elm;
        }
      }
    }
  }
  return NULL;
}

x2ap_eNB_data_t  * x2ap_is_eNB_id_in_list (const uint32_t eNB_id)
{
  x2ap_eNB_instance_t    *inst;
  struct x2ap_eNB_data_s *elm;

  STAILQ_FOREACH(inst, &x2ap_eNB_internal_data.x2ap_eNB_instances_head, x2ap_eNB_entries) {
    RB_FOREACH(elm, x2ap_enb_map, &inst->x2ap_enb_head) {
      if (elm->eNB_id == eNB_id)
        return elm;
    }
  }
  return NULL;
}

x2ap_eNB_data_t  * x2ap_is_eNB_assoc_id_in_list (const uint32_t sctp_assoc_id)
{
  x2ap_eNB_instance_t    *inst;
  struct x2ap_eNB_data_s *found;
  struct x2ap_eNB_data_s temp;

  temp.assoc_id = sctp_assoc_id;
  temp.cnx_id = -1;

  STAILQ_FOREACH(inst, &x2ap_eNB_internal_data.x2ap_eNB_instances_head, x2ap_eNB_entries) {
    found = RB_FIND(x2ap_enb_map, &inst->x2ap_enb_head, &temp);
    if (found != NULL){
      if (found->assoc_id == sctp_assoc_id) {
	return found;
      }
    }
  }
  return NULL;
}
