/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

   Contact Information
   OpenAirInterface Admin: openair_admin@eurecom.fr
   OpenAirInterface Tech : openair_tech@eurecom.fr
   OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

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

