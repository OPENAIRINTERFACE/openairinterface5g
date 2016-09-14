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

