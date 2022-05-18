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

#include "executables/nr-softmodem-common.h"
#include "common/utils/nr/nr_common.h"
#include "common/ran_context.h"
#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "openair1/PHY/MODULATION/nr_modulation.h"
/*#include "RadioResourceConfigCommonSIB.h"
#include "RadioResourceConfigDedicated.h"
#include "TDD-Config.h"
#include "MBSFN-SubframeConfigList.h"*/
#include "openair1/PHY/defs_RU.h"
#include "openair1/PHY/CODING/nrLDPC_extern.h"
#include "assertions.h"
#include <math.h>
#include <complex.h>
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "SCHED_NR/fapi_nr_l1.h"
#include "nfapi_nr_interface.h"

#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"


int l1_north_init_gNB() {

  if (RC.nb_nr_L1_inst > 0 &&  RC.gNB != NULL) {

    AssertFatal(RC.nb_nr_L1_inst>0,"nb_nr_L1_inst=%d\n",RC.nb_nr_L1_inst);
    AssertFatal(RC.gNB!=NULL,"RC.gNB is null\n");
    LOG_I(PHY,"%s() RC.nb_nr_L1_inst:%d\n", __FUNCTION__, RC.nb_nr_L1_inst);

    for (int i=0; i<RC.nb_nr_L1_inst; i++) {
      AssertFatal(RC.gNB[i]!=NULL,"RC.gNB[%d] is null\n",i);

      if ((RC.gNB[i]->if_inst =  NR_IF_Module_init(i))<0) return(-1);
      
      LOG_I(PHY,"%s() RC.gNB[%d] installing callbacks\n", __FUNCTION__, i);
      RC.gNB[i]->if_inst->NR_PHY_config_req = nr_phy_config_request;
      RC.gNB[i]->if_inst->NR_Schedule_response = nr_schedule_response;
    }
  } else {
    LOG_I(PHY,"%s() Not installing PHY callbacks - RC.nb_nr_L1_inst:%d RC.gNB:%p\n", __FUNCTION__, RC.nb_nr_L1_inst, RC.gNB);
  }

  return(0);
}

int init_codebook_gNB(PHY_VARS_gNB *gNB) {

  if(gNB->frame_parms.nb_antennas_tx>1){
    int CSI_RS_antenna_ports = gNB->frame_parms.nb_antennas_tx;
    //NR Codebook Generation for codebook type1 SinglePanel
    int N1 = gNB->ap_N1;
    int N2 = gNB->ap_N2;
    //Uniform Planner Array: UPA
    //    X X X X ... X
    //    X X X X ... X
    // N2 . . . . ... .
    //    X X X X ... X
    //   |<-----N1---->|
    int x_polarization = gNB->ap_XP;
    //Get the uniform planar array parameters
    // To be confirmed
    int O2 = N2 > 1? 4 : 1; //Vertical beam oversampling (1 or 4)
    int O1 = CSI_RS_antenna_ports > 2 ? 4 : 1; //Horizontal beam oversampling (1 or 4)
    AssertFatal(CSI_RS_antenna_ports == N1*N2*x_polarization,
                "Nb of antenna ports at PHY %d does not correspond to what passed down with fapi %d\n",
                 N1*N2*x_polarization, CSI_RS_antenna_ports);

    // Generation of codebook Type1 with codebookMode 1 (CSI_RS_antenna_ports < 16)
    if (CSI_RS_antenna_ports < 16) {
      //Generate DFT vertical beams
      //ll: index of a vertical beams vector (represented by i1_1 in TS 38.214)
      double complex v[N1*O1][N1];
      for (int ll=0; ll<N1*O1; ll++) { //i1_1
        for (int nn=0; nn<N1; nn++) {
          v[ll][nn] = cexp(I*(2*M_PI*nn*ll)/(N1*O1));
          //printf("v[%d][%d] = %f +j %f\n", ll,nn, creal(v[ll][nn]),cimag(v[ll][nn]));
        }
      }
      //Generate DFT Horizontal beams
      //mm: index of a Horizontal beams vector (represented by i1_2 in TS 38.214)
      double complex u[N2*O2][N2];
      for (int mm=0; mm<N2*O2; mm++) { //i1_2
        for (int nn=0; nn<N2; nn++) {
          u[mm][nn] = cexp(I*(2*M_PI*nn*mm)/(N2*O2));
              //printf("u[%d][%d] = %f +j %f\n", mm,nn, creal(u[mm][nn]),cimag(u[mm][nn]));
        }
      }
      //Generate co-phasing angles
      //i_2: index of a co-phasing vector
      //i1_1, i1_2, and i_2 are reported from UEs
      double complex theta_n[4];
      for (int nn=0; nn<4; nn++) {
        theta_n[nn] = cexp(I*M_PI*nn/2);
        //printf("theta_n[%d] = %f +j %f\n", nn, creal(theta_n[nn]),cimag(theta_n[nn]));
      }
      //Kronecker product v_lm
      double complex v_lm[N1*O1][N2*O2][N2*N1];
      //v_ll_mm_codebook denotes the elements of a precoding matrix W_i1,1_i_1,2
      for(int ll=0; ll<N1*O1; ll++) { //i_1_1
        for (int mm=0; mm<N2*O2; mm++) { //i_1_2
          for (int nn1=0; nn1<N1; nn1++) {
            for (int nn2=0; nn2<N2; nn2++) {
              //printf("indx %d \n",nn1*N2+nn2);
              v_lm[ll][mm][nn1*N2+nn2] = v[ll][nn1]*u[mm][nn2];
              //printf("v_lm[%d][%d][%d] = %f +j %f\n",ll,mm, nn1*N2+nn2, creal(v_lm[ll][mm][nn1*N2+nn2]),cimag(v_lm[ll][mm][nn1*N2+nn2]));
            }
          }
        }
      }

      int max_mimo_layers = (CSI_RS_antenna_ports<NR_MAX_NB_LAYERS) ? CSI_RS_antenna_ports : NR_MAX_NB_LAYERS;

      gNB->nr_mimo_precoding_matrix = (int32_t ***)malloc16(max_mimo_layers* sizeof(int32_t **));
      int32_t ***mat = gNB->nr_mimo_precoding_matrix;
      double complex res_code;

      //Table 5.2.2.2.1-5:
      //Codebook for 1-layer CSI reporting using antenna ports 3000 to 2999+PCSI-RS
      gNB->pmiq_size[0] = N1*O1*N2*O2*4+1;
      mat[0] = (int32_t **)malloc16(gNB->pmiq_size[0]*sizeof(int32_t *));

      //pmi=0 corresponds to unit matrix
      mat[0][0] = (int32_t *)calloc(2*N1*N2,sizeof(int32_t));
      for(int j_col=0; j_col<1; j_col++) { //1 layer
        for (int i_rows=0; i_rows<2*N1*N2; i_rows++) { //2-x polarized antenna
          if(j_col==i_rows) {
            mat[0][0][i_rows+j_col] = 0x7fff;
          }
        }
      }

      for(int ll=0; ll<N1*O1; ll++) { //i_1_1
        for (int mm=0; mm<N2*O2; mm++) { //i_1_2
          for (int nn=0; nn<4; nn++) {
            int pmiq = 1+ll*N2*O2*4+mm*4+nn;
            mat[0][pmiq] = (int32_t *)malloc16((2*N1*N2)*1*sizeof(int32_t));
            LOG_D(PHY, "layer 1 Codebook pmiq = %d\n",pmiq);
            for (int len=0; len<N1*N2; len++) {
              res_code=sqrt(1/(double)CSI_RS_antenna_ports)*v_lm[ll][mm][len];
              if (creal(res_code)>0)
                ((short*) &mat[0][pmiq][len])[0] = (short) ((creal(res_code)*32768)+0.5);//convert to Q15
              else
                ((short*) &mat[0][pmiq][len])[0] = (short) ((creal(res_code)*32768)-0.5);//convert to Q15
              if (cimag(res_code)>0)
                ((short*) &mat[0][pmiq][len])[1] = (short) ((cimag(res_code)*32768)+0.5);//convert to Q15
              else
                ((short*) &mat[0][pmiq][len])[1] = (short) ((cimag(res_code)*32768)-0.5);//convert to Q15
              LOG_D(PHY, "1 Layer Precoding Matrix[0][pmi %d][antPort %d]= %f+j %f -> Fixed Point %d+j %d \n",
                    pmiq, len, creal(res_code), cimag(res_code),((short*) &mat[0][pmiq][len])[0],((short*) &mat[0][pmiq][len])[1]);
            }

            for(int len=N1*N2; len<2*N1*N2; len++) {
              res_code=sqrt(1/(double)CSI_RS_antenna_ports)*theta_n[nn]*v_lm[ll][mm][len-N1*N2];
              if (creal(res_code)>0)
                ((short*) &mat[0][pmiq][len])[0] = (short) ((creal(res_code)*32768)+0.5);//convert to Q15
              else
                ((short*) &mat[0][pmiq][len])[0] = (short) ((creal(res_code)*32768)-0.5);//convert to Q15
              if (cimag(res_code)>0)
                ((short*) &mat[0][pmiq][len])[1] = (short) ((cimag(res_code)*32768)+0.5);//convert to Q15
              else
                ((short*) &mat[0][pmiq][len])[1] = (short) ((cimag(res_code)*32768)-0.5);//convert to Q15
              LOG_D(PHY, "1 Layer Precoding Matrix[0][pmi %d][antPort %d]= %f+j %f -> Fixed Point %d+j %d \n",
                    pmiq, len, creal(res_code), cimag(res_code),((short*) &mat[0][pmiq][len])[0],((short*) &mat[0][pmiq][len])[1]);
            }
          }
        }
      }

      int llc;
      int mmc;
      double complex phase_sign;
      //Table 5.2.2.2.1-6:
      //Codebook for 2-layer CSI reporting using antenna ports 3000 to 2999+PCSI-RS
      //Compute the code book size for generating 2 layers out of Tx antenna ports

      //pmi_size is computed as follows
      gNB->pmiq_size[1] = 1;//1 for unity matrix
      for(int llb=0; llb<N1*O1; llb++) { //i_1_1
        for (int mmb=0; mmb<N2*O2; mmb++) { //i_1_2
          for(int ll=0; ll<N1*O1; ll++) { //i_1_1
            for (int mm=0; mm<N2*O2; mm++) { //i_1_2
              for (int nn=0; nn<2; nn++) {
                if((llb != ll) || (mmb != mm) || ((N1 == 1) && (N2 == 1))) gNB->pmiq_size[1] += 1;
              }
            }
          }
        }
      }
      mat[1] = (int32_t **)malloc16(gNB->pmiq_size[1]*sizeof(int32_t *));

      //pmi=0 corresponds to unit matrix
      mat[1][0] = (int32_t *)calloc((2*N1*N2)*(2),sizeof(int32_t));
      for(int j_col=0; j_col<2; j_col++) { //2 layers
        for (int i_rows=0; i_rows<2*N1*N2; i_rows++) { //2-x polarized antenna
          if(j_col==i_rows) {
            mat[1][0][i_rows*2+j_col] = 0x7fff;
          }
        }
      }

      //pmi=1,...,pmi_size, we construct
      int pmiq = 0;
      for(int llb=0; llb<N1*O1; llb++) { //i_1_1
        for (int mmb=0; mmb<N2*O2; mmb++) { //i_1_2
          for(int ll=0; ll<N1*O1; ll++) { //i_1_1
            for (int mm=0; mm<N2*O2; mm++) { //i_1_2
              for (int nn=0; nn<2; nn++) {
                if((llb != ll) || (mmb != mm) || ((N1 == 1) && (N2 == 1))){
                  pmiq += 1;
                  mat[1][pmiq] = (int32_t *)malloc16((2*N1*N2)*(2)*sizeof(int32_t));
                  LOG_D(PHY, "layer 2 Codebook pmiq = %d\n",pmiq);
                  for(int j_col=0; j_col<2; j_col++) {
                    if (j_col==0) {
                      llc = llb;
                      mmc = mmb;
                      phase_sign = 1;
                    }
                    if (j_col==1) {
                      llc = ll;
                      mmc = mm;
                      phase_sign = -1;
                    }
                    for (int i_rows=0; i_rows<N1*N2; i_rows++) {
                      res_code=sqrt(1/(double)(2*CSI_RS_antenna_ports))*v_lm[llc][mmc][i_rows];
                      if (creal(res_code)>0)
                        ((short*) &mat[1][pmiq][i_rows*2+j_col])[0] = (short) ((creal(res_code)*32768)+0.5);//convert to Q15
                      else
                        ((short*) &mat[1][pmiq][i_rows*2+j_col])[0] = (short) ((creal(res_code)*32768)-0.5);//convert to Q15
                      if (cimag(res_code)>0)
                        ((short*) &mat[1][pmiq][i_rows*2+j_col])[1] = (short) ((cimag(res_code)*32768)+0.5);//convert to Q15
                      else
                        ((short*) &mat[1][pmiq][i_rows*2+j_col])[1] = (short) ((cimag(res_code)*32768)-0.5);//convert to Q15
                      LOG_D(PHY, "2 Layer Precoding Matrix[1][pmi %d][antPort %d][layerIdx %d]= %f+j %f -> Fixed Point %d+j %d \n",
                            pmiq,i_rows,j_col, creal(res_code), cimag(res_code),((short*) &mat[1][pmiq][i_rows*2+j_col])[0],((short*) &mat[1][pmiq][i_rows*2+j_col])[1]);
                    }
                    for (int i_rows=N1*N2; i_rows<2*N1*N2; i_rows++) {
                      res_code=sqrt(1/(double)(2*CSI_RS_antenna_ports))*(phase_sign)*theta_n[nn]*v_lm[llc][mmc][i_rows-N1*N2];
                      if (creal(res_code)>0)
                        ((short*) &mat[1][pmiq][i_rows*2+j_col])[0] = (short) ((creal(res_code)*32768)+0.5);//convert to Q15
                      else
                        ((short*) &mat[1][pmiq][i_rows*2+j_col])[0] = (short) ((creal(res_code)*32768)-0.5);//convert to Q15
                      if (cimag(res_code)>0)
                        ((short*) &mat[1][pmiq][i_rows*2+j_col])[1] = (short) ((cimag(res_code)*32768)+0.5);//convert to Q15
                      else
                        ((short*) &mat[1][pmiq][i_rows*2+j_col])[1] = (short) ((cimag(res_code)*32768)-0.5);//convert to Q15
                      LOG_D(PHY, "2 Layer Precoding Matrix[1][pmi %d][antPort %d][layerIdx %d]= %f+j %f -> Fixed Point %d+j %d \n",
                            pmiq,i_rows,j_col, creal(res_code), cimag(res_code),((short*) &mat[1][pmiq][i_rows*2+j_col])[0],((short*) &mat[1][pmiq][i_rows*2+j_col])[1]);
                    }
                  }
                }
              }
            }
          }
        }
      }

      //Table 5.2.2.2.1-7:
      //Codebook for 3-layer CSI reporting using antenna ports 3000 to 2999+PCSI-RS
      if(max_mimo_layers>=3) {

        //pmi_size is computed as follows
        gNB->pmiq_size[2] = 1;//unity matrix
        for(int llb=0; llb<N1*O1; llb++) { //i_1_1
          for (int mmb=0; mmb<N2*O2; mmb++) { //i_1_2
            for(int ll=0; ll<N1*O1; ll++) { //i_1_1
              for (int mm=0; mm<N2*O2; mm++) { //i_1_2
                for (int nn=0; nn<2; nn++) {
                  if((llb != ll) || (mmb != mm)) gNB->pmiq_size[2] += 1;
                }
              }
            }
          }
        }
        mat[2] = (int32_t **)malloc16(gNB->pmiq_size[2]*sizeof(int32_t *));
        //pmi=0 corresponds to unit matrix
        mat[2][0] = (int32_t *)calloc((2*N1*N2)*(3),sizeof(int32_t));
        for(int j_col=0; j_col<3; j_col++) { //3 layers
          for (int i_rows=0; i_rows<2*N1*N2; i_rows++) { //2-x polarized antenna
            if(j_col==i_rows) {
              mat[2][0][i_rows*3+j_col] = 0x7fff;
            }
          }
        }

        pmiq = 0;
        //pmi=1,...,pmi_size are computed as follows
        for(int llb=0; llb<N1*O1; llb++) { //i_1_1
          for (int mmb=0; mmb<N2*O2; mmb++) { //i_1_2
            for(int ll=0; ll<N1*O1; ll++) { //i_1_1
              for (int mm=0; mm<N2*O2; mm++) { //i_1_2
                for (int nn=0; nn<2; nn++) {
                  if((llb != ll) || (mmb != mm)){
                    pmiq += 1;
                    mat[2][pmiq] = (int32_t *)malloc16((2*N1*N2)*(3)*sizeof(int32_t));
                    LOG_D(PHY, "layer 3 Codebook pmiq = %d\n",pmiq);
                    for(int j_col=0; j_col<3; j_col++) {
                      if (j_col==0) {
                        llc = llb;
                        mmc = mmb;
                        phase_sign = 1;
                      }
                      if (j_col==1) {
                        llc = ll;
                        mmc = mm;
                        phase_sign = 1;
                      }
                      if (j_col==2) {
                        llc = ll;
                        mmc = mm;
                        phase_sign = -1;
                      }
                      for (int i_rows=0; i_rows<N1*N2; i_rows++) {
                        res_code=sqrt(1/(double)(3*CSI_RS_antenna_ports))*v_lm[llc][mmc][i_rows];
                        if (creal(res_code)>0)
                          ((short*) &mat[2][pmiq][i_rows*3+j_col])[0] = (short) ((creal(res_code)*32768)+0.5);//convert to Q15
                        else
                          ((short*) &mat[2][pmiq][i_rows*3+j_col])[0] = (short) ((creal(res_code)*32768)-0.5);//convert to Q15
                        if (cimag(res_code)>0)
                          ((short*) &mat[2][pmiq][i_rows*3+j_col])[1] = (short) ((cimag(res_code)*32768)+0.5);//convert to Q15
                        else
                          ((short*) &mat[2][pmiq][i_rows*3+j_col])[1] = (short) ((cimag(res_code)*32768)-0.5);//convert to Q15
                        LOG_D(PHY, "3 Layer Precoding Matrix[2][pmi %d][antPort %d][layerIdx %d]= %f+j %f -> Fixed Point %d+j %d \n",
                              pmiq,i_rows,j_col, creal(res_code), cimag(res_code),((short*) &mat[2][pmiq][i_rows*3+j_col])[0],((short*) &mat[2][pmiq][i_rows*3+j_col])[1]);
                      }
                      for (int i_rows=N1*N2; i_rows<2*N1*N2; i_rows++) {
                        res_code=sqrt(1/(double)(3*CSI_RS_antenna_ports))*(phase_sign)*theta_n[nn]*v_lm[llc][mmc][i_rows-N1*N2];
                        if (creal(res_code)>0)
                          ((short*) &mat[2][pmiq][i_rows*3+j_col])[0] = (short) ((creal(res_code)*32768)+0.5);//convert to Q15
                        else
                          ((short*) &mat[2][pmiq][i_rows*3+j_col])[0] = (short) ((creal(res_code)*32768)-0.5);//convert to Q15
                        if (cimag(res_code)>0)
                          ((short*) &mat[2][pmiq][i_rows*3+j_col])[1] = (short) ((cimag(res_code)*32768)+0.5);//convert to Q15
                        else
                          ((short*) &mat[2][pmiq][i_rows*3+j_col])[1] = (short) ((cimag(res_code)*32768)-0.5);//convert to Q15
                        LOG_D(PHY, "3 Layer Precoding Matrix[2][pmi %d][antPort %d][layerIdx %d]= %f+j %f -> Fixed Point %d+j %d \n",
                              pmiq,i_rows,j_col, creal(res_code), cimag(res_code),((short*) &mat[2][pmiq][i_rows*3+j_col])[0],((short*) &mat[2][pmiq][i_rows*3+j_col])[1]);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }

      //Table 5.2.2.2.1-8:
      //Codebook for 4-layer CSI reporting using antenna ports 3000 to 2999+PCSI-RS
      if(max_mimo_layers>=4) {
        //pmi_size is computed as follows
        gNB->pmiq_size[3] = 1;//unity matrix
        for(int llb=0; llb<N1*O1; llb++) { //i_1_1
          for (int mmb=0; mmb<N2*O2; mmb++) { //i_1_2
            for(int ll=0; ll<N1*O1; ll++) { //i_1_1
              for (int mm=0; mm<N2*O2; mm++) { //i_1_2
                for (int nn=0; nn<2; nn++) {
                  if((llb != ll) || (mmb != mm)) gNB->pmiq_size[3] += 1;
                }
              }
            }
          }
        }

        mat[3] = (int32_t **)malloc16(gNB->pmiq_size[3]*sizeof(int32_t *));
        //pmi=0 corresponds to unit matrix
        mat[3][0] = (int32_t *)calloc((2*N1*N2)*(4),sizeof(int32_t));
        for(int j_col=0; j_col<4; j_col++) { //4 layers
          for (int i_rows=0; i_rows<2*N1*N2; i_rows++) { //2-x polarized antenna
            if(j_col==i_rows) {
              mat[3][0][i_rows*4+j_col] = 0x7fff;
            }
          }
        }

        pmiq = 0;
        //pmi=1,...,pmi_size are computed as follows
        for(int llb=0; llb<N1*O1; llb++) { //i_1_1
          for (int mmb=0; mmb<N2*O2; mmb++) { //i_1_2
            for(int ll=0; ll<N1*O1; ll++) { //i_1_1
              for (int mm=0; mm<N2*O2; mm++) { //i_1_2
                for (int nn=0; nn<2; nn++) {

                  if((llb != ll) || (mmb != mm)){
                    pmiq += 1;
                    mat[3][pmiq] = (int32_t *)malloc16((2*N1*N2)*4*sizeof(int32_t));
                    LOG_D(PHY, "layer 4 pmiq = %d\n",pmiq);
                    for(int j_col=0; j_col<4; j_col++) {
                      if (j_col==0) {
                        llc = llb;
                        mmc = mmb;
                        phase_sign = 1;
                      }
                      if (j_col==1) {
                        llc = ll;
                        mmc = mm;
                        phase_sign = 1;
                      }
                      if (j_col==2) {
                        llc = llb;
                        mmc = mmb;
                        phase_sign = -1;
                      }
                      if (j_col==3) {
                        llc = ll;
                        mmc = mm;
                        phase_sign = -1;
                      }
                      for (int i_rows=0; i_rows<N1*N2; i_rows++) {
                        res_code=sqrt(1/(double)(4*CSI_RS_antenna_ports))*v_lm[llc][mmc][i_rows];
                        if (creal(res_code)>0)
                          ((short*) &mat[3][pmiq][i_rows*4+j_col])[0] = (short) ((creal(res_code)*32768)+0.5);//convert to Q15
                        else
                          ((short*) &mat[3][pmiq][i_rows*4+j_col])[0] = (short) ((creal(res_code)*32768)-0.5);//convert to Q15
                        if (cimag(res_code)>0)
                          ((short*) &mat[3][pmiq][i_rows*4+j_col])[1] = (short) ((cimag(res_code)*32768)+0.5);//convert to Q15
                        else
                          ((short*) &mat[3][pmiq][i_rows*4+j_col])[1] = (short) ((cimag(res_code)*32768)-0.5);//convert to Q15
                        LOG_D(PHY, "4 Layer Precoding Matrix[3][pmi %d][antPort %d][layerIdx %d]= %f+j %f -> Fixed Point %d+j %d \n",
                              pmiq,i_rows,j_col, creal(res_code), cimag(res_code),((short*) &mat[3][pmiq][i_rows*4+j_col])[0],((short*) &mat[3][pmiq][i_rows*4+j_col])[1]);
                      }

                      for (int i_rows=N1*N2; i_rows<2*N1*N2; i_rows++) {
                        res_code=sqrt(1/(double)(4*CSI_RS_antenna_ports))*(phase_sign)*theta_n[nn]*v_lm[llc][mmc][i_rows-N1*N2];
                        if (creal(res_code)>0)
                          ((short*) &mat[3][pmiq][i_rows*4+j_col])[0] = (short) ((creal(res_code)*32768)+0.5);//convert to Q15
                        else
                          ((short*) &mat[3][pmiq][i_rows*4+j_col])[0] = (short) ((creal(res_code)*32768)-0.5);//convert to Q15
                        if (cimag(res_code)>0)
                          ((short*) &mat[3][pmiq][i_rows*4+j_col])[1] = (short) ((cimag(res_code)*32768)+0.5);//convert to Q15
                        else
                          ((short*) &mat[3][pmiq][i_rows*4+j_col])[1] = (short) ((cimag(res_code)*32768)-0.5);//convert to Q15
                        LOG_D(PHY, "4 Layer Precoding Matrix[3][pmi %d][antPort %d][layerIdx %d]= %f+j %f -> Fixed Point %d+j %d \n",
                              pmiq,i_rows,j_col, creal(res_code), cimag(res_code),((short*) &mat[3][pmiq][i_rows*4+j_col])[0],((short*) &mat[3][pmiq][i_rows*4+j_col])[1]);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

int phy_init_nr_gNB(PHY_VARS_gNB *gNB,
                    unsigned char is_secondary_gNB,
                    unsigned char lowmem_flag) {
  // shortcuts
  NR_DL_FRAME_PARMS *const fp       = &gNB->frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  NR_gNB_COMMON *const common_vars  = &gNB->common_vars;
  NR_gNB_PRACH *const prach_vars   = &gNB->prach_vars;
  NR_gNB_PUSCH **const pusch_vars   = gNB->pusch_vars;

  int i;
  int Ptx=cfg->carrier_config.num_tx_ant.value;
  int Prx=cfg->carrier_config.num_rx_ant.value;
  int max_ul_mimo_layers = 4;

  AssertFatal(Ptx>0 && Ptx<9,"Ptx %d is not supported\n",Ptx);
  AssertFatal(Prx>0 && Prx<9,"Prx %d is not supported\n",Prx);
  LOG_I(PHY,"[gNB %d] %s() About to wait for gNB to be configured\n", gNB->Mod_id, __FUNCTION__);

  while(gNB->configured == 0) usleep(10000);

  if (lowmem_flag == 1) {
    gNB->number_of_nr_dlsch_max = 2;
    gNB->number_of_nr_ulsch_max = 2;
  }
  else {
    gNB->number_of_nr_dlsch_max = NUMBER_OF_NR_DLSCH_MAX;
    gNB->number_of_nr_ulsch_max = NUMBER_OF_NR_ULSCH_MAX;
  }  

  load_dftslib();

  crcTableInit();
  init_scrambling_luts();
  init_pucch2_luts();
  load_nrLDPClib(NULL);

  init_codebook_gNB(gNB);

  // PBCH DMRS gold sequences generation
  nr_init_pbch_dmrs(gNB);
  //PDCCH DMRS init
  gNB->nr_gold_pdcch_dmrs = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t **));
  uint32_t ***pdcch_dmrs             = gNB->nr_gold_pdcch_dmrs;
  AssertFatal(pdcch_dmrs!=NULL, "NR init: pdcch_dmrs malloc failed\n");

  gNB->bad_pucch = 0;

  // ceil(((NB_RB<<1)*3)/32) // 3 RE *2(QPSK)
  int pdcch_dmrs_init_length =  (((fp->N_RB_DL<<1)*3)>>5)+1;

  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    pdcch_dmrs[slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t *));
    AssertFatal(pdcch_dmrs[slot]!=NULL, "NR init: pdcch_dmrs for slot %d - malloc failed\n", slot);

    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      pdcch_dmrs[slot][symb] = (uint32_t *)malloc16(pdcch_dmrs_init_length*sizeof(uint32_t));
      LOG_D(PHY,"pdcch_dmrs[%d][%d] %p\n",slot,symb,pdcch_dmrs[slot][symb]);
      AssertFatal(pdcch_dmrs[slot][symb]!=NULL, "NR init: pdcch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);
    }
  }

  nr_generate_modulation_table();
  gNB->pdcch_gold_init = cfg->cell_config.phy_cell_id.value;
  nr_init_pdcch_dmrs(gNB, cfg->cell_config.phy_cell_id.value);
  nr_init_pbch_interleaver(gNB->nr_pbch_interleaver);

  //PDSCH DMRS init
  gNB->nr_gold_pdsch_dmrs = (uint32_t ****)malloc16(fp->slots_per_frame*sizeof(uint32_t ***));
  uint32_t ****pdsch_dmrs             = gNB->nr_gold_pdsch_dmrs;

  // ceil(((NB_RB*6(k)*2(QPSK)/32) // 3 RE *2(QPSK)
  int pdsch_dmrs_init_length =  ((fp->N_RB_DL*12)>>5)+1;
  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    pdsch_dmrs[slot] = (uint32_t ***)malloc16(fp->symbols_per_slot*sizeof(uint32_t **));
    AssertFatal(pdsch_dmrs[slot]!=NULL, "NR init: pdsch_dmrs for slot %d - malloc failed\n", slot);

    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      pdsch_dmrs[slot][symb] = (uint32_t **)malloc16(NR_NB_NSCID*sizeof(uint32_t *));
      AssertFatal(pdsch_dmrs[slot][symb]!=NULL, "NR init: pdsch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);

      for (int q=0; q<NR_NB_NSCID; q++) {
        pdsch_dmrs[slot][symb][q] = (uint32_t *)malloc16(pdsch_dmrs_init_length*sizeof(uint32_t));
        AssertFatal(pdsch_dmrs[slot][symb][q]!=NULL, "NR init: pdsch_dmrs for slot %d symbol %d nscid %d - malloc failed\n", slot, symb, q);
      }
    }
  }


  for (int nscid = 0; nscid < NR_NB_NSCID; nscid++) {
    gNB->pdsch_gold_init[nscid] = cfg->cell_config.phy_cell_id.value;
    nr_init_pdsch_dmrs(gNB, nscid, cfg->cell_config.phy_cell_id.value);
  }

  //PUSCH DMRS init
  gNB->nr_gold_pusch_dmrs = (uint32_t ****)malloc16(NR_NB_NSCID*sizeof(uint32_t ***));

  uint32_t ****pusch_dmrs = gNB->nr_gold_pusch_dmrs;

  int pusch_dmrs_init_length =  ((fp->N_RB_UL*12)>>5)+1;
  for(int nscid=0; nscid<NR_NB_NSCID; nscid++) {
    pusch_dmrs[nscid] = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t **));
    AssertFatal(pusch_dmrs[nscid]!=NULL, "NR init: pusch_dmrs for nscid %d - malloc failed\n", nscid);

    for (int slot=0; slot<fp->slots_per_frame; slot++) {
      pusch_dmrs[nscid][slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t *));
      AssertFatal(pusch_dmrs[nscid][slot]!=NULL, "NR init: pusch_dmrs for slot %d - malloc failed\n", slot);

      for (int symb=0; symb<fp->symbols_per_slot; symb++) {
        pusch_dmrs[nscid][slot][symb] = (uint32_t *)malloc16(pusch_dmrs_init_length*sizeof(uint32_t));
        AssertFatal(pusch_dmrs[nscid][slot][symb]!=NULL, "NR init: pusch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);
      }
    }
  }

  for (int nscid=0; nscid<NR_NB_NSCID; nscid++) {
    gNB->pusch_gold_init[nscid] = cfg->cell_config.phy_cell_id.value;
    nr_gold_pusch(gNB, nscid, gNB->pusch_gold_init[nscid]);
  }

  //CSI RS init
  gNB->nr_gold_csi_rs = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t **));
  uint32_t ***csi_rs = gNB->nr_gold_csi_rs;
  AssertFatal(csi_rs!=NULL, "NR init: csi reference signal malloc failed\n");

  // ceil((NB_RB*8(max allocation per RB)*2(QPSK))/32)
  int csi_dmrs_init_length =  ((fp->N_RB_DL<<4)>>5)+1;

  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    csi_rs[slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t *));
    AssertFatal(csi_rs[slot]!=NULL, "NR init: csi reference signal for slot %d - malloc failed\n", slot);

    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      csi_rs[slot][symb] = (uint32_t *)malloc16(csi_dmrs_init_length*sizeof(uint32_t));
      AssertFatal(csi_rs[slot][symb]!=NULL, "NR init: csi reference signal for slot %d symbol %d - malloc failed\n", slot, symb);
    }
  }

  gNB->csi_gold_init = cfg->cell_config.phy_cell_id.value;
  nr_init_csi_rs(gNB, cfg->cell_config.phy_cell_id.value);

  for (int id=0; id<NUMBER_OF_NR_SRS_MAX; id++) {
    gNB->nr_srs_info[id] = (nr_srs_info_t *)malloc16_clear(sizeof(nr_srs_info_t));
    gNB->nr_srs_info[id]->sc_list = (uint16_t *) malloc16_clear(6*fp->N_RB_UL*sizeof(uint16_t));
    gNB->nr_srs_info[id]->srs_generated_signal = (int32_t*)malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
    gNB->nr_srs_info[id]->noise_power = (uint32_t*)malloc16_clear(sizeof(uint32_t));
    gNB->nr_srs_info[id]->srs_received_signal = (int32_t **)malloc16(Prx*sizeof(int32_t*));
    gNB->nr_srs_info[id]->srs_ls_estimated_channel = (int32_t **)malloc16(Prx*sizeof(int32_t*));
    gNB->nr_srs_info[id]->srs_estimated_channel_freq = (int32_t **)malloc16(Prx*sizeof(int32_t*));
    gNB->nr_srs_info[id]->srs_estimated_channel_time = (int32_t **)malloc16(Prx*sizeof(int32_t*));
    gNB->nr_srs_info[id]->srs_estimated_channel_time_shifted = (int32_t **)malloc16(Prx*sizeof(int32_t*));
    for (i=0;i<Prx;i++){
      gNB->nr_srs_info[id]->srs_received_signal[i] = (int32_t*)malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
      gNB->nr_srs_info[id]->srs_ls_estimated_channel[i] = (int32_t*)malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
      gNB->nr_srs_info[id]->srs_estimated_channel_freq[i] = (int32_t*)malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
      gNB->nr_srs_info[id]->srs_estimated_channel_time[i] = (int32_t*)malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
      gNB->nr_srs_info[id]->srs_estimated_channel_time_shifted[i] = (int32_t*)malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
    }
  }

  generate_ul_reference_signal_sequences(SHRT_MAX);

  /* Generate low PAPR type 1 sequences for PUSCH DMRS, these are used if transform precoding is enabled.  */
  generate_lowpapr_typ1_refsig_sequences(SHRT_MAX);

  /// Transport init necessary for NR synchro
  init_nr_transport(gNB);

  gNB->first_run_I0_measurements = 1;

  common_vars->rxdata  = (int32_t **)malloc16(Prx*sizeof(int32_t*));
  common_vars->txdataF = (int32_t **)malloc16(Ptx*sizeof(int32_t*));
  common_vars->rxdataF = (int32_t **)malloc16(Prx*sizeof(int32_t*));
  common_vars->beam_id = (uint8_t **)malloc16(Ptx*sizeof(uint8_t*));

  for (i=0;i<Ptx;i++){
    common_vars->txdataF[i] = (int32_t*)malloc16_clear(fp->samples_per_frame_wCP*sizeof(int32_t)); // [hna] samples_per_frame without CP
    LOG_D(PHY,"[INIT] common_vars->txdataF[%d] = %p (%lu bytes)\n",
          i,common_vars->txdataF[i],
          fp->samples_per_frame_wCP*sizeof(int32_t));
    common_vars->beam_id[i] = (uint8_t*)malloc16_clear(fp->symbols_per_slot*fp->slots_per_frame*sizeof(uint8_t));
    memset(common_vars->beam_id[i],255,fp->symbols_per_slot*fp->slots_per_frame);
  }
  for (i=0;i<Prx;i++){
    common_vars->rxdataF[i] = (int32_t*)malloc16_clear(fp->samples_per_frame_wCP*sizeof(int32_t));
    common_vars->rxdata[i] = (int32_t*)malloc16_clear(fp->samples_per_frame*sizeof(int32_t));
  }
  common_vars->debugBuff = (int32_t*)malloc16_clear(fp->samples_per_frame*sizeof(int32_t)*100);	
  common_vars->debugBuff_sample_offset = 0; 

  // PRACH
  prach_vars->prachF = (int16_t *)malloc16_clear( 1024*2*sizeof(int16_t) );
  prach_vars->rxsigF = (int16_t **)malloc16_clear(Prx*sizeof(int16_t*));
  prach_vars->prach_ifft       = (int32_t *)malloc16_clear(1024*2*sizeof(int32_t));

  init_prach_list(gNB);

  int N_RB_UL = cfg->carrier_config.ul_grid_size[cfg->ssb_config.scs_common.value].value;
  int n_buf = Prx*max_ul_mimo_layers;

  int nb_re_pusch = N_RB_UL * NR_NB_SC_PER_RB;
#ifdef __AVX2__
  int nb_re_pusch2 = nb_re_pusch + (nb_re_pusch&7);
#else
  int nb_re_pusch2 = nb_re_pusch;
#endif

  for (int ULSCH_id=0; ULSCH_id<gNB->number_of_nr_ulsch_max; ULSCH_id++) {
    pusch_vars[ULSCH_id] = (NR_gNB_PUSCH *)malloc16_clear( sizeof(NR_gNB_PUSCH) );
    pusch_vars[ULSCH_id]->rxdataF_ext           = (int32_t **)malloc16(Prx*sizeof(int32_t *) );
    pusch_vars[ULSCH_id]->ul_ch_estimates       = (int32_t **)malloc16(n_buf*sizeof(int32_t *) );
    pusch_vars[ULSCH_id]->ul_ch_estimates_ext   = (int32_t **)malloc16(n_buf*sizeof(int32_t *) );
    pusch_vars[ULSCH_id]->ptrs_phase_per_slot   = (int32_t **)malloc16(n_buf*sizeof(int32_t *) );
    pusch_vars[ULSCH_id]->ul_ch_estimates_time  = (int32_t **)malloc16(n_buf*sizeof(int32_t *) );
    pusch_vars[ULSCH_id]->rxdataF_comp          = (int32_t **)malloc16(n_buf*sizeof(int32_t *) );
    pusch_vars[ULSCH_id]->ul_ch_mag0            = (int32_t **)malloc16(n_buf*sizeof(int32_t *) );
    pusch_vars[ULSCH_id]->ul_ch_magb0           = (int32_t **)malloc16(n_buf*sizeof(int32_t *) );
    pusch_vars[ULSCH_id]->ul_ch_mag             = (int32_t **)malloc16(n_buf*sizeof(int32_t *) );
    pusch_vars[ULSCH_id]->ul_ch_magb            = (int32_t **)malloc16(n_buf*sizeof(int32_t *) );
    pusch_vars[ULSCH_id]->rho                   = (int32_t ***)malloc16(Prx*sizeof(int32_t **) );
    pusch_vars[ULSCH_id]->llr_layers            = (int16_t **)malloc16(max_ul_mimo_layers*sizeof(int32_t *) );

    for (i=0; i<Prx; i++) {
      pusch_vars[ULSCH_id]->rxdataF_ext[i]           = (int32_t *)malloc16_clear( sizeof(int32_t)*nb_re_pusch2*fp->symbols_per_slot );
      pusch_vars[ULSCH_id]->rho[i]                   = (int32_t **)malloc16_clear(NR_MAX_NB_LAYERS*NR_MAX_NB_LAYERS*sizeof(int32_t*));

      for (int j=0; j< max_ul_mimo_layers; j++) {
        for (int k=0; k<max_ul_mimo_layers; k++) {
          pusch_vars[ULSCH_id]->rho[i][j*max_ul_mimo_layers+k]=(int32_t *)malloc16_clear( sizeof(int32_t)*nb_re_pusch2*fp->symbols_per_slot );
        }
      }
    }
    for (i=0; i<n_buf; i++) {
      pusch_vars[ULSCH_id]->ul_ch_estimates[i]       = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->ofdm_symbol_size*fp->symbols_per_slot );
      pusch_vars[ULSCH_id]->ul_ch_estimates_ext[i]   = (int32_t *)malloc16_clear( sizeof(int32_t)*nb_re_pusch2*fp->symbols_per_slot );
      pusch_vars[ULSCH_id]->ul_ch_estimates_time[i]  = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->ofdm_symbol_size );
      pusch_vars[ULSCH_id]->ptrs_phase_per_slot[i]   = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->symbols_per_slot); // symbols per slot
      pusch_vars[ULSCH_id]->rxdataF_comp[i]          = (int32_t *)malloc16_clear( sizeof(int32_t)*nb_re_pusch2*fp->symbols_per_slot );
      pusch_vars[ULSCH_id]->ul_ch_mag0[i]            = (int32_t *)malloc16_clear( sizeof(int32_t)*nb_re_pusch2*fp->symbols_per_slot );
      pusch_vars[ULSCH_id]->ul_ch_magb0[i]           = (int32_t *)malloc16_clear( sizeof(int32_t)*nb_re_pusch2*fp->symbols_per_slot );
      pusch_vars[ULSCH_id]->ul_ch_mag[i]             = (int32_t *)malloc16_clear( sizeof(int32_t)*nb_re_pusch2*fp->symbols_per_slot );
      pusch_vars[ULSCH_id]->ul_ch_magb[i]            = (int32_t *)malloc16_clear( sizeof(int32_t)*nb_re_pusch2*fp->symbols_per_slot );
    }

    for (i=0; i< max_ul_mimo_layers; i++) {
      pusch_vars[ULSCH_id]->llr_layers[i] = (int16_t *)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) ); // [hna] 6144 is LTE and (8*((3*8*6144)+12)) is not clear
    }
    pusch_vars[ULSCH_id]->llr = (int16_t *)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) ); // [hna] 6144 is LTE and (8*((3*8*6144)+12)) is not clear
    pusch_vars[ULSCH_id]->ul_valid_re_per_slot  = (int16_t *)malloc16_clear( sizeof(int16_t)*fp->symbols_per_slot);
  } //ulsch_id
/*
  for (ulsch_id=0; ulsch_id<NUMBER_OF_UE_MAX; ulsch_id++)
    gNB->UE_stats_ptr[ulsch_id] = &gNB->UE_stats[ulsch_id];
*/
  return (0);
}

void phy_free_nr_gNB(PHY_VARS_gNB *gNB)
{
  NR_DL_FRAME_PARMS* const fp       = &gNB->frame_parms;
  const int Ptx = gNB->gNB_config.carrier_config.num_tx_ant.value;
  const int Prx = gNB->gNB_config.carrier_config.num_rx_ant.value;
  const int max_ul_mimo_layers = 4; // taken from phy_init_nr_gNB()
  const int n_buf = Prx * max_ul_mimo_layers;

  int max_dl_mimo_layers =(fp->nb_antennas_tx<NR_MAX_NB_LAYERS) ? fp->nb_antennas_tx : NR_MAX_NB_LAYERS;
  if (fp->nb_antennas_tx>1) {
    for (int nl = 0; nl < max_dl_mimo_layers; nl++) {
      for(int size = 0; size < gNB->pmiq_size[nl]; size++)
        free_and_zero(gNB->nr_mimo_precoding_matrix[nl][size]);
      free_and_zero(gNB->nr_mimo_precoding_matrix[nl]);
    }
    free_and_zero(gNB->nr_mimo_precoding_matrix);
  }

  uint32_t ***pdcch_dmrs = gNB->nr_gold_pdcch_dmrs;
  for (int slot = 0; slot < fp->slots_per_frame; slot++) {
    for (int symb = 0; symb < fp->symbols_per_slot; symb++)
      free_and_zero(pdcch_dmrs[slot][symb]);
    free_and_zero(pdcch_dmrs[slot]);
  }
  free_and_zero(pdcch_dmrs);

  uint32_t ****pdsch_dmrs = gNB->nr_gold_pdsch_dmrs;
  for (int slot = 0; slot < fp->slots_per_frame; slot++) {
    for (int symb = 0; symb < fp->symbols_per_slot; symb++) {
      for (int q = 0; q < NR_NB_NSCID; q++)
        free_and_zero(pdsch_dmrs[slot][symb][q]);
      free_and_zero(pdsch_dmrs[slot][symb]);
    }
    free_and_zero(pdsch_dmrs[slot]);
  }
  free_and_zero(gNB->nr_gold_pdsch_dmrs);

  uint32_t ****pusch_dmrs = gNB->nr_gold_pusch_dmrs;
  for(int nscid = 0; nscid < 2; nscid++) {
    for (int slot = 0; slot < fp->slots_per_frame; slot++) {
      for (int symb = 0; symb < fp->symbols_per_slot; symb++)
        free_and_zero(pusch_dmrs[nscid][slot][symb]);
      free_and_zero(pusch_dmrs[nscid][slot]);
    }
    free_and_zero(pusch_dmrs[nscid]);
  }
  free_and_zero(pusch_dmrs);

  uint32_t ***csi_rs = gNB->nr_gold_csi_rs;
  for (int slot = 0; slot < fp->slots_per_frame; slot++) {
    for (int symb = 0; symb < fp->symbols_per_slot; symb++)
      free_and_zero(csi_rs[slot][symb]);
    free_and_zero(csi_rs[slot]);
  }
  free_and_zero(csi_rs);

  for (int id = 0; id < NUMBER_OF_NR_SRS_MAX; id++) {
    for (int i = 0; i < Prx; i++) {
      free_and_zero(gNB->nr_srs_info[id]->srs_received_signal[i]);
      free_and_zero(gNB->nr_srs_info[id]->srs_ls_estimated_channel[i]);
      free_and_zero(gNB->nr_srs_info[id]->srs_estimated_channel_freq[i]);
      free_and_zero(gNB->nr_srs_info[id]->srs_estimated_channel_time[i]);
      free_and_zero(gNB->nr_srs_info[id]->srs_estimated_channel_time_shifted[i]);
    }
    free_and_zero(gNB->nr_srs_info[id]->sc_list);
    free_and_zero(gNB->nr_srs_info[id]->srs_generated_signal);
    free_and_zero(gNB->nr_srs_info[id]->noise_power);
    free_and_zero(gNB->nr_srs_info[id]->srs_received_signal);
    free_and_zero(gNB->nr_srs_info[id]->srs_ls_estimated_channel);
    free_and_zero(gNB->nr_srs_info[id]->srs_estimated_channel_freq);
    free_and_zero(gNB->nr_srs_info[id]->srs_estimated_channel_time);
    free_and_zero(gNB->nr_srs_info[id]->srs_estimated_channel_time_shifted);
    free_and_zero(gNB->nr_srs_info[id]);
  }

  free_ul_reference_signal_sequences();
  free_gnb_lowpapr_sequences();

  reset_nr_transport(gNB);

  NR_gNB_COMMON * common_vars = &gNB->common_vars;
  for (int i = 0; i < Ptx; i++) {
    free_and_zero(common_vars->txdataF[i]);
    free_and_zero(common_vars->beam_id[i]);
  }

  for (int i = 0; i < Prx; ++i) {
    free_and_zero(common_vars->rxdataF[i]);
    free_and_zero(common_vars->rxdata[i]);
  }

  free_and_zero(common_vars->txdataF);
  free_and_zero(common_vars->rxdata);
  free_and_zero(common_vars->rxdataF);
  free_and_zero(common_vars->beam_id);

  free_and_zero(common_vars->debugBuff);

  NR_gNB_PRACH* prach_vars = &gNB->prach_vars;
  free_and_zero(prach_vars->prachF);
  free_and_zero(prach_vars->rxsigF);
  free_and_zero(prach_vars->prach_ifft);

  NR_gNB_PUSCH** pusch_vars = gNB->pusch_vars;
  for (int ULSCH_id=0; ULSCH_id<gNB->number_of_nr_ulsch_max; ULSCH_id++) {
    for (int i=0; i< max_ul_mimo_layers; i++)
      free_and_zero(pusch_vars[ULSCH_id]->llr_layers[i]);
    for (int i = 0; i < Prx; i++) {
      free_and_zero(pusch_vars[ULSCH_id]->rxdataF_ext[i]);
      for (int j=0; j< max_ul_mimo_layers; j++) {
        for (int k=0; k<max_ul_mimo_layers; k++)
          free_and_zero(pusch_vars[ULSCH_id]->rho[i][j*max_ul_mimo_layers+k]);
      }
      free_and_zero(pusch_vars[ULSCH_id]->rho[i]);
    }
    for (int i = 0; i < n_buf; i++) {
      free_and_zero(pusch_vars[ULSCH_id]->ul_ch_estimates[i]);
      free_and_zero(pusch_vars[ULSCH_id]->ul_ch_estimates_ext[i]);
      free_and_zero(pusch_vars[ULSCH_id]->ul_ch_estimates_time[i]);
      free_and_zero(pusch_vars[ULSCH_id]->ptrs_phase_per_slot[i]);
      free_and_zero(pusch_vars[ULSCH_id]->rxdataF_comp[i]);
      free_and_zero(pusch_vars[ULSCH_id]->ul_ch_mag0[i]);
      free_and_zero(pusch_vars[ULSCH_id]->ul_ch_magb0[i]);
      free_and_zero(pusch_vars[ULSCH_id]->ul_ch_mag[i]);
      free_and_zero(pusch_vars[ULSCH_id]->ul_ch_magb[i]);
    }
    free_and_zero(pusch_vars[ULSCH_id]->llr_layers);
    free_and_zero(pusch_vars[ULSCH_id]->rxdataF_ext);
    free_and_zero(pusch_vars[ULSCH_id]->ul_ch_estimates);
    free_and_zero(pusch_vars[ULSCH_id]->ul_ch_estimates_ext);
    free_and_zero(pusch_vars[ULSCH_id]->ptrs_phase_per_slot);
    free_and_zero(pusch_vars[ULSCH_id]->ul_ch_estimates_time);
    free_and_zero(pusch_vars[ULSCH_id]->ul_valid_re_per_slot);
    free_and_zero(pusch_vars[ULSCH_id]->rxdataF_comp);
    free_and_zero(pusch_vars[ULSCH_id]->ul_ch_mag0);
    free_and_zero(pusch_vars[ULSCH_id]->ul_ch_magb0);
    free_and_zero(pusch_vars[ULSCH_id]->ul_ch_mag);
    free_and_zero(pusch_vars[ULSCH_id]->ul_ch_magb);
    free_and_zero(pusch_vars[ULSCH_id]->rho);

    free_and_zero(pusch_vars[ULSCH_id]->llr);
    free_and_zero(pusch_vars[ULSCH_id]);
  } //ULSCH_id
}

//Adding nr_schedule_handler
void install_nr_schedule_handlers(NR_IF_Module_t *if_inst)
{
  if_inst->NR_PHY_config_req = nr_phy_config_request;
  if_inst->NR_Schedule_response = nr_schedule_response;
}
/*
void install_schedule_handlers(IF_Module_t *if_inst)
{
  if_inst->PHY_config_req = phy_config_request;
  if_inst->schedule_response = schedule_response;
}*/

/// this function is a temporary addition for NR configuration


void nr_phy_config_request_sim(PHY_VARS_gNB *gNB,
                               int N_RB_DL,
                               int N_RB_UL,
                               int mu,
                               int Nid_cell,
                               uint64_t position_in_burst)
{
  NR_DL_FRAME_PARMS *fp                                   = &gNB->frame_parms;
  nfapi_nr_config_request_scf_t *gNB_config               = &gNB->gNB_config;
  //overwrite for new NR parameters

  uint64_t rev_burst=0;
  for (int i=0; i<64; i++)
    rev_burst |= (((position_in_burst>>(63-i))&0x01)<<i);

  gNB_config->cell_config.phy_cell_id.value             = Nid_cell;
  gNB_config->ssb_config.scs_common.value               = mu;
  gNB_config->ssb_table.ssb_subcarrier_offset.value     = 0;
  gNB_config->ssb_table.ssb_offset_point_a.value        = (N_RB_DL-20)>>1;
  gNB_config->ssb_table.ssb_mask_list[1].ssb_mask.value = (rev_burst)&(0xFFFFFFFF);
  gNB_config->ssb_table.ssb_mask_list[0].ssb_mask.value = (rev_burst>>32)&(0xFFFFFFFF);
  gNB_config->cell_config.frame_duplex_type.value       = TDD;
  gNB_config->ssb_table.ssb_period.value		= 1; //10ms
  gNB_config->carrier_config.dl_grid_size[mu].value     = N_RB_DL;
  gNB_config->carrier_config.ul_grid_size[mu].value     = N_RB_UL;
  gNB_config->carrier_config.num_tx_ant.value           = fp->nb_antennas_tx;
  gNB_config->carrier_config.num_rx_ant.value           = fp->nb_antennas_rx;

  gNB_config->tdd_table.tdd_period.value = 0;
  //gNB_config->subframe_config.dl_cyclic_prefix_type.value = (fp->Ncp == NORMAL) ? NFAPI_CP_NORMAL : NFAPI_CP_EXTENDED;

  gNB->mac_enabled   = 1;
  if (mu==0) {
    fp->dl_CarrierFreq = 2600000000;//from_nrarfcn(gNB_config->nfapi_config.rf_bands.rf_band[0],gNB_config->nfapi_config.nrarfcn.value);
    fp->ul_CarrierFreq = 2600000000;//fp->dl_CarrierFreq - (get_uldl_offset(gNB_config->nfapi_config.rf_bands.rf_band[0])*100000);
    fp->nr_band = 38;
    //  fp->threequarter_fs= 0;
  } else if (mu==1) {
    fp->dl_CarrierFreq = 3600000000;//from_nrarfcn(gNB_config->nfapi_config.rf_bands.rf_band[0],gNB_config->nfapi_config.nrarfcn.value);
    fp->ul_CarrierFreq = 3600000000;//fp->dl_CarrierFreq - (get_uldl_offset(gNB_config->nfapi_config.rf_bands.rf_band[0])*100000);
    fp->nr_band = 78;
    //  fp->threequarter_fs= 0;
  } else if (mu==3) {
    fp->dl_CarrierFreq = 27524520000;//from_nrarfcn(gNB_config->nfapi_config.rf_bands.rf_band[0],gNB_config->nfapi_config.nrarfcn.value);
    fp->ul_CarrierFreq = 27524520000;//fp->dl_CarrierFreq - (get_uldl_offset(gNB_config->nfapi_config.rf_bands.rf_band[0])*100000);
    fp->nr_band = 261;
    //  fp->threequarter_fs= 0;
  }

  fp->threequarter_fs = 0;
  gNB_config->carrier_config.dl_bandwidth.value = config_bandwidth(mu, N_RB_DL, fp->nr_band);

  nr_init_frame_parms(gNB_config, fp);
  fp->ofdm_offset_divisor = UINT_MAX;
  gNB->configured    = 1;
  LOG_I(PHY,"gNB configured\n");
}


void nr_phy_config_request(NR_PHY_Config_t *phy_config) {
  uint8_t Mod_id = phy_config->Mod_id;
  uint8_t short_sequence, num_sequences, rootSequenceIndex, fd_occasion;
  NR_DL_FRAME_PARMS *fp = &RC.gNB[Mod_id]->frame_parms;
  nfapi_nr_config_request_scf_t *gNB_config = &RC.gNB[Mod_id]->gNB_config;

  memcpy((void*)gNB_config,phy_config->cfg,sizeof(*phy_config->cfg));
  RC.gNB[Mod_id]->mac_enabled     = 1;

  uint64_t dl_bw_khz = (12*gNB_config->carrier_config.dl_grid_size[gNB_config->ssb_config.scs_common.value].value)*(15<<gNB_config->ssb_config.scs_common.value);
  fp->dl_CarrierFreq = ((dl_bw_khz>>1) + gNB_config->carrier_config.dl_frequency.value)*1000 ;
  
  uint64_t ul_bw_khz = (12*gNB_config->carrier_config.ul_grid_size[gNB_config->ssb_config.scs_common.value].value)*(15<<gNB_config->ssb_config.scs_common.value);
  fp->ul_CarrierFreq = ((ul_bw_khz>>1) + gNB_config->carrier_config.uplink_frequency.value)*1000 ;

  int32_t dlul_offset = fp->ul_CarrierFreq - fp->dl_CarrierFreq;
  fp->nr_band = get_band(fp->dl_CarrierFreq, dlul_offset);

  LOG_I(PHY, "DL frequency %lu Hz, UL frequency %lu Hz: band %d, uldl offset %d Hz\n", fp->dl_CarrierFreq, fp->ul_CarrierFreq, fp->nr_band, dlul_offset);

  fp->threequarter_fs = openair0_cfg[0].threequarter_fs;
  LOG_A(PHY,"Configuring MIB for instance %d, : (Nid_cell %d,DL freq %llu, UL freq %llu)\n",
        Mod_id,
        gNB_config->cell_config.phy_cell_id.value,
        (unsigned long long)fp->dl_CarrierFreq,
        (unsigned long long)fp->ul_CarrierFreq);

  nr_init_frame_parms(gNB_config, fp);
  

  if (RC.gNB[Mod_id]->configured == 1) {
    LOG_E(PHY,"Already gNB already configured, do nothing\n");
    return;
  }

  fd_occasion = 0;
  nfapi_nr_prach_config_t *prach_config = &gNB_config->prach_config;
  short_sequence = prach_config->prach_sequence_length.value;
//  for(fd_occasion = 0; fd_occasion <= prach_config->num_prach_fd_occasions.value ; fd_occasion) { // TODO Need to handle for msg1-fdm > 1
  num_sequences = prach_config->num_prach_fd_occasions_list[fd_occasion].num_root_sequences.value;
  rootSequenceIndex = prach_config->num_prach_fd_occasions_list[fd_occasion].prach_root_sequence_index.value;

  compute_nr_prach_seq(short_sequence, num_sequences, rootSequenceIndex, RC.gNB[Mod_id]->X_u);
//  }
  RC.gNB[Mod_id]->configured     = 1;

  fp->ofdm_offset_divisor = RC.gNB[Mod_id]->ofdm_offset_divisor;
  init_symbol_rotation(fp);
  init_timeshift_rotation(fp);

  LOG_I(PHY,"gNB %d configured\n",Mod_id);
}

void init_DLSCH_struct(PHY_VARS_gNB *gNB, processingData_L1tx_t *msg) {
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  uint16_t grid_size = cfg->carrier_config.dl_grid_size[fp->numerology_index].value;
  msg->num_pdsch_slot = 0;

  int num_cw = NR_MAX_NB_LAYERS > 4? 2:1;
  for (int i=0; i<gNB->number_of_nr_dlsch_max; i++) {
    LOG_I(PHY,"Allocating Transport Channel Buffers for DLSCH %d/%d\n",i,gNB->number_of_nr_dlsch_max);
    for (int j=0; j<num_cw; j++) {
      msg->dlsch[i][j] = new_gNB_dlsch(fp,1,16,NSOFT,0,grid_size);
      AssertFatal(msg->dlsch[i][j]!=NULL,"Can't initialize dlsch %d \n", i);
    }
  }
}

void reset_DLSCH_struct(const PHY_VARS_gNB *gNB, processingData_L1tx_t *msg)
{
  const NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  const nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  const uint16_t grid_size = cfg->carrier_config.dl_grid_size[fp->numerology_index].value;
  int num_cw = NR_MAX_NB_LAYERS > 4? 2:1;
  for (int i=0; i<gNB->number_of_nr_dlsch_max; i++)
    for (int j=0; j<num_cw; j++)
      free_gNB_dlsch(&msg->dlsch[i][j], grid_size, fp);
}

void init_nr_transport(PHY_VARS_gNB *gNB) {
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  LOG_I(PHY, "Initialise nr transport\n");

  memset(gNB->num_pdsch_rnti, 0, sizeof(uint16_t)*80);

  for (int i=0; i<NUMBER_OF_NR_PUCCH_MAX; i++) {
    LOG_I(PHY,"Allocating Transport Channel Buffers for PUCCH %d/%d\n",i,NUMBER_OF_NR_PUCCH_MAX);
    gNB->pucch[i] = new_gNB_pucch();
    AssertFatal(gNB->pucch[i]!=NULL,"Can't initialize pucch %d \n", i);
  }

  for (int i=0; i<NUMBER_OF_NR_SRS_MAX; i++) {
    LOG_I(PHY,"Allocating Transport Channel Buffers for SRS %d/%d\n",i,NUMBER_OF_NR_SRS_MAX);
    gNB->srs[i] = new_gNB_srs();
    AssertFatal(gNB->srs[i]!=NULL,"Can't initialize srs %d \n", i);
  }

  for (int i=0; i<gNB->number_of_nr_ulsch_max; i++) {

    LOG_I(PHY,"Allocating Transport Channel Buffer for ULSCH  %d/%d\n",i,gNB->number_of_nr_ulsch_max);

    gNB->ulsch[i] = new_gNB_ulsch(MAX_LDPC_ITERATIONS, fp->N_RB_UL);

    if (!gNB->ulsch[i]) {
      LOG_E(PHY,"Can't get gNB ulsch structures\n");
      exit(-1);
    }
  }

  gNB->rx_total_gain_dB=130;

  //fp->pucch_config_common.deltaPUCCH_Shift = 1;
}

void reset_nr_transport(PHY_VARS_gNB *gNB)
{
  const NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;

  for (int i = 0; i < NUMBER_OF_NR_PUCCH_MAX; i++)
    free_gNB_pucch(gNB->pucch[i]);

  for (int i = 0; i < NUMBER_OF_NR_SRS_MAX; i++)
    free_gNB_srs(gNB->srs[i]);

  for (int i=0; i<gNB->number_of_nr_ulsch_max; i++)
    free_gNB_ulsch(&gNB->ulsch[i], fp->N_RB_UL);
}
