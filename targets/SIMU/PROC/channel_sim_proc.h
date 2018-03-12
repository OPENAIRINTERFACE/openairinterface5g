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
 * channel_sim_proc.h
 *
 */

#ifndef CHANNEL_SIM_PROC_H_
#define CHANNEL_SIM_PROC_H_

void mmap_enb(int id,int **tx_data[3],int **rx_data[3],LTE_DL_FRAME_PARMS *frame_parms);

void mmap_ue(int id,int ***tx_data,int ***rx_data,LTE_DL_FRAME_PARMS *frame_parms);

void Clean_Param(double **r_re,double **r_im,LTE_DL_FRAME_PARMS *frame_parms);

void do_DL_sig_channel_T(void *param);

void do_UL_sig_channel_T(void *param);

void init_rre(LTE_DL_FRAME_PARMS *frame_parms,double ***r_re0,double ***r_im0);

void Channel_Out(lte_subframe_t direction,int eNB_id,int UE_id,double **r_re,double **r_im,double **r_re0,double **r_im0,LTE_DL_FRAME_PARMS *frame_parms);

#endif /* CHANNEL_SIM_PROC_H_ */

