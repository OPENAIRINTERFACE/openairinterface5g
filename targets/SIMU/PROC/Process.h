#ifndef __PROCESS_H__
#    define __PROCESS_H__

#include "interface.h"

int32_t **tx[MAX_eNB+MAX_UE][3],**rx[MAX_eNB+MAX_UE][3];
int nslot;

void Process_Func(int node_id,int port,double **r_re0,double **r_im0,double **r_re,double **r_im,double **s_re,double **s_im,
                  node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms);

void UE_Inst(int node_id,int port,double **r_re0,double **r_im0,double **r_re,double **r_im,double **s_re,double **s_im,
             node_desc_t *ue_data[NUMBER_OF_UE_MAX],uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms);

void eNB_Inst(int node_id,int port,double **r_re0,double **r_im0,double **r_re,double **r_im,double **s_re,double **s_im,
              node_desc_t *enb_data[NUMBER_OF_eNB_MAX],uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms);

void Channel_Inst(int node_id,int port,double **s_re[MAX_eNB+MAX_UE],double **s_im[MAX_eNB+MAX_UE],double **r_re[MAX_eNB+MAX_UE],double **r_im[MAX_eNB+MAX_UE],double **r_re0,double **r_im0,
                  double **r_re0_d[MAX_UE][MAX_eNB],double **r_im0_d[MAX_UE][MAX_eNB],double **r_re0_u[MAX_eNB][MAX_UE],double **r_im0_u[MAX_eNB][MAX_UE],channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX],
                  channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX],node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms);

void Channel_DL(double **s_re[MAX_eNB+MAX_UE],double **s_im[MAX_eNB+MAX_UE],double **r_re[MAX_eNB+MAX_UE],double **r_im[MAX_eNB+MAX_UE],double **r_re0,double **r_im0,
                double **r_re0_d[MAX_UE][MAX_eNB],double **r_im0_d[MAX_UE][MAX_eNB],double **r_re0_u[MAX_eNB][MAX_UE],double **r_im0_u[MAX_eNB][MAX_UE],channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX],
                channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX],node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms,
                int slot);

void Channel_UL(double **s_re[MAX_eNB+MAX_UE],double **s_im[MAX_eNB+MAX_UE],double **r_re[MAX_eNB+MAX_UE],double **r_im[MAX_eNB+MAX_UE],double **r_re0,double **r_im0,
                double **r_re0_d[MAX_UE][MAX_eNB],double **r_im0_d[MAX_UE][MAX_eNB],double **r_re0_u[MAX_eNB][MAX_UE],double **r_im0_u[MAX_eNB][MAX_UE],channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX],
                channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX],node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms,
                int slot);

void Channel_Func(double **s_re[MAX_eNB+MAX_UE],double **s_im[MAX_eNB+MAX_UE],double **r_re[MAX_eNB+MAX_UE],double **r_im[MAX_eNB+MAX_UE],double **r_re0,double **r_im0,
                  double **r_re0_d[MAX_UE][MAX_eNB],double **r_im0_d[MAX_UE][MAX_eNB],double **r_re0_u[MAX_eNB][MAX_UE],double **r_im0_u[MAX_eNB][MAX_UE],channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX],
                  channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX],node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms,
                  int slot);


#endif
