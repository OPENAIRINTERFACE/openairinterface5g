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

/*
                             pdcp_proto_extern.h
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr

 ***************************************************************************/
#ifndef __PDCP_PROTO_EXTERN_H__
#    define __PDCP_PROTO_EXTERN_H__

//#    include "pdcp_entity.h"
//#    include "rb_mux.h"
#    include "mem_block.h"

#ifdef ROHC
extern void     pdcp_data_ind (module_id_t module_idP, rb_id_t rab_idP, sdu_size_t data_sizeP, mem_block_t * sduP);
extern void     pdcp_data_req (struct pdcp_entity *pdcpP, mem_block * sduP);
extern void     send_pdcp_control_primitive (struct pdcp_entity *pdcpP, mem_block * cprimitiveP);
extern void     control_pdcp (struct pdcp_entity *pdcpP);
extern void pdcp_process_input_sdus_high(struct pdcp_entity *pdcpP);
extern void     pdcp_process_input_sdus_am (struct pdcp_entity *pdcpP, uint16_t data_sizeP, mem_block * sduP);
extern void     pdcp_process_output_sdus (struct pdcp_entity *pdcpP, mem_block * sduP, uint8_t rb_idP);
extern void   pdcp_process_output_sdus_high (struct pdcp_entity *pdcpP, mem_block * sduP, uint16_t data_sizeP,  uint16_t rb_idP);
extern void     pdcp_process_input_sdus_um (struct pdcp_entity *pdcpP, uint16_t data_sizeP, mem_block * sduP);
extern void     pdcp_process_input_sdus_tr (struct pdcp_entity *pdcpP, uint16_t data_sizeP, mem_block * sduP);
extern void     init_pdcp (struct pdcp_entity *pdcpP, struct rb_dispatcher *rbP, uint8_t rb_idP);
extern void    *pdcp_tx (struct pdcp_entity *pdcpP, uint16_t data_sizeP, mem_block * sduP);
extern int  reception_from_rohc_mt(void);
extern int  reception_from_rohc_bs(void);
#else
extern BOOL     pdcp_data_ind (module_id_t module_idP, rb_id_t rab_idP, sdu_size_t data_sizeP, mem_block_t * sduP, uint8_t is_data_plane);
extern BOOL     pdcp_data_req (module_id_t module_id, uint32_t frame, uint8_t eNB_flag, rb_id_t rab_id, uint32_t muiP, uint32_t confirmP, sdu_size_t sdu_buffer_size, unsigned char* sdu_buffer,
                               uint8_t is_data_pdu);
//extern BOOL     pdcp_data_req (struct pdcp_entity *pdcpP, mem_block * sduP);
extern void     send_pdcp_control_primitive (struct pdcp_entity *pdcpP, mem_block * cprimitiveP);
extern void     control_pdcp (struct pdcp_entity *pdcpP);
extern void     pdcp_process_input_sdus_am (struct pdcp_entity *pdcpP);
extern void     pdcp_process_output_sdus (struct pdcp_entity *pdcpP, mem_block * sduP, uint8_t rb_idP);
extern void     pdcp_process_input_sdus_um (struct pdcp_entity *pdcpP);
extern void     pdcp_process_input_sdus_tr (struct pdcp_entity *pdcpP);
extern void     init_pdcp (struct pdcp_entity *pdcpP, struct rb_dispatcher *rbP, uint8_t rb_idP);
extern void    *pdcp_tx (void *argP);
#endif
#endif
