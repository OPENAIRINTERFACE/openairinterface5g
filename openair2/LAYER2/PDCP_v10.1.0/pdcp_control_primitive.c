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

/***************************************************************************
                        pdcp_control_primitives.c
                             -------------------
    begin                : Mon Dec 10 2001
    email                : Lionel.Gauthier@eurecom.fr
                             -------------------
    description
    This file contains the functions used for configuration of pdcp

 ***************************************************************************/
#include "rtos_header.h"
#include "platform.h"
#include "protocol_vars_extern.h"
#include "print.h"
//-----------------------------------------------------------------------------
#include "rlc.h"
#include "pdcp.h"
#include "debug_l2.h"
//-----------------------------------------------------------------------------
void
configure_pdcp_req (struct pdcp_entity *pdcpP, void *rlcP, uint8_t rlc_sap_typeP, uint8_t header_compression_typeP)
{
  //-----------------------------------------------------------------------------
  mem_block      *mb;

  mb = get_free_mem_block (sizeof (struct cpdcp_primitive), __func__);
  if(mb==NULL) return;
  ((struct cpdcp_primitive *) mb->data)->type = CPDCP_CONFIG_REQ;
  ((struct cpdcp_primitive *) mb->data)->primitive.config_req.rlc_sap = rlcP;
  ((struct cpdcp_primitive *) mb->data)->primitive.config_req.rlc_type_sap = rlc_sap_typeP;
  ((struct cpdcp_primitive *) mb->data)->primitive.config_req.header_compression_type = header_compression_typeP;
  send_pdcp_control_primitive (pdcpP, mb);
}
