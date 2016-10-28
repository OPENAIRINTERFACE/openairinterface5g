/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file PHY/LTE_TRANSPORT/pucch.c
* \brief Top-level routines for generating and decoding the PUCCH physical channel V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs.h"
#include "PHY/extern.h" 
#include "LAYER2/MAC/extern.h"

#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

#include "T.h"

//uint8_t ncs_cell[20][7];
//#define DEBUG_PUCCH_TX
//#define DEBUG_PUCCH_RX

int16_t cfo_pucch_np[24*7] = {20787,-25330,27244,-18205,31356,-9512,32767,0,31356,9511,27244,18204,20787,25329,
                              27244,-18205,30272,-12540,32137,-6393,32767,0,32137,6392,30272,12539,27244,18204,
                              31356,-9512,32137,-6393,32609,-3212,32767,0,32609,3211,32137,6392,31356,9511,
                              32767,0,32767,0,32767,0,32767,0,32767,0,32767,0,32767,0,
                              31356,9511,32137,6392,32609,3211,32767,0,32609,-3212,32137,-6393,31356,-9512,
                              27244,18204,30272,12539,32137,6392,32767,0,32137,-6393,30272,-12540,27244,-18205,
                              20787,25329,27244,18204,31356,9511,32767,0,31356,-9512,27244,-18205,20787,-25330
                             };

int16_t cfo_pucch_ep[24*6] = {24278,-22005,29621,-14010,32412,-4808,32412,4807,29621,14009,24278,22004,
                              28897,-15447,31356,-9512,32609,-3212,32609,3211,31356,9511,28897,15446,
                              31785,-7962,32412,-4808,32727,-1608,32727,1607,32412,4807,31785,7961,
                              32767,0,32767,0,32767,0,32767,0,32767,0,32767,0,
                              31785,7961,32412,4807,32727,1607,32727,-1608,32412,-4808,31785,-7962,
                              28897,15446,31356,9511,32609,3211,32609,-3212,31356,-9512,28897,-15447,
                              24278,22004,29621,14009,32412,4807,32412,-4808,29621,-14010,24278,-22005
                             };


void init_ncs_cell(LTE_DL_FRAME_PARMS *frame_parms,uint8_t ncs_cell[20][7])
{

  uint8_t ns,l,reset=1,i,N_UL_symb;
  uint32_t x1,x2,j=0,s=0;

  N_UL_symb = (frame_parms->Ncp==0) ? 7 : 6;
  x2 = frame_parms->Nid_cell;

  for (ns=0; ns<20; ns++) {

    for (l=0; l<N_UL_symb; l++) {
      ncs_cell[ns][l]=0;

      for (i=0; i<8; i++) {
        if ((j%32) == 0) {
          s = lte_gold_generic(&x1,&x2,reset);
          //    printf("s %x\n",s);
          reset=0;
        }

        if (((s>>(j%32))&1)==1)
          ncs_cell[ns][l] += (1<<i);

        j++;
      }

#ifdef DEBUG_PUCCH_TX
      printf("[PHY] PUCCH ncs_cell init (j %d): Ns %d, l %d => ncs_cell %d\n",j,ns,l,ncs_cell[ns][l]);
#endif
    }

  }
}

int16_t alpha_re[12] = {32767, 28377, 16383,     0,-16384,  -28378,-32768,-28378,-16384,    -1, 16383, 28377};
int16_t alpha_im[12] = {0,     16383, 28377, 32767, 28377,   16383,     0,-16384,-28378,-32768,-28378,-16384};

int16_t W4[3][4] = {{32767, 32767, 32767, 32767},
  {32767,-32768, 32767,-32768},
  {32767,-32768,-32768, 32767}
};
int16_t W3_re[3][6] = {{32767, 32767, 32767},
  {32767,-16384,-16384},
  {32767,-16384,-16384}
};

int16_t W3_im[3][6] = {{0    ,0     ,0     },
  {0    , 28377,-28378},
  {0    ,-28378, 28377}
};

char pucch_format_string[6][20] = {"format 1\0","format 1a\0","format 1b\0","format 2\0","format 2a\0","format 2b\0"};

void generate_pucch1x(int32_t **txdataF,
		      LTE_DL_FRAME_PARMS *frame_parms,
		      uint8_t ncs_cell[20][7],
		      PUCCH_FMT_t fmt,
		      PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
		      uint16_t n1_pucch,
		      uint8_t shortened_format,
		      uint8_t *payload,
		      int16_t amp,
		      uint8_t subframe)
{

  uint32_t u,v,n;
  uint32_t z[12*14],*zptr;
  int16_t d0;
  uint8_t ns,N_UL_symb,nsymb,n_oc,n_oc0,n_oc1;
  uint8_t c = (frame_parms->Ncp==0) ? 3 : 2;
  uint16_t nprime,nprime0,nprime1;
  uint16_t i,j,re_offset,thres,h;
  uint8_t Nprime_div_deltaPUCCH_Shift,Nprime,d;
  uint8_t m,l,refs;
  uint8_t n_cs,S,alpha_ind,rem;
  int16_t tmp_re,tmp_im,ref_re,ref_im,W_re=0,W_im=0;
  int32_t *txptr;
  uint32_t symbol_offset;

  uint8_t deltaPUCCH_Shift          = frame_parms->pucch_config_common.deltaPUCCH_Shift;
  uint8_t NRB2                      = frame_parms->pucch_config_common.nRB_CQI;
  uint8_t Ncs1                      = frame_parms->pucch_config_common.nCS_AN;
  uint8_t Ncs1_div_deltaPUCCH_Shift = Ncs1/deltaPUCCH_Shift;

  LOG_D(PHY,"generate_pucch Start [deltaPUCCH_Shift %d, NRB2 %d, Ncs1_div_deltaPUCCH_Shift %d, n1_pucch %d]\n", deltaPUCCH_Shift, NRB2, Ncs1_div_deltaPUCCH_Shift,n1_pucch);


  uint32_t u0 = (frame_parms->Nid_cell + frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.grouphop[subframe<<1]) % 30;
  uint32_t u1 = (frame_parms->Nid_cell + frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.grouphop[1+(subframe<<1)]) % 30;
  uint32_t v0=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[subframe<<1];
  uint32_t v1=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[1+(subframe<<1)];

  if ((deltaPUCCH_Shift==0) || (deltaPUCCH_Shift>3)) {
    printf("[PHY] generate_pucch: Illegal deltaPUCCH_shift %d (should be 1,2,3)\n",deltaPUCCH_Shift);
    return;
  }

  if (Ncs1_div_deltaPUCCH_Shift > 7) {
    printf("[PHY] generate_pucch: Illegal Ncs1_div_deltaPUCCH_Shift %d (should be 0...7)\n",Ncs1_div_deltaPUCCH_Shift);
    return;
  }

  zptr = z;
  thres = (c*Ncs1_div_deltaPUCCH_Shift);
  Nprime_div_deltaPUCCH_Shift = (n1_pucch < thres) ? Ncs1_div_deltaPUCCH_Shift : (12/deltaPUCCH_Shift);
  Nprime = Nprime_div_deltaPUCCH_Shift * deltaPUCCH_Shift;

#ifdef DEBUG_PUCCH_TX
  printf("[PHY] PUCCH: cNcs1/deltaPUCCH_Shift %d, Nprime %d, n1_pucch %d\n",thres,Nprime,n1_pucch);
#endif

  LOG_D(PHY,"[PHY] PUCCH: n1_pucch %d, thres %d Ncs1_div_deltaPUCCH_Shift %d (12/deltaPUCCH_Shift) %d Nprime_div_deltaPUCCH_Shift %d \n",
		              n1_pucch, thres, Ncs1_div_deltaPUCCH_Shift, (int)(12/deltaPUCCH_Shift), Nprime_div_deltaPUCCH_Shift);
  LOG_D(PHY,"[PHY] PUCCH: deltaPUCCH_Shift %d, Nprime %d\n",deltaPUCCH_Shift,Nprime);


  N_UL_symb = (frame_parms->Ncp==0) ? 7 : 6;

  if (n1_pucch < thres)
    nprime0=n1_pucch;
  else
    nprime0 = (n1_pucch - thres)%(12*c/deltaPUCCH_Shift);

  if (n1_pucch >= thres)
    nprime1= ((c*(nprime0+1))%((12*c/deltaPUCCH_Shift)+1))-1;
  else {
    d = (frame_parms->Ncp==0) ? 2 : 0;
    h= (nprime0+d)%(c*Nprime_div_deltaPUCCH_Shift);
#ifdef DEBUG_PUCCH_TX
    printf("[PHY] PUCCH: h %d, d %d\n",h,d);
#endif
    nprime1 = (h/c) + (h%c)*Nprime_div_deltaPUCCH_Shift;
  }

#ifdef DEBUG_PUCCH_TX
  printf("[PHY] PUCCH: nprime0 %d nprime1 %d, %s, payload (%d,%d)\n",nprime0,nprime1,pucch_format_string[fmt],payload[0],payload[1]);
#endif

  n_oc0 = nprime0/Nprime_div_deltaPUCCH_Shift;

  if (frame_parms->Ncp==1)
    n_oc0<<=1;

  n_oc1 = nprime1/Nprime_div_deltaPUCCH_Shift;

  if (frame_parms->Ncp==1)  // extended CP
    n_oc1<<=1;

#ifdef DEBUG_PUCCH_TX
  printf("[PHY] PUCCH: noc0 %d noc1 %d\n",n_oc0,n_oc1);
#endif

  nprime=nprime0;
  n_oc  =n_oc0;

  // loop over 2 slots
  for (ns=(subframe<<1),u=u0,v=v0; ns<(2+(subframe<<1)); ns++,u=u1,v=v1) {

    if ((nprime&1) == 0)
      S=0;  // 1
    else
      S=1;  // j

    //loop over symbols in slot
    for (l=0; l<N_UL_symb; l++) {
      // Compute n_cs (36.211 p. 18)
      n_cs = ncs_cell[ns][l];

      if (frame_parms->Ncp==0) { // normal CP
        n_cs = ((uint16_t)n_cs + (nprime*deltaPUCCH_Shift + (n_oc%deltaPUCCH_Shift))%Nprime)%12;
      } else {
        n_cs = ((uint16_t)n_cs + (nprime*deltaPUCCH_Shift + (n_oc>>1))%Nprime)%12;
      }


      refs=0;

      // Comput W_noc(m) (36.211 p. 19)
      if ((ns==(1+(subframe<<1))) && (shortened_format==1)) {  // second slot and shortened format

        if (l<2) {                                         // data
          W_re=W3_re[n_oc][l];
          W_im=W3_im[n_oc][l];
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==0)) { // reference and normal CP
          W_re=W3_re[n_oc][l-2];
          W_im=W3_im[n_oc][l-2];
          refs=1;
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==1)) { // reference and extended CP
          W_re=W4[n_oc][l-2];
          W_im=0;
          refs=1;
        } else if ((l>=N_UL_symb-2)) {                      // data
          W_re=W3_re[n_oc][l-N_UL_symb+4];
          W_im=W3_im[n_oc][l-N_UL_symb+4];
        }
      } else {
        if (l<2) {                                         // data
          W_re=W4[n_oc][l];
          W_im=0;
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==0)) { // reference and normal CP
          W_re=W3_re[n_oc][l-2];
          W_im=W3_im[n_oc][l-2];
          refs=1;
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==1)) { // reference and extended CP
          W_re=W4[n_oc][l-2];
          W_im=0;
          refs=1;
        } else if ((l>=N_UL_symb-2)) {                     // data
          W_re=W4[n_oc][l-N_UL_symb+4];
          W_im=0;
        }
      }

      // multiply W by S(ns) (36.211 p.17). only for data, reference symbols do not have this factor
      if ((S==1)&&(refs==0)) {
        tmp_re = W_re;
        W_re = -W_im;
        W_im = tmp_re;
      }

#ifdef DEBUG_PUCCH_TX
      printf("[PHY] PUCCH: ncs[%d][%d]=%d, W_re %d, W_im %d, S %d, refs %d\n",ns,l,n_cs,W_re,W_im,S,refs);
#endif
      alpha_ind=0;
      // compute output sequence

      for (n=0; n<12; n++) {

        // this is r_uv^alpha(n)
        tmp_re = (int16_t)(((int32_t)alpha_re[alpha_ind] * ul_ref_sigs[u][v][0][n<<1] - (int32_t)alpha_im[alpha_ind] * ul_ref_sigs[u][v][0][1+(n<<1)])>>15);
        tmp_im = (int16_t)(((int32_t)alpha_re[alpha_ind] * ul_ref_sigs[u][v][0][1+(n<<1)] + (int32_t)alpha_im[alpha_ind] * ul_ref_sigs[u][v][0][n<<1])>>15);

        // this is S(ns)*w_noc(m)*r_uv^alpha(n)
        ref_re = (tmp_re*W_re - tmp_im*W_im)>>15;
        ref_im = (tmp_re*W_im + tmp_im*W_re)>>15;

        if ((l<2)||(l>=(N_UL_symb-2))) { //these are PUCCH data symbols
          switch (fmt) {
          case pucch_format1:   //OOK 1-bit

            ((int16_t *)&zptr[n])[0] = ((int32_t)amp*ref_re)>>15;
            ((int16_t *)&zptr[n])[1] = ((int32_t)amp*ref_im)>>15;

            break;

          case pucch_format1a:  //BPSK 1-bit
            d0 = (payload[0]&1)==0 ? amp : -amp;
            ((int16_t *)&zptr[n])[0] = ((int32_t)d0*ref_re)>>15;
            ((int16_t *)&zptr[n])[1] = ((int32_t)d0*ref_im)>>15;
            //      printf("d0 %d\n",d0);
            break;

          case pucch_format1b:  //QPSK 2-bits (Table 5.4.1-1 from 36.211, pg. 18)
            if (((payload[0]&1)==0) && ((payload[1]&1)==0))  {// 1
              ((int16_t *)&zptr[n])[0] = ((int32_t)amp*ref_re)>>15;
              ((int16_t *)&zptr[n])[1] = ((int32_t)amp*ref_im)>>15;
            } else if (((payload[0]&1)==0) && ((payload[1]&1)==1))  { // -j
              ((int16_t *)&zptr[n])[0] = ((int32_t)amp*ref_im)>>15;
              ((int16_t *)&zptr[n])[1] = (-(int32_t)amp*ref_re)>>15;
            } else if (((payload[0]&1)==1) && ((payload[1]&1)==0))  { // j
              ((int16_t *)&zptr[n])[0] = (-(int32_t)amp*ref_im)>>15;
              ((int16_t *)&zptr[n])[1] = ((int32_t)amp*ref_re)>>15;
            } else  { // -1
              ((int16_t *)&zptr[n])[0] = (-(int32_t)amp*ref_re)>>15;
              ((int16_t *)&zptr[n])[1] = (-(int32_t)amp*ref_im)>>15;
            }

            break;

          case pucch_format2:
          case pucch_format2a:
          case pucch_format2b:
            AssertFatal(1==0,"should not go here\n");
            break;
          } // switch fmt
        } else { // These are PUCCH reference symbols

          ((int16_t *)&zptr[n])[0] = ((int32_t)amp*ref_re)>>15;
          ((int16_t *)&zptr[n])[1] = ((int32_t)amp*ref_im)>>15;
          //    printf("ref\n");
        }

#ifdef DEBUG_PUCCH_TX
        printf("[PHY] PUCCH subframe %d z(%d,%d) => %d,%d, alpha(%d) => %d,%d\n",subframe,l,n,((int16_t *)&zptr[n])[0],((int16_t *)&zptr[n])[1],
            alpha_ind,alpha_re[alpha_ind],alpha_im[alpha_ind]);
#endif
        alpha_ind = (alpha_ind + n_cs)%12;
      } // n

      zptr+=12;
    } // l

    nprime=nprime1;
    n_oc  =n_oc1;
  } // ns

  rem = ((((12*Ncs1_div_deltaPUCCH_Shift)>>3)&7)>0) ? 1 : 0;

  m = (n1_pucch < thres) ? NRB2 : (((n1_pucch-thres)/(12*c/deltaPUCCH_Shift))+NRB2+((deltaPUCCH_Shift*Ncs1_div_deltaPUCCH_Shift)>>3)+rem);

#ifdef DEBUG_PUCCH_TX
  printf("[PHY] PUCCH: m %d\n",m);
#endif
  nsymb = N_UL_symb<<1;

  //for (j=0,l=0;l<(nsymb-1);l++) {
  for (j=0,l=0; l<(nsymb); l++) {
    if ((l<(nsymb>>1)) && ((m&1) == 0))
      re_offset = (m*6) + frame_parms->first_carrier_offset;
    else if ((l<(nsymb>>1)) && ((m&1) == 1))
      re_offset = frame_parms->first_carrier_offset + (frame_parms->N_RB_DL - (m>>1) - 1)*12;
    else if ((m&1) == 0)
      re_offset = frame_parms->first_carrier_offset + (frame_parms->N_RB_DL - (m>>1) - 1)*12;
    else
      re_offset = ((m-1)*6) + frame_parms->first_carrier_offset;

    if (re_offset > frame_parms->ofdm_symbol_size)
      re_offset -= (frame_parms->ofdm_symbol_size);

    symbol_offset = (unsigned int)frame_parms->ofdm_symbol_size*(l+(subframe*nsymb));
    txptr = &txdataF[0][symbol_offset];

    for (i=0; i<12; i++,j++) {
      txptr[re_offset++] = z[j];

      if (re_offset==frame_parms->ofdm_symbol_size)
        re_offset = 0;

#ifdef DEBUG_PUCCH_TX
      printf("[PHY] PUCCH subframe %d (%d,%d,%d,%d) => %d,%d\n",subframe,l,i,re_offset-1,m,((int16_t *)&z[j])[0],((int16_t *)&z[j])[1]);
#endif
    }
  }

}

void generate_pucch_emul(PHY_VARS_UE *ue,
			 UE_rxtx_proc_t *proc,
                         PUCCH_FMT_t format,
                         uint8_t ncs1,
                         uint8_t *pucch_payload,
                         uint8_t sr)

{

  int subframe = proc->subframe_tx;

  UE_transport_info[ue->Mod_id][ue->CC_id].cntl.pucch_flag    = format;
  UE_transport_info[ue->Mod_id][ue->CC_id].cntl.pucch_Ncs1    = ncs1;


  UE_transport_info[ue->Mod_id][ue->CC_id].cntl.sr            = sr;
  // the value of ue->pucch_sel[subframe] is set by get_n1_pucch
  UE_transport_info[ue->Mod_id][ue->CC_id].cntl.pucch_sel      = ue->pucch_sel[subframe];

  // LOG_I(PHY,"subframe %d emu tx pucch_sel is %d sr is %d \n", subframe, UE_transport_info[ue->Mod_id].cntl.pucch_sel, sr);

  if (format == pucch_format1a) {

    ue->pucch_payload[0] = pucch_payload[0];
    UE_transport_info[ue->Mod_id][ue->CC_id].cntl.pucch_payload = pucch_payload[0];
  } else if (format == pucch_format1b) {
    ue->pucch_payload[0] = pucch_payload[0] + (pucch_payload[1]<<1);
    UE_transport_info[ue->Mod_id][ue->CC_id].cntl.pucch_payload = pucch_payload[0] + (pucch_payload[1]<<1);
  } else if (format == pucch_format1) {
    //    LOG_D(PHY,"[UE %d] Frame %d subframe %d Generating PUCCH for SR %d\n",ue->Mod_id,proc->frame_tx,subframe,sr);
  }

  ue->sr[subframe] = sr;

}


inline void pucch2x_scrambling(LTE_DL_FRAME_PARMS *fp,int subframe,uint16_t rnti,uint32_t B,uint8_t *btilde) __attribute__((always_inline));
inline void pucch2x_scrambling(LTE_DL_FRAME_PARMS *fp,int subframe,uint16_t rnti,uint32_t B,uint8_t *btilde) {

  uint32_t x1, x2, s=0;
  int i;
  uint8_t c;

  x2 = (rnti) + ((uint32_t)(1+subframe)<<16)*(1+(fp->Nid_cell<<1)); //this is c_init in 36.211 Sec 6.3.1
  s = lte_gold_generic(&x1, &x2, 1);
  for (i=0;i<19;i++) {
    c = (uint8_t)((s>>i)&1);
    btilde[i] = (((B>>i)&1) ^ c);
  }
}

inline void pucch2x_modulation(uint8_t *btilde,int16_t *d,int16_t amp) __attribute__((always_inline));
inline void pucch2x_modulation(uint8_t *btilde,int16_t *d,int16_t amp) {

  int i;

  for (i=0;i<20;i++) 
    d[i] = btilde[i] == 1 ? -amp : amp;

}



uint32_t pucch_code[13] = {0xFFFFF,0x5A933,0x10E5A,0x6339C,0x73CE0,
			   0xFFC00,0xD8E64,0x4F6B0,0x218EC,0x1B746,
			   0x0FFFF,0x33FFF,0x3FFFC};


void generate_pucch2x(int32_t **txdataF,
		      LTE_DL_FRAME_PARMS *fp,
		      uint8_t ncs_cell[20][7],
		      PUCCH_FMT_t fmt,
		      PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
		      uint16_t n2_pucch,
		      uint16_t *payload,
		      int A,
		      int B2,
		      int16_t amp,
		      uint8_t subframe,
		      uint16_t rnti) {

  int i,j;
  uint32_t B=0;
  uint8_t btilde[20];
  int16_t d[22];
  uint8_t deltaPUCCH_Shift          = fp->pucch_config_common.deltaPUCCH_Shift;
  uint8_t NRB2                      = fp->pucch_config_common.nRB_CQI;
  uint8_t Ncs1                      = fp->pucch_config_common.nCS_AN;

  uint32_t u0 = fp->pucch_config_common.grouphop[subframe<<1];
  uint32_t u1 = fp->pucch_config_common.grouphop[1+(subframe<<1)];
  uint32_t v0 = fp->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[subframe<<1];
  uint32_t v1 = fp->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[1+(subframe<<1)];

  uint32_t z[12*14],*zptr;
  uint32_t u,v,n;
  uint8_t ns,N_UL_symb,nsymb_slot0,nsymb_pertti;
  uint32_t nprime,l,n_cs;
  int alpha_ind,data_ind;
  int16_t ref_re,ref_im;
  int m,re_offset,symbol_offset;
  int32_t *txptr;

  if ((deltaPUCCH_Shift==0) || (deltaPUCCH_Shift>3)) {
    printf("[PHY] generate_pucch: Illegal deltaPUCCH_shift %d (should be 1,2,3)\n",deltaPUCCH_Shift);
    return;
  }

  if (Ncs1 > 7) {
    printf("[PHY] generate_pucch: Illegal Ncs1 %d (should be 0...7)\n",Ncs1);
    return;
  }

  // pucch2x_encoding
  for (i=0;i<A;i++)
    if ((*payload & (1<<i)) > 0)
      B=B^pucch_code[i];

  // scrambling
  pucch2x_scrambling(fp,subframe,rnti,B,btilde);
  // modulation
  pucch2x_modulation(btilde,d,amp);

  // add extra symbol for 2a/2b
  d[20]=0;
  d[21]=0;
  if (fmt==pucch_format2a)
    d[20] = (B2 == 0) ? amp : -amp;
  else if (fmt==pucch_format2b) {
    switch (B2) {
    case 0:
      d[20] = amp;
      break;
    case 1:
      d[21] = -amp;
      break;
    case 2:
      d[21] = amp;
      break;
    case 3:
      d[20] = -amp;
      break;
    default:
      AssertFatal(1==0,"Illegal modulation symbol %d for PUCCH %s\n",B2,pucch_format_string[fmt]);
      break;
    }
  }


#ifdef DEBUG_PUCCH_TX
  printf("[PHY] PUCCH2x: n2_pucch %d\n",n2_pucch);
#endif

  N_UL_symb = (fp->Ncp==0) ? 7 : 6;
  data_ind  = 0;
  zptr      = z;
  for (ns=(subframe<<1),u=u0,v=v0; ns<(2+(subframe<<1)); ns++,u=u1,v=v1) {

    if ((ns&1) == 0)
        nprime = (n2_pucch < 12*NRB2) ?
                n2_pucch % 12 :
                (n2_pucch+Ncs1 + 1)%12;
    else {
        nprime = (n2_pucch < 12*NRB2) ?
                ((12*(nprime+1)) % 13)-1 :
                (10-n2_pucch)%12;
    }
    //loop over symbols in slot
    for (l=0; l<N_UL_symb; l++) {
      // Compute n_cs (36.211 p. 18)
      n_cs = (ncs_cell[ns][l]+nprime)%12;

      alpha_ind = 0;
      for (n=0; n<12; n++) {
          // this is r_uv^alpha(n)
          ref_re = (int16_t)(((int32_t)alpha_re[alpha_ind] * ul_ref_sigs[u][v][0][n<<1] - (int32_t)alpha_im[alpha_ind] * ul_ref_sigs[u][v][0][1+(n<<1)])>>15);
          ref_im = (int16_t)(((int32_t)alpha_re[alpha_ind] * ul_ref_sigs[u][v][0][1+(n<<1)] + (int32_t)alpha_im[alpha_ind] * ul_ref_sigs[u][v][0][n<<1])>>15);

          if ((l!=1)&&(l!=5)) { //these are PUCCH data symbols
              ((int16_t *)&zptr[n])[0] = ((int32_t)d[data_ind]*ref_re - (int32_t)d[data_ind+1]*ref_im)>>15;
              ((int16_t *)&zptr[n])[1] = ((int32_t)d[data_ind]*ref_im + (int32_t)d[data_ind+1]*ref_re)>>15;
              //LOG_I(PHY,"slot %d ofdm# %d ==> d[%d,%d] \n",ns,l,data_ind,n);
          }
          else {
              ((int16_t *)&zptr[n])[0] = ((int32_t)amp*ref_re>>15);
              ((int16_t *)&zptr[n])[1] = ((int32_t)amp*ref_im>>15);
              //LOG_I(PHY,"slot %d ofdm# %d ==> dmrs[%d] \n",ns,l,n);
          }
          alpha_ind = (alpha_ind + n_cs)%12;
      } // n
      zptr+=12;

      if ((l!=1)&&(l!=5))  //these are PUCCH data symbols so increment data index
	     data_ind+=2;
    } // l
  } //ns

  m = n2_pucch/12;

#ifdef DEBUG_PUCCH_TX
  LOG_D(PHY,"[PHY] PUCCH: n2_pucch %d m %d\n",n2_pucch,m);
#endif

  nsymb_slot0  = ((fp->Ncp==0) ? 7 : 6);
  nsymb_pertti = nsymb_slot0 << 1;

  //nsymb = nsymb_slot0<<1;

  //for (j=0,l=0;l<(nsymb-1);l++) {
  for (j=0,l=0; l<(nsymb_pertti); l++) {

    if ((l<nsymb_slot0) && ((m&1) == 0))
      re_offset = (m*6) + fp->first_carrier_offset;
    else if ((l<nsymb_slot0) && ((m&1) == 1))
      re_offset = fp->first_carrier_offset + (fp->N_RB_DL - (m>>1) - 1)*12;
    else if ((m&1) == 0)
      re_offset = fp->first_carrier_offset + (fp->N_RB_DL - (m>>1) - 1)*12;
    else
      re_offset = ((m-1)*6) + fp->first_carrier_offset;

    if (re_offset > fp->ofdm_symbol_size)
      re_offset -= (fp->ofdm_symbol_size);



    symbol_offset = (unsigned int)fp->ofdm_symbol_size*(l+(subframe*nsymb_pertti));
    txptr = &txdataF[0][symbol_offset];

    //LOG_I(PHY,"ofdmSymb %d/%d, firstCarrierOffset %d, symbolOffset[sfn %d] %d, reOffset %d, &txptr: %x \n", l, nsymb, fp->first_carrier_offset, subframe, symbol_offset, re_offset, &txptr[0]);

    for (i=0; i<12; i++,j++) {
      txptr[re_offset] = z[j];

      re_offset++;

      if (re_offset==fp->ofdm_symbol_size)
          re_offset -= (fp->ofdm_symbol_size);

#ifdef DEBUG_PUCCH_TX
      LOG_D(PHY,"[PHY] PUCCH subframe %d (%d,%d,%d,%d) => %d,%d\n",subframe,l,i,re_offset-1,m,((int16_t *)&z[j])[0],((int16_t *)&z[j])[1]);
#endif
    }
  }
}


//#define Amax 13
//void init_pucch2x_rx() {};


uint32_t rx_pucch(PHY_VARS_eNB *eNB,
		  PUCCH_FMT_t fmt,
		  uint8_t UE_id,
		  uint16_t n1_pucch,
		  uint16_t n2_pucch,
		  uint8_t shortened_format,
		  uint8_t *payload,
		  int     frame,
		  uint8_t subframe,
		  uint8_t pucch1_thres)
{


  static int first_call=1;
  LTE_eNB_COMMON *common_vars                = &eNB->common_vars;
  LTE_DL_FRAME_PARMS *frame_parms                    = &eNB->frame_parms;
  //  PUCCH_CONFIG_DEDICATED *pucch_config_dedicated = &eNB->pucch_config_dedicated[UE_id];
  int8_t sigma2_dB                                   = eNB->measurements[0].n0_subband_power_tot_dB[0]-10;
  uint32_t *Po_PUCCH                                  = &(eNB->UE_stats[UE_id].Po_PUCCH);
  int32_t *Po_PUCCH_dBm                              = &(eNB->UE_stats[UE_id].Po_PUCCH_dBm);
  uint32_t *Po_PUCCH1_below                           = &(eNB->UE_stats[UE_id].Po_PUCCH1_below);
  uint32_t *Po_PUCCH1_above                           = &(eNB->UE_stats[UE_id].Po_PUCCH1_above);
  int32_t *Po_PUCCH_update                           = &(eNB->UE_stats[UE_id].Po_PUCCH_update);
  uint32_t u,v,n,aa;
  uint32_t z[12*14];
  int16_t *zptr;
  int16_t rxcomp[NB_ANTENNAS_RX][2*12*14];
  uint8_t ns,N_UL_symb,nsymb,n_oc,n_oc0,n_oc1;
  uint8_t c = (frame_parms->Ncp==0) ? 3 : 2;
  uint16_t nprime,nprime0,nprime1;
  uint16_t i,j,re_offset,thres,h,off;
  uint8_t Nprime_div_deltaPUCCH_Shift,Nprime,d;
  uint8_t m,l,refs,phase,re,l2,phase_max=0;
  uint8_t n_cs,S,alpha_ind,rem;
  int16_t tmp_re,tmp_im,W_re=0,W_im=0;
  int16_t *rxptr;
  uint32_t symbol_offset;
  int16_t stat_ref_re,stat_ref_im,*cfo,chest_re,chest_im;
  int32_t stat_re=0,stat_im=0;
  uint32_t stat,stat_max=0;

  uint8_t deltaPUCCH_Shift          = frame_parms->pucch_config_common.deltaPUCCH_Shift;
  uint8_t NRB2                      = frame_parms->pucch_config_common.nRB_CQI;
  uint8_t Ncs1_div_deltaPUCCH_Shift = frame_parms->pucch_config_common.nCS_AN;

  uint32_t u0 = (frame_parms->Nid_cell + frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.grouphop[subframe<<1]) % 30;
  uint32_t u1 = (frame_parms->Nid_cell + frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.grouphop[1+(subframe<<1)]) % 30;
  uint32_t v0=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[subframe<<1];
  uint32_t v1=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[1+(subframe<<1)];
  int chL;

  if (first_call == 1) {
    for (i=0;i<10;i++) {
      for (j=0;j<NUMBER_OF_UE_MAX;j++) {
	eNB->pucch1_stats_cnt[j][i]=0;
	eNB->pucch1ab_stats_cnt[j][i]=0;
      }
    }
    first_call=0;
  }
  /*
  switch (frame_parms->N_RB_UL) {

  case 6:
    sigma2_dB -= 8;
    break;
  case 25:
    sigma2_dB -= 14;
    break;
  case 50:
    sigma2_dB -= 17;
    break;
  case 100:
    sigma2_dB -= 20;
    break;
  default:
    sigma2_dB -= 14;
  }
  */  

    
  if ((deltaPUCCH_Shift==0) || (deltaPUCCH_Shift>3)) {
    LOG_E(PHY,"[eNB] rx_pucch: Illegal deltaPUCCH_shift %d (should be 1,2,3)\n",deltaPUCCH_Shift);
    return(-1);
  }

  if (Ncs1_div_deltaPUCCH_Shift > 7) {
    LOG_E(PHY,"[eNB] rx_pucch: Illegal Ncs1_div_deltaPUCCH_Shift %d (should be 0...7)\n",Ncs1_div_deltaPUCCH_Shift);
    return(-1);
  }

  zptr = (int16_t *)z;
  thres = (c*Ncs1_div_deltaPUCCH_Shift);
  Nprime_div_deltaPUCCH_Shift = (n1_pucch < thres) ? Ncs1_div_deltaPUCCH_Shift : (12/deltaPUCCH_Shift);
  Nprime = Nprime_div_deltaPUCCH_Shift * deltaPUCCH_Shift;

#ifdef DEBUG_PUCCH_RX
  printf("[eNB] PUCCH: cNcs1/deltaPUCCH_Shift %d, Nprime %d, n1_pucch %d\n",thres,Nprime,n1_pucch);
#endif

  N_UL_symb = (frame_parms->Ncp==NORMAL) ? 7 : 6;

  if (n1_pucch < thres)
    nprime0=n1_pucch;
  else
    nprime0 = (n1_pucch - thres)%(12*c/deltaPUCCH_Shift);

  if (n1_pucch >= thres)
    nprime1= ((c*(nprime0+1))%((12*c/deltaPUCCH_Shift)+1))-1;
  else {
    d = (frame_parms->Ncp==0) ? 2 : 0;
    h= (nprime0+d)%(c*Nprime_div_deltaPUCCH_Shift);
    nprime1 = (h/c) + (h%c)*Nprime_div_deltaPUCCH_Shift;
  }

#ifdef DEBUG_PUCCH_RX
  printf("PUCCH: nprime0 %d nprime1 %d\n",nprime0,nprime1);
#endif

  n_oc0 = nprime0/Nprime_div_deltaPUCCH_Shift;

  if (frame_parms->Ncp==1)
    n_oc0<<=1;

  n_oc1 = nprime1/Nprime_div_deltaPUCCH_Shift;

  if (frame_parms->Ncp==1)  // extended CP
    n_oc1<<=1;

#ifdef DEBUG_PUCCH_RX
  printf("[eNB] PUCCH: noc0 %d noc11 %d\n",n_oc0,n_oc1);
#endif

  nprime=nprime0;
  n_oc  =n_oc0;

  // loop over 2 slots
  for (ns=(subframe<<1),u=u0,v=v0; ns<(2+(subframe<<1)); ns++,u=u1,v=v1) {

    if ((nprime&1) == 0)
      S=0;  // 1
    else
      S=1;  // j
    /*
    if (fmt==pucch_format1)
      LOG_I(PHY,"[eNB] subframe %d => PUCCH1: u%d %d, v%d %d : ", subframe,ns&1,u,ns&1,v);
    else
      LOG_I(PHY,"[eNB] subframe %d => PUCCH1a/b: u%d %d, v%d %d : ", subframe,ns&1,u,ns&1,v);
    */

    //loop over symbols in slot
    for (l=0; l<N_UL_symb; l++) {
      // Compute n_cs (36.211 p. 18)
      n_cs = eNB->ncs_cell[ns][l];

      if (frame_parms->Ncp==0) { // normal CP
        n_cs = ((uint16_t)n_cs + (nprime*deltaPUCCH_Shift + (n_oc%deltaPUCCH_Shift))%Nprime)%12;
      } else {
        n_cs = ((uint16_t)n_cs + (nprime*deltaPUCCH_Shift + (n_oc>>1))%Nprime)%12;
      }



      refs=0;

      // Comput W_noc(m) (36.211 p. 19)
      if ((ns==(1+(subframe<<1))) && (shortened_format==1)) {  // second slot and shortened format

        if (l<2) {                                         // data
          W_re=W3_re[n_oc][l];
          W_im=W3_im[n_oc][l];
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==0)) { // reference and normal CP
          W_re=W3_re[n_oc][l-2];
          W_im=W3_im[n_oc][l-2];
          refs=1;
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==1)) { // reference and extended CP
          W_re=W4[n_oc][l-2];
          W_im=0;
          refs=1;
        } else if ((l>=N_UL_symb-2)) {                      // data
          W_re=W3_re[n_oc][l-N_UL_symb+4];
          W_im=W3_im[n_oc][l-N_UL_symb+4];
        }
      } else {
        if (l<2) {                                         // data
          W_re=W4[n_oc][l];
          W_im=0;
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==NORMAL)) { // reference and normal CP
          W_re=W3_re[n_oc][l-2];
          W_im=W3_im[n_oc][l-2];
          refs=1;
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==EXTENDED)) { // reference and extended CP
          W_re=W4[n_oc][l-2];
          W_im=0;
          refs=1;
        } else if ((l>=N_UL_symb-2)) {                     // data
          W_re=W4[n_oc][l-N_UL_symb+4];
          W_im=0;
        }
      }

      // multiply W by S(ns) (36.211 p.17). only for data, reference symbols do not have this factor
      if ((S==1)&&(refs==0)) {
        tmp_re = W_re;
        W_re = -W_im;
        W_im = tmp_re;
      }

#ifdef DEBUG_PUCCH_RX
      printf("[eNB] PUCCH: ncs[%d][%d]=%d, W_re %d, W_im %d, S %d, refs %d\n",ns,l,n_cs,W_re,W_im,S,refs);
#endif
      alpha_ind=0;
      // compute output sequence

      for (n=0; n<12; n++) {

        // this is r_uv^alpha(n)
        tmp_re = (int16_t)(((int32_t)alpha_re[alpha_ind] * ul_ref_sigs[u][v][0][n<<1] - (int32_t)alpha_im[alpha_ind] * ul_ref_sigs[u][v][0][1+(n<<1)])>>15);
        tmp_im = (int16_t)(((int32_t)alpha_re[alpha_ind] * ul_ref_sigs[u][v][0][1+(n<<1)] + (int32_t)alpha_im[alpha_ind] * ul_ref_sigs[u][v][0][n<<1])>>15);

        // this is S(ns)*w_noc(m)*r_uv^alpha(n)
        zptr[n<<1] = (tmp_re*W_re - tmp_im*W_im)>>15;
        zptr[1+(n<<1)] = -(tmp_re*W_im + tmp_im*W_re)>>15;

#ifdef DEBUG_PUCCH_RX
        printf("[eNB] PUCCH subframe %d z(%d,%d) => %d,%d, alpha(%d) => %d,%d\n",subframe,l,n,zptr[n<<1],zptr[(n<<1)+1],
              alpha_ind,alpha_re[alpha_ind],alpha_im[alpha_ind]);
#endif

        alpha_ind = (alpha_ind + n_cs)%12;
      } // n

      zptr+=24;
    } // l

    nprime=nprime1;
    n_oc  =n_oc1;
  } // ns

  rem = ((((deltaPUCCH_Shift*Ncs1_div_deltaPUCCH_Shift)>>3)&7)>0) ? 1 : 0;

  m = (n1_pucch < thres) ? NRB2 : (((n1_pucch-thres)/(12*c/deltaPUCCH_Shift))+NRB2+((deltaPUCCH_Shift*Ncs1_div_deltaPUCCH_Shift)>>3)+rem);

#ifdef DEBUG_PUCCH_RX
  printf("[eNB] PUCCH: m %d\n",m);
#endif
  nsymb = N_UL_symb<<1;

  zptr = (int16_t*)z;

  // Do detection
  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {

    //for (j=0,l=0;l<(nsymb-1);l++) {
    for (j=0,l=0; l<nsymb; l++) {
      if ((l<(nsymb>>1)) && ((m&1) == 0))
        re_offset = (m*6) + frame_parms->first_carrier_offset;
      else if ((l<(nsymb>>1)) && ((m&1) == 1))
        re_offset = frame_parms->first_carrier_offset + (frame_parms->N_RB_DL - (m>>1) - 1)*12;
      else if ((m&1) == 0)
        re_offset = frame_parms->first_carrier_offset + (frame_parms->N_RB_DL - (m>>1) - 1)*12;
      else
        re_offset = ((m-1)*6) + frame_parms->first_carrier_offset;

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size);

      symbol_offset = (unsigned int)frame_parms->ofdm_symbol_size*l;
      rxptr = (int16_t *)&common_vars->rxdataF[0][aa][symbol_offset];

      for (i=0; i<12; i++,j+=2,re_offset++) {
        rxcomp[aa][j]   = (int16_t)((rxptr[re_offset<<1]*(int32_t)zptr[j])>>15)   - ((rxptr[1+(re_offset<<1)]*(int32_t)zptr[1+j])>>15);
        rxcomp[aa][1+j] = (int16_t)((rxptr[re_offset<<1]*(int32_t)zptr[1+j])>>15) + ((rxptr[1+(re_offset<<1)]*(int32_t)zptr[j])>>15);

        if (re_offset==frame_parms->ofdm_symbol_size)
          re_offset = 0;

#ifdef DEBUG_PUCCH_RX
        printf("[eNB] PUCCH subframe %d (%d,%d,%d,%d,%d) => (%d,%d) x (%d,%d) : (%d,%d)\n",subframe,l,i,re_offset,m,j,
              rxptr[re_offset<<1],rxptr[1+(re_offset<<1)],
              zptr[j],zptr[1+j],
              rxcomp[aa][j],rxcomp[aa][1+j]);
#endif
      } //re
    } // symbol
  }  // antenna


  // PUCCH Format 1
  // Do cfo correction and MRC across symbols

  if (fmt == pucch_format1) {
#ifdef DEBUG_PUCCH_RX
    printf("Doing PUCCH detection for format 1\n");
#endif

    stat_max = 0;


    for (phase=0; phase<7; phase++) {
      stat=0;

      for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (re=0; re<12; re++) {
          stat_re=0;
          stat_im=0;
          off=re<<1;
          cfo =  (frame_parms->Ncp==0) ? &cfo_pucch_np[14*phase] : &cfo_pucch_ep[12*phase];

          for (l=0; l<(nsymb>>1); l++) {
            stat_re += (((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15))/nsymb;
            stat_im += (((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15))/nsymb;
            off+=2;

		    
#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d,%d) => (%d,%d) x (%d,%d) : (%d,%d) , stat %d\n",subframe,phase,l,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l<<1],cfo[1+(l<<1)],
                  stat_re,stat_im,stat);
#endif
          }

          for (l2=0,l=(nsymb>>1); l<(nsymb-1); l++,l2++) {
            stat_re += (((rxcomp[aa][off]*(int32_t)cfo[l2<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l2<<1)])>>15))/nsymb;
            stat_im += (((rxcomp[aa][off]*(int32_t)cfo[1+(l2<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l2<<1)])>>15))/nsymb;
            off+=2;

#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d,%d) => (%d,%d) x (%d,%d) : (%d,%d), stat %d\n",subframe,phase,l2,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l2<<1],cfo[1+(l2<<1)],
                  stat_re,stat_im,stat);
#endif


          }
	  stat += ((stat_re*stat_re) + (stat_im*stat_im));

       } //re
      } // aa

 
      if (stat>stat_max) {
        stat_max = stat;
        phase_max = phase;
      }

    } //phase

    stat_max *= nsymb;  // normalize to energy per symbol
    stat_max /= (frame_parms->N_RB_UL*12); // 
#ifdef DEBUG_PUCCH_RX
    printf("[eNB] PUCCH: stat %d, stat_max %d, phase_max %d\n", stat,stat_max,phase_max);
#endif

#ifdef DEBUG_PUCCH_RX
    LOG_I(PHY,"[eNB] PUCCH fmt1:  stat_max : %d, sigma2_dB %d (%d, %d), phase_max : %d\n",dB_fixed(stat_max),sigma2_dB,eNB->measurements[0].n0_subband_power_tot_dBm[6],pucch1_thres,phase_max);
#endif

    eNB->pucch1_stats[UE_id][(subframe<<10)+eNB->pucch1_stats_cnt[UE_id][subframe]] = stat_max;
    eNB->pucch1_stats_thres[UE_id][(subframe<<10)+eNB->pucch1_stats_cnt[UE_id][subframe]] = sigma2_dB+pucch1_thres;
    eNB->pucch1_stats_cnt[UE_id][subframe] = (eNB->pucch1_stats_cnt[UE_id][subframe]+1)&1023;

    T(T_ENB_PHY_PUCCH_1_ENERGY, T_INT(eNB->Mod_id), T_INT(UE_id), T_INT(frame), T_INT(subframe),
      T_INT(stat_max), T_INT(sigma2_dB+pucch1_thres));

    /*
    if (eNB->pucch1_stats_cnt[UE_id][subframe] == 0) {
      write_output("pucch_debug.m","pucch_energy",
		   &eNB->pucch1_stats[UE_id][(subframe<<10)],
		   1024,1,2);
      AssertFatal(0,"Exiting for PUCCH 1 debug\n");

    }
    */

    // This is a moving average of the PUCCH1 statistics conditioned on being above or below the threshold
    if (sigma2_dB<(dB_fixed(stat_max)-pucch1_thres))  {
      *payload = 1;
      *Po_PUCCH1_above = ((*Po_PUCCH1_above<<9) + (stat_max<<9)+1024)>>10;
      //LOG_I(PHY,"[eNB] PUCCH fmt1:  stat_max : %d, sigma2_dB %d (%d, %d), phase_max : %d\n",dB_fixed(stat_max),sigma2_dB,eNB->PHY_measurements_eNB[0].n0_power_tot_dBm,pucch1_thres,phase_max);
    }
    else {
      *payload = 0;
      *Po_PUCCH1_below = ((*Po_PUCCH1_below<<9) + (stat_max<<9)+1024)>>10;
    }
    //printf("[eNB] PUCCH fmt1:  stat_max : %d, sigma2_dB %d (I0 %d dBm, thres %d), Po_PUCCH1_below/above : %d / %d\n",dB_fixed(stat_max),sigma2_dB,eNB->measurements[0].n0_subband_power_tot_dBm[6],pucch1_thres,dB_fixed(*Po_PUCCH1_below),dB_fixed(*Po_PUCCH1_above));
    *Po_PUCCH_update = 1;
    if (UE_id==0) {
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SR_ENERGY,dB_fixed(stat_max));
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SR_THRES,sigma2_dB+pucch1_thres);
    }
  } else if ((fmt == pucch_format1a)||(fmt == pucch_format1b)) {
    stat_max = 0;
#ifdef DEBUG_PUCCH_RX
    LOG_I(PHY,"Doing PUCCH detection for format 1a/1b\n");
#endif

    for (phase=3;phase<4;phase++){ //phase=0; phase<7; phase++) {
      stat=0;

      for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (re=0; re<12; re++) {
          stat_re=0;
          stat_im=0;
          stat_ref_re=0;
          stat_ref_im=0;
          off=re<<1;
          cfo =  (frame_parms->Ncp==0) ? &cfo_pucch_np[14*phase] : &cfo_pucch_ep[12*phase];


          for (l=0; l<(nsymb>>1); l++) {
            if ((l<2)||(l>(nsymb>>1) - 3)) {  //data symbols
              stat_re += ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
              stat_im += ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            } else { //reference symbols
              stat_ref_re += ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
              stat_ref_im += ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            }

            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d)\n",subframe,l,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l<<1],cfo[1+(l<<1)],
                  stat_re,stat_im);
#endif
          }




          for (l2=0,l=(nsymb>>1); l<(nsymb-1); l++,l2++) {
            if ((l2<2) || ((l2>(nsymb>>1) - 3)) ) {  // data symbols
              stat_re += ((rxcomp[aa][off]*(int32_t)cfo[l2<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l2<<1)])>>15);
              stat_im += ((rxcomp[aa][off]*(int32_t)cfo[1+(l2<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l2<<1)])>>15);
            } else { //reference_symbols
              stat_ref_re += ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
              stat_ref_im += ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            }

            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d)\n",subframe,l2,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l2<<1],cfo[1+(l2<<1)],
                  stat_re,stat_im);
#endif

          }

#ifdef DEBUG_PUCCH_RX
          printf("aa%d re %d : phase %d : stat %d\n",aa,re,phase,stat);
#endif

	  stat += ((((stat_re*stat_re)) + ((stat_im*stat_im)) +
		    ((stat_ref_re*stat_ref_re)) + ((stat_ref_im*stat_ref_im)))/nsymb);


        } //re
      } // aa

#ifdef DEBUG_PUCCH_RX
      LOG_I(PHY,"Format 1A: phase %d : stat %d\n",phase,stat);
#endif
      if (stat>stat_max) {
        stat_max = stat;
        phase_max = phase;
      }
    } //phase

    stat_max/=(12);  //normalize to energy per symbol and RE
#ifdef DEBUG_PUCCH_RX
    printf("[eNB] PUCCH fmt1a/b:  stat_max : %d, phase_max : %d\n",stat_max,phase_max);
#endif

    stat_re=0;
    stat_im=0;
    //    printf("PUCCH1A : Po_PUCCH before %d dB (%d)\n",dB_fixed(*Po_PUCCH),*Po_PUCCH);
    *Po_PUCCH = ((*Po_PUCCH>>1) + ((stat_max)>>1));
    *Po_PUCCH_dBm = dB_fixed(*Po_PUCCH/frame_parms->N_RB_UL) - eNB->rx_total_gain_dB;
    *Po_PUCCH_update = 1;
    /*
    printf("PUCCH1A : stat_max %d (%d,%d,%d) => Po_PUCCH %d\n",
	   dB_fixed(stat_max),
	   pucch1_thres+sigma2_dB,
	   pucch1_thres,
	   sigma2_dB,
	   dB_fixed(*Po_PUCCH));
    */
    // Do detection now
    if (sigma2_dB<(dB_fixed(stat_max)-pucch1_thres))  {//


      *Po_PUCCH = ((*Po_PUCCH*1023) + stat_max)>>10;

      chL = (nsymb>>1)-4;

      for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (re=0; re<12; re++) {
          chest_re=0;
          chest_im=0;
          cfo =  (frame_parms->Ncp==0) ? &cfo_pucch_np[14*phase_max] : &cfo_pucch_ep[12*phase_max];

          // channel estimate for first slot
          for (l=2; l<(nsymb>>1)-2; l++) {
            off=(re<<1) + (24*l);
            chest_re += (((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15))/chL;
	    chest_im += (((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15))/chL;
          }

#ifdef DEBUG_PUCCH_RX
          printf("[eNB] PUCCH subframe %d l %d re %d chest1 => (%d,%d)\n",subframe,l,re,
                chest_re,chest_im);
#endif

          for (l=0; l<2; l++) {
            off=(re<<1) + (24*l);
            tmp_re = ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
            tmp_im = ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            stat_re += (((tmp_re*chest_re)>>15) + ((tmp_im*chest_im)>>15))/4;
            stat_im += (((tmp_re*chest_im)>>15) - ((tmp_im*chest_re)>>15))/4;
            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d)\n",subframe,l,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l<<1],cfo[1+(l<<1)],
                  stat_re,stat_im);
#endif
          }

          for (l=(nsymb>>1)-2; l<(nsymb>>1); l++) {
            off=(re<<1) + (24*l);
            tmp_re = ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
            tmp_im = ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            stat_re += (((tmp_re*chest_re)>>15) + ((tmp_im*chest_im)>>15)/4);
            stat_im += (((tmp_re*chest_im)>>15) - ((tmp_im*chest_re)>>15)/4);
            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d)\n",subframe,l,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l<<1],cfo[1+(l<<1)],
                  stat_re,stat_im);
#endif
          }

          chest_re=0;
          chest_im=0;

          // channel estimate for second slot
          for (l=2; l<(nsymb>>1)-2; l++) {
            off=(re<<1) + (24*l) + (nsymb>>1)*24;
            chest_re += (((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15))/chL;
	    chest_im += (((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15))/chL;
          }

#ifdef DEBUG_PUCCH_RX
          printf("[eNB] PUCCH subframe %d l %d re %d chest2 => (%d,%d)\n",subframe,l,re,
                chest_re,chest_im);
#endif

          for (l=0; l<2; l++) {
            off=(re<<1) + (24*l) + (nsymb>>1)*24;
            tmp_re = ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
            tmp_im = ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            stat_re += (((tmp_re*chest_re)>>15) + ((tmp_im*chest_im)>>15))/4;
            stat_im += (((tmp_re*chest_im)>>15) - ((tmp_im*chest_re)>>15))/4;
            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[PHY][eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d)\n",subframe,l,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l<<1],cfo[1+(l<<1)],
                  stat_re,stat_im);
#endif
          }

          for (l=(nsymb>>1)-2; l<(nsymb>>1)-1; l++) {
            off=(re<<1) + (24*l) + (nsymb>>1)*24;
            tmp_re = ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
            tmp_im = ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            stat_re += (((tmp_re*chest_re)>>15) + ((tmp_im*chest_im)>>15))/4;
            stat_im += (((tmp_re*chest_im)>>15) - ((tmp_im*chest_re)>>15))/4;
            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[PHY][eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d)\n",subframe,l,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l<<1],cfo[1+(l<<1)],
                  stat_re,stat_im);
#endif
          }

#ifdef DEBUG_PUCCH_RX
          printf("aa%d re %d : stat %d,%d\n",aa,re,stat_re,stat_im);
#endif

        } //re
      } // aa

#ifdef DEBUG_PUCCH_RX
      LOG_I(PHY,"PUCCH 1a/b: subframe %d : stat %d,%d (pos %d)\n",subframe,stat_re,stat_im,
	    (subframe<<10) + (eNB->pucch1ab_stats_cnt[UE_id][subframe]));
#endif

	eNB->pucch1ab_stats[UE_id][(subframe<<11) + 2*(eNB->pucch1ab_stats_cnt[UE_id][subframe])] = (stat_re);
	eNB->pucch1ab_stats[UE_id][(subframe<<11) + 1+2*(eNB->pucch1ab_stats_cnt[UE_id][subframe])] = (stat_im);
	eNB->pucch1ab_stats_cnt[UE_id][subframe] = (eNB->pucch1ab_stats_cnt[UE_id][subframe]+1)&1023;

      /* frame not available here - set to -1 for the moment */
      T(T_ENB_PHY_PUCCH_1AB_IQ, T_INT(eNB->Mod_id), T_INT(UE_id), T_INT(-1), T_INT(subframe), T_INT(stat_re), T_INT(stat_im));

	  
      *payload = (stat_re<0) ? 1 : 0;

      if (fmt==pucch_format1b)
        *(1+payload) = (stat_im<0) ? 1 : 0;
    } else { // insufficient energy on PUCCH so NAK
      *payload = 0;
      ((int16_t*)&eNB->pucch1ab_stats[UE_id][(subframe<<10) + (eNB->pucch1ab_stats_cnt[UE_id][subframe])])[0] = (int16_t)(stat_re);
      ((int16_t*)&eNB->pucch1ab_stats[UE_id][(subframe<<10) + (eNB->pucch1ab_stats_cnt[UE_id][subframe])])[1] = (int16_t)(stat_im);
      eNB->pucch1ab_stats_cnt[UE_id][subframe] = (eNB->pucch1ab_stats_cnt[UE_id][subframe]+1)&1023;

      if (fmt==pucch_format1b)
        *(1+payload) = 0;
    }
  } else {
    LOG_E(PHY,"[eNB] PUCCH fmt2/2a/2b not supported\n");
  }

  return((int32_t)stat_max);

}


int32_t rx_pucch_emul(PHY_VARS_eNB *eNB,
		      eNB_rxtx_proc_t *proc,
                      uint8_t UE_index,
                      PUCCH_FMT_t fmt,
                      uint8_t n1_pucch_sel,
                      uint8_t *payload)

{
  uint8_t UE_id;
  uint16_t rnti;
  int subframe = proc->subframe_rx;
  uint8_t CC_id = eNB->CC_id;

  rnti = eNB->ulsch[UE_index]->rnti;

  for (UE_id=0; UE_id<NB_UE_INST; UE_id++) {
    if (rnti == PHY_vars_UE_g[UE_id][CC_id]->pdcch_vars[0]->crnti)
      break;
  }

  if (UE_id==NB_UE_INST) {
    LOG_W(PHY,"rx_pucch_emul: Didn't find UE with rnti %x\n",rnti);
    return(-1);
  }

  if (fmt == pucch_format1) {
    payload[0] = PHY_vars_UE_g[UE_id][CC_id]->sr[subframe];
  } else if (fmt == pucch_format1a) {
    payload[0] = PHY_vars_UE_g[UE_id][CC_id]->pucch_payload[0];
  } else if (fmt == pucch_format1b) {
    payload[0] = PHY_vars_UE_g[UE_id][CC_id]->pucch_payload[0];
    payload[1] = PHY_vars_UE_g[UE_id][CC_id]->pucch_payload[1];
  } else
    LOG_E(PHY,"[eNB] Frame %d: Can't handle formats 2/2a/2b\n",proc->frame_rx);

  if (PHY_vars_UE_g[UE_id][CC_id]->pucch_sel[subframe] == n1_pucch_sel)
    return(99);
  else
    return(0);
}
