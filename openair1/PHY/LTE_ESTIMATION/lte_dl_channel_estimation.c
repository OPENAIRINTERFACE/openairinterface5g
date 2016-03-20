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
#ifdef USER_MODE
#include <string.h>
#endif
#include "defs.h"
#include "PHY/defs.h"
#include "filt96_32.h"
//#define DEBUG_CH

int lte_dl_channel_estimation(PHY_VARS_UE *phy_vars_ue,
                              uint8_t eNB_id,
                              uint8_t eNB_offset,
                              unsigned char Ns,
                              unsigned char p,
                              unsigned char l,
                              unsigned char symbol)
{



  int pilot[2][200] __attribute__((aligned(16)));
  unsigned char nu,aarx;
  unsigned short k;
  unsigned int rb,pilot_cnt;
  int16_t ch[2],*pil,*rxF,*dl_ch,*dl_ch_prev,*f,*f2,*fl,*f2l2,*fr,*f2r2,*f2_dc,*f_dc;
  int ch_offset,symbol_offset;
  //  unsigned int n;
  //  int i;

  uint16_t Nid_cell = (eNB_offset == 0) ? phy_vars_ue->lte_frame_parms.Nid_cell : phy_vars_ue->PHY_measurements.adj_cell_id[eNB_offset-1];

  uint8_t nushift,pilot1,pilot2,pilot3;
  int **dl_ch_estimates=phy_vars_ue->lte_ue_common_vars.dl_ch_estimates[eNB_offset];
  int **rxdataF=phy_vars_ue->lte_ue_common_vars.rxdataF;

  if (phy_vars_ue->lte_frame_parms.Ncp == 0) {  // normal prefix
    pilot1 = 4;
    pilot2 = 7;
    pilot3 = 11;
  } else { // extended prefix
    pilot1 = 3;
    pilot2 = 6;
    pilot3 = 9;
  }

  // recompute nushift with eNB_offset corresponding to adjacent eNB on which to perform channel estimation
  nushift =  Nid_cell%6;

  if ((p==0) && (l==0) )
    nu = 0;
  else if ((p==0) && (l>0))
    nu = 3;
  else if ((p==1) && (l==0))
    nu = 3;
  else if ((p==1) && (l>0))
    nu = 0;
  else {
    msg("lte_dl_channel_estimation: p %d, l %d -> ERROR\n",p,l);
    return(-1);
  }


  //ch_offset     = (l*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size));
  if (phy_vars_ue->high_speed_flag == 0) // use second channel estimate position for temporary storage
    ch_offset     = phy_vars_ue->lte_frame_parms.ofdm_symbol_size ;
  else
    ch_offset     = phy_vars_ue->lte_frame_parms.ofdm_symbol_size*symbol;

  symbol_offset = phy_vars_ue->lte_frame_parms.ofdm_symbol_size*symbol;

  k = (nu + nushift)%6;

#ifdef DEBUG_CH
  printf("Channel Estimation : eNB_offset %d cell_id %d ch_offset %d, OFDM size %d, Ncp=%d, l=%d, Ns=%d, k=%d\n",eNB_offset,Nid_cell,ch_offset,phy_vars_ue->lte_frame_parms.ofdm_symbol_size,
         phy_vars_ue->lte_frame_parms.Ncp,l,Ns,k);
#endif

  switch (k) {
  case 0 :
    f=filt24_0;  //for first pilot of RB, first half
    f2=filt24_2; //for second pilot of RB, first half
    fl=filt24_0; //for first pilot of leftmost RB
    f2l2=filt24_2;
    //    fr=filt24_2r; //for first pilot of rightmost RB
    fr=filt24_0r2; //for first pilot of rightmost RB
    //    f2r2=filt24_0r2;
    f2r2=filt24_2r;

    f_dc=filt24_0_dcr;
    f2_dc=filt24_2_dcl;

    break;

  case 1 :
    f=filt24_1;
    f2=filt24_3;
    fl=filt24_1l;
    f2l2=filt24_3l2;
    fr=filt24_1r2;
    f2r2=filt24_3r;
    f_dc=filt24_1_dcr;  //for first pilot of RB, first half
    f2_dc=filt24_3_dcl;  //for first pilot of RB, first half
    break;

  case 2 :
    f=filt24_2;
    f2=filt24_4;
    fl=filt24_2l;
    f2l2=filt24_4l2;
    fr=filt24_2r2;
    f2r2=filt24_4r;
    f_dc=filt24_2_dcr;  //for first pilot of RB, first half
    f2_dc=filt24_4_dcl;  //for first pilot of RB, first half
    break;

  case 3 :
    f=filt24_3;
    f2=filt24_5;
    fl=filt24_3l;
    f2l2=filt24_5l2;
    fr=filt24_3r2;
    f2r2=filt24_5r;
    f_dc=filt24_3_dcr;  //for first pilot of RB, first half
    f2_dc=filt24_5_dcl;  //for first pilot of RB, first half
    break;

  case 4 :
    f=filt24_4;
    f2=filt24_6;
    fl=filt24_4l;
    f2l2=filt24_6l2;
    fr=filt24_4r2;
    f2r2=filt24_6r;
    f_dc=filt24_4_dcr;  //for first pilot of RB, first half
    f2_dc=filt24_6_dcl;  //for first pilot of RB, first half
    break;

  case 5 :
    f=filt24_5;
    f2=filt24_7;
    fl=filt24_5l;
    f2l2=filt24_7l2;
    fr=filt24_5r2;
    f2r2=filt24_7r;
    f_dc=filt24_5_dcr;  //for first pilot of RB, first half
    f2_dc=filt24_7_dcl;  //for first pilot of RB, first half
    break;

  default:
    msg("lte_dl_channel_estimation: k=%d -> ERROR\n",k);
    return(-1);
    break;
  }



  // generate pilot
  lte_dl_cell_spec_rx(phy_vars_ue,
                      eNB_offset,
                      &pilot[p][0],
                      Ns,
                      (l==0)?0:1,
                      p);


  for (aarx=0; aarx<phy_vars_ue->lte_frame_parms.nb_antennas_rx; aarx++) {

    pil   = (int16_t *)&pilot[p][0];
    rxF   = (int16_t *)&rxdataF[aarx][((symbol_offset+k+phy_vars_ue->lte_frame_parms.first_carrier_offset))];
    dl_ch = (int16_t *)&dl_ch_estimates[(p<<1)+aarx][ch_offset];


    //    if (eNb_id==0)
    memset(dl_ch,0,4*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size));
    if (phy_vars_ue->high_speed_flag==0) // multiply previous channel estimate by ch_est_alpha
      multadd_complex_vector_real_scalar(dl_ch-(phy_vars_ue->lte_frame_parms.ofdm_symbol_size<<1),
                                         phy_vars_ue->ch_est_alpha,dl_ch-(phy_vars_ue->lte_frame_parms.ofdm_symbol_size<<1),
                                         1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
#ifdef DEBUG_CH
    printf("k %d, first_carrier %d\n",k,phy_vars_ue->lte_frame_parms.first_carrier_offset);
#endif
    if ((phy_vars_ue->lte_frame_parms.N_RB_DL==6)  ||
        (phy_vars_ue->lte_frame_parms.N_RB_DL==50) ||
        (phy_vars_ue->lte_frame_parms.N_RB_DL==100)) {

      //First half of pilots
      // Treat first 2 pilots specially (left edge)
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
      printf("pilot 0 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fl,
                                         ch,
                                         dl_ch,
                                         24);
      pil+=2;    // Re Im
      rxF+=12;
      dl_ch+=8;

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
      printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(f2l2,
                                         ch,
                                         dl_ch,
                                         24);
      pil+=2;
      rxF+=12;
      dl_ch+=16;

      for (pilot_cnt=2; pilot_cnt<((phy_vars_ue->lte_frame_parms.N_RB_DL)-1); pilot_cnt+=2) {



        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15); //Re
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15); //Im
#ifdef DEBUG_CH
	printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
        multadd_real_vector_complex_scalar(f,
                                           ch,
                                           dl_ch,
                                           24);


        pil+=2;    // Re Im
        rxF+=12;
        dl_ch+=8;

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
	printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
        multadd_real_vector_complex_scalar(f2,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;
        rxF+=12;
        dl_ch+=16;

      }

      //       printf("Second half\n");
      // Second half of RBs

      k = (nu + nushift)%6;

      if (k > 6)
        k -=6;

      rxF   = (int16_t *)&rxdataF[aarx][((symbol_offset+1+k))];

#ifdef DEBUG_CH
      printf("second half k %d\n",k);
#endif
      for (pilot_cnt=0; pilot_cnt<((phy_vars_ue->lte_frame_parms.N_RB_DL)-3); pilot_cnt+=2) {


        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
	printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
        multadd_real_vector_complex_scalar(f,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;
        rxF+=12;
        dl_ch+=8;

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
	printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
        multadd_real_vector_complex_scalar(f2,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;
        rxF+=12;
        dl_ch+=16;

      }

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
      printf("pilot %d: rxF -> (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fr,
                                         ch,
                                         dl_ch,
                                         24);
      pil+=2;    // Re Im
      rxF+=12;
      dl_ch+=8;

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
      printf("pilot %d: rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(f2r2,
                                         ch,
                                         dl_ch,
                                         24);


    }

    else if (phy_vars_ue->lte_frame_parms.N_RB_DL==25) {
      //printf("Channel estimation\n");

      // Treat first 2 pilots specially (left edge)
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_CH
      printf("pilot 0 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);

      //      ch[0] = 1024;
      //      ch[1] = -128;
#endif

      multadd_real_vector_complex_scalar(fl,
                                         ch,
                                         dl_ch,
                                         24);
      pil+=2;    // Re Im
      rxF+=12;
      dl_ch+=8;

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_CH
      printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);

      //      ch[0] = 1024;
      //      ch[1] = -128;
#endif

      multadd_real_vector_complex_scalar(f2l2,
                                         ch,
                                         dl_ch,
                                         24);
      pil+=2;
      rxF+=12;
      dl_ch+=16;

      for (pilot_cnt=2; pilot_cnt<24; pilot_cnt+=2) {

        // printf("pilot[%d][%d] (%d,%d)\n",p,rb,pil[0],pil[1]);
        // printf("rx[%d][%d] -> (%d,%d)\n",p,phy_vars_ue->lte_frame_parms.first_carrier_offset + phy_vars_ue->lte_frame_parms.nushift + 6*rb+(3*p),rxF[0],rxF[1]);

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_CH
        printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);

        //  ch[0] = 1024;
        //  ch[1] = -128;
#endif


        multadd_real_vector_complex_scalar(f,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;    // Re Im
        rxF+=12;
        dl_ch+=8;

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_CH
        printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);

        //  ch[0] = 1024;
        //  ch[1] = -128;
#endif
        multadd_real_vector_complex_scalar(f2,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;
        rxF+=12;
        dl_ch+=16;

      }

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
      printf("pilot 24: rxF -> (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);

      //      ch[0] = 1024;
      //      ch[1] = -128;
#endif


      multadd_real_vector_complex_scalar(f_dc,
                                         ch,
                                         dl_ch,
                                         24);
      pil+=2;    // Re Im
      dl_ch+=8;

      // printf("Second half\n");
      // Second half of RBs
      rxF   = (int16_t *)&rxdataF[aarx][((symbol_offset+1+k))];

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
      printf("pilot 25: rxF -> (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);

      //      ch[0] = 1024;
      //      ch[1] = -128;
#endif

      multadd_real_vector_complex_scalar(f2_dc,
                                         ch,
                                         dl_ch,
                                         24);
      pil+=2;
      rxF+=12;
      dl_ch+=16;

      for (pilot_cnt=0; pilot_cnt<22; pilot_cnt+=2) {

        // printf("* pilot[%d][%d] (%d,%d)\n",p,rb,pil[0],pil[1]);
        // printf("rx[%d][%d] -> (%d,%d)\n",p,phy_vars_ue->lte_frame_parms.first_carrier_offset + phy_vars_ue->lte_frame_parms.nushift + 6*rb+(3*p),rxF[0],rxF[1]);

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
        printf("pilot %d rxF -> (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",26+pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);

        //  ch[0] = 1024;
        //  ch[1] = -128;
#endif

        multadd_real_vector_complex_scalar(f,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;
        rxF+=12;
        dl_ch+=8;

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_CH
        printf("pilot %d : rxF -> (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",27+pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);

        //  ch[0] = 1024;
        //  ch[1] = -128;
#endif

        multadd_real_vector_complex_scalar(f2,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;
        rxF+=12;
        dl_ch+=16;

      }

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_CH
      printf("pilot 49: rxF -> (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);

      //      ch[0] = 1024;
      //      ch[1] = -128;
#endif


      multadd_real_vector_complex_scalar(fr,
                                         ch,
                                         dl_ch,
                                         24);
      pil+=2;    // Re Im
      rxF+=12;
      dl_ch+=8;

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_CH

      printf("pilot 50: rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);

      //      ch[0] = 1024;
      //      ch[1] = -128;
#endif

      multadd_real_vector_complex_scalar(f2r2,
                                         ch,
                                         dl_ch,
                                         24);

    } else if (phy_vars_ue->lte_frame_parms.N_RB_DL==15) {

      //printf("First Half\n");
      for (rb=0; rb<28; rb+=4) {

        //printf("aarx=%d\n",aarx);
        //printf("pilot[%d][%d] (%d,%d)\n",p,rb,pil[0],pil[1]);
        //printf("rx[%d][%d] -> (%d,%d)\n",p,
        //       phy_vars_ue->lte_frame_parms.first_carrier_offset + phy_vars_ue->lte_frame_parms.nushift + 6*rb+(3*p),
        //       rxF[0],
        //       rxF[1]);
        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
        //printf("ch -> (%d,%d)\n",ch[0],ch[1]);
        multadd_real_vector_complex_scalar(f,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;    // Re Im
        rxF+=12;
        dl_ch+=8;

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
        //printf("ch -> (%d,%d)\n",ch[0],ch[1]);
        multadd_real_vector_complex_scalar(f2,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;
        rxF+=12;
        dl_ch+=16;

      }

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
      //     printf("ch -> (%d,%d)\n",ch[0],ch[1]);
      multadd_real_vector_complex_scalar(f,
                                         ch,
                                         dl_ch,
                                         24);
      pil+=2;    // Re Im
      dl_ch+=8;

      //printf("Second half\n");
      //Second half of RBs
      rxF   = (int16_t *)&rxdataF[aarx][((symbol_offset+1+nushift + (3*p)))];
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

      multadd_real_vector_complex_scalar(f2,
                                         ch,
                                         dl_ch,
                                         24);
      pil+=2;
      rxF+=12;
      dl_ch+=16;

      for (rb=0; rb<28; rb+=4) {
        //printf("aarx=%d\n",aarx);
        //printf("pilot[%d][%d] (%d,%d)\n",p,rb,pil[0],pil[1]);
        //printf("rx[%d][%d] -> (%d,%d)\n",p,
        //       phy_vars_ue->lte_frame_parms.first_carrier_offset + phy_vars_ue->lte_frame_parms.nushift + 6*rb+(3*p),
        //       rxF[0],
        //       rxF[1]);
        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

        multadd_real_vector_complex_scalar(f,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;
        rxF+=12;
        dl_ch+=8;

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

        multadd_real_vector_complex_scalar(f2,
                                           ch,
                                           dl_ch,
                                           24);
        pil+=2;
        rxF+=12;
        dl_ch+=16;

      }
    } else {
      msg("channel estimation not implemented for phy_vars_ue->lte_frame_parms.N_RB_DL = %d\n",phy_vars_ue->lte_frame_parms.N_RB_DL);
    }


    if (phy_vars_ue->perfect_ce == 0) {
      // Temporal Interpolation
      // printf("ch_offset %d\n",ch_offset);

      dl_ch = (int16_t *)&dl_ch_estimates[(p<<1)+aarx][ch_offset];

      if (phy_vars_ue->high_speed_flag == 0) {
        multadd_complex_vector_real_scalar(dl_ch,
                                           32767-phy_vars_ue->ch_est_alpha,
                                           dl_ch-(phy_vars_ue->lte_frame_parms.ofdm_symbol_size<<1),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      } else { // high_speed_flag == 1
        if (symbol == 0) {
          //      printf("Interpolating %d->0\n",4-phy_vars_ue->lte_frame_parms.Ncp);
          //      dl_ch_prev = (int16_t *)&dl_ch_estimates[(p<<1)+aarx][(4-phy_vars_ue->lte_frame_parms.Ncp)*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)];
          dl_ch_prev = (int16_t *)&dl_ch_estimates[(p<<1)+aarx][pilot3*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)];

          multadd_complex_vector_real_scalar(dl_ch_prev,21845,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
          multadd_complex_vector_real_scalar(dl_ch,10923,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);

          multadd_complex_vector_real_scalar(dl_ch_prev,10923,dl_ch_prev+(2*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
          multadd_complex_vector_real_scalar(dl_ch,21845,dl_ch_prev+(2*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
        } // this is 1/3,2/3 combination for pilots spaced by 3 symbols
        else if (symbol == pilot1) {
          dl_ch_prev = (int16_t *)&dl_ch_estimates[(p<<1)+aarx][0];

          if (phy_vars_ue->lte_frame_parms.Ncp==0) {// pilot spacing 4 symbols (1/4,1/2,3/4 combination)
            multadd_complex_vector_real_scalar(dl_ch_prev,24576,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_ch,8192,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);

            multadd_complex_vector_real_scalar(dl_ch_prev,16384,dl_ch_prev+(2*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_ch,16384,dl_ch_prev+(2*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);

            multadd_complex_vector_real_scalar(dl_ch_prev,8192,dl_ch_prev+(3*2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_ch,24576,dl_ch_prev+(3*2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
          } else {
            multadd_complex_vector_real_scalar(dl_ch_prev,10923,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_ch,21845,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);

            multadd_complex_vector_real_scalar(dl_ch_prev,21845,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_ch,10923,dl_ch_prev+(2*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
          } // pilot spacing 3 symbols (1/3,2/3 combination)
        } else if (symbol == pilot2) {
          dl_ch_prev = (int16_t *)&dl_ch_estimates[(p<<1)+aarx][pilot1*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)];

          multadd_complex_vector_real_scalar(dl_ch_prev,21845,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
          multadd_complex_vector_real_scalar(dl_ch,10923,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);

          multadd_complex_vector_real_scalar(dl_ch_prev,10923,dl_ch_prev+(2*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
          multadd_complex_vector_real_scalar(dl_ch,21845,dl_ch_prev+(2*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
        } else { // symbol == pilot3
          //      printf("Interpolating 0->%d\n",4-phy_vars_ue->lte_frame_parms.Ncp);
          dl_ch_prev = (int16_t *)&dl_ch_estimates[(p<<1)+aarx][pilot2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)];

          if (phy_vars_ue->lte_frame_parms.Ncp==0) {// pilot spacing 4 symbols (1/4,1/2,3/4 combination)
            multadd_complex_vector_real_scalar(dl_ch_prev,24576,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_ch,8192,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);

            multadd_complex_vector_real_scalar(dl_ch_prev,16384,dl_ch_prev+(2*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_ch,16384,dl_ch_prev+(2*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);

            multadd_complex_vector_real_scalar(dl_ch_prev,8192,dl_ch_prev+(3*2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_ch,24576,dl_ch_prev+(3*2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
          } else {
            multadd_complex_vector_real_scalar(dl_ch_prev,10923,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_ch,21845,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);

            multadd_complex_vector_real_scalar(dl_ch_prev,21845,dl_ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_ch,10923,dl_ch_prev+(2*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size)<<1)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
          } // pilot spacing 3 symbols (1/3,2/3 combination)
        }

      }
    }
  }

  void (*idft)(int16_t *,int16_t *, int);

  switch (phy_vars_ue->lte_frame_parms.ofdm_symbol_size) {
  case 128:
    idft = idft128;
    break;

  case 256:
    idft = idft256;
    break;

  case 512:
    idft = idft512;
    break;

  case 1024:
    idft = idft1024;
    break;

  case 1536:
    idft = idft1536;
    break;

  case 2048:
    idft = idft2048;
    break;

  default:
    idft = idft512;
    break;
  }

  // do ifft of channel estimate
  for (aarx=0; aarx<phy_vars_ue->lte_frame_parms.nb_antennas_rx; aarx++)
    for (p=0; p<phy_vars_ue->lte_frame_parms.nb_antennas_tx_eNB; p++) {
      if (phy_vars_ue->lte_ue_common_vars.dl_ch_estimates[eNB_offset][(p<<1)+aarx])
        idft((int16_t*) &phy_vars_ue->lte_ue_common_vars.dl_ch_estimates[eNB_offset][(p<<1)+aarx][8],
             (int16_t*) phy_vars_ue->lte_ue_common_vars.dl_ch_estimates_time[eNB_offset][(p<<1)+aarx],1);
    }

  return(0);
}

