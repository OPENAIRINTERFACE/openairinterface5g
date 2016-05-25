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

//#define DEBUG_CH
int lte_dl_mbsfn_channel_estimation(PHY_VARS_UE *phy_vars_ue,
                                    uint8_t eNB_id,
                                    uint8_t eNB_offset,
                                    int subframe,
                                    unsigned char l)
{



  int pilot[600] __attribute__((aligned(16)));
  unsigned char aarx,aa;
  unsigned int rb;
  short *pil,*rxF,*ch,*ch0,*ch1,*ch11,*chp,*ch_prev;
  int ch_offset,symbol_offset;

  //  unsigned int n;
  //  int i;

  int **dl_ch_estimates=phy_vars_ue->lte_ue_common_vars.dl_ch_estimates[0];
  int **rxdataF=phy_vars_ue->lte_ue_common_vars.rxdataF;

  ch_offset     = (l*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size));
  symbol_offset = ch_offset;//phy_vars_ue->lte_frame_parms.ofdm_symbol_size*l;

  //  printf("dl_mbsfn_channel_estimation.c:  symbol %d (%d)\n",l,ch_offset);
  // m=phy_vars_ue->lte_frame_parms.N_RB_DL*6;
  /*
      if ((l==2) || (l==10))
      k = 0;//m<<1;
      else if (l==6)
      k = 1;//+(m<<1);
      else {
      msg("lte_dl_mbsfn: l %d -> ERROR\n",l);
      return(-1);
      }
  */
  for (aarx=0; aarx<phy_vars_ue->lte_frame_parms.nb_antennas_rx; aarx++) {
    // generate pilot
    if ((l==2)||(l==6)||(l==10)) {
      lte_dl_mbsfn_rx(phy_vars_ue,
                      &pilot[0],
                      subframe,
                      l>>2);
    } // if symbol==2, return 0 else if symbol = 6, return 1, else if symbol=10 return 2



    pil   = (short *)&pilot[0];
    rxF   = (short *)&rxdataF[aarx][((symbol_offset+phy_vars_ue->lte_frame_parms.first_carrier_offset))];
    ch = (short *)&dl_ch_estimates[aarx][ch_offset];

    //    if (eNb_id==0)
    memset(ch,0,4*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size));

    //***********************************************************************
    if ((phy_vars_ue->lte_frame_parms.N_RB_DL==6)  ||
        (phy_vars_ue->lte_frame_parms.N_RB_DL==50) ||
        (phy_vars_ue->lte_frame_parms.N_RB_DL==100)) {

      // Interpolation  and extrapolation;
      if (l==6) {
        // ________________________First half of RBs____________________
        ch+=2;
        rxF+=2;
      }

      for (rb=0; rb<phy_vars_ue->lte_frame_parms.N_RB_DL; rb++) {
        // ------------------------1st pilot------------------------

        if (rb==(phy_vars_ue->lte_frame_parms.N_RB_DL>>1)) {
          rxF = (short *)&rxdataF[aarx][symbol_offset+1];

          if (l==6)
            rxF+=2;
        }

        ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
        ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
        ch[2] = ch[0]>>1;
        ch[3] = ch[1]>>1;

        /*
          printf("rb %d: pil0 (%d,%d) x (%d,%d) = (%d,%d)\n",
          rb,pil[0],pil[1],rxF[0],rxF[1],ch[0],ch[1]);*/
        if ((rb>0)&&(rb!=(phy_vars_ue->lte_frame_parms.N_RB_DL>>1))) {
          ch[-2] += ch[2];
          ch[-1] += ch[3];
        } else {
          ch[-2]= ch[0];
          ch[-1]= ch[1];
        }

        // ------------------------2nd pilot------------------------
        ch[4] = (short)(((int)pil[2]*rxF[4] - (int)pil[3]*rxF[5])>>15);
        ch[5] = (short)(((int)pil[2]*rxF[5] + (int)pil[3]*rxF[4])>>15);
        ch[6] = ch[4]>>1;
        ch[7] = ch[5]>>1;
        ch[2] += ch[6];
        ch[3] += ch[7];
        /*    printf("rb %d: pil1 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[2],pil[3],rxF[4],rxF[5],ch[4],ch[5]);*/
        // ------------------------3rd pilot------------------------
        ch[8] = (short)(((int)pil[4]*rxF[8] - (int)pil[5]*rxF[9])>>15);
        ch[9] = (short)(((int)pil[4]*rxF[9] + (int)pil[5]*rxF[8])>>15);
        ch[10] = ch[8]>>1;
        ch[11] = ch[9]>>1;
        ch[6] += ch[10];
        ch[7] += ch[11];
        /*    printf("rb %d: pil2 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[4],pil[5],rxF[8],rxF[9],ch[8],ch[9]);*/
        // ------------------------4th pilot------------------------
        ch[12] = (short)(((int)pil[6]*rxF[12] - (int)pil[7]*rxF[13])>>15);
        ch[13] = (short)(((int)pil[6]*rxF[13] + (int)pil[7]*rxF[12])>>15);
        ch[14] = ch[12]>>1;
        ch[15] = ch[13]>>1;
        ch[10] += ch[14];
        ch[11] += ch[15];
        /*    printf("rb %d: pil3 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[6],pil[7],rxF[12],rxF[13],ch[12],ch[13]);*/
        // ------------------------5th pilot------------------------
        ch[16] = (short)(((int)pil[8]*rxF[16] - (int)pil[9]*rxF[17])>>15);
        ch[17] = (short)(((int)pil[8]*rxF[17] + (int)pil[9]*rxF[16])>>15);
        ch[18] = ch[16]>>1;
        ch[19] = ch[17]>>1;
        ch[14] += ch[18];
        ch[15] += ch[19];
        /*    printf("rb %d: pil4 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[4],pil[5],rxF[16],rxF[17],ch[16],ch[17]);*/
        // ------------------------6th pilot------------------------
        ch[20] = (short)(((int)pil[10]*rxF[20] - (int)pil[11]*rxF[21])>>15);
        ch[21] = (short)(((int)pil[10]*rxF[21] + (int)pil[11]*rxF[20])>>15);

        if ((rb<(phy_vars_ue->lte_frame_parms.N_RB_DL-1))&&
            (rb!=((phy_vars_ue->lte_frame_parms.N_RB_DL>>1)-1))) {
          ch[22] = ch[20]>>1;
          ch[23] = ch[21]>>1;
          ch[18] += ch[22];
          ch[19] += ch[23];
        } else {
          ch[22] = ch[20];
          ch[23] = ch[21];
          ch[18] += (ch[22]>>1);
          ch[19] += (ch[23]>>1);
        }

        /*    printf("rb %d: pil5 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[10],pil[11],rxF[20],rxF[21],ch[20],ch[21]);
            printf("ch11 (%d,%d)\n",ch[22],ch[23]);*/
        pil+=12;
        ch+=24;
        rxF+=24;
      }
    }

    //*********************************************************************
    else if (phy_vars_ue->lte_frame_parms.N_RB_DL==25) {
      //printf("Channel estimation\n");
      //------------------------ loop over first 12 RBs------------------------
      if (l==6) {
        // ________________________First half of RBs____________________
        ch+=2;
        rxF+=2;

        for (rb=0; rb<12; rb++) {
          // ------------------------1st pilot------------------------

          ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
          ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
          ch[2] = ch[0]>>1;
          ch[3] = ch[1]>>1;

          /*
          printf("rb %d: pil0 (%d,%d) x (%d,%d) = (%d,%d)\n",
          rb,pil[0],pil[1],rxF[0],rxF[1],ch[0],ch[1]);*/
          if (rb>0) {
            ch[-2] += ch[2];
            ch[-1] += ch[3];
          } else {
            ch[-2]= ch[0];
            ch[-1]= ch[1];
            // ch[-2]= (ch[0]>>1)*3- ch[4];
            // ch[-1]= (ch[1]>>1)*3- ch[5];
          }

          // ------------------------2nd pilot------------------------
          ch[4] = (short)(((int)pil[2]*rxF[4] - (int)pil[3]*rxF[5])>>15);
          ch[5] = (short)(((int)pil[2]*rxF[5] + (int)pil[3]*rxF[4])>>15);
          ch[6] = ch[4]>>1;
          ch[7] = ch[5]>>1;
          ch[2] += ch[6];
          ch[3] += ch[7];
          /*    printf("rb %d: pil1 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[2],pil[3],rxF[4],rxF[5],ch[4],ch[5]);*/
          // ------------------------3rd pilot------------------------
          ch[8] = (short)(((int)pil[4]*rxF[8] - (int)pil[5]*rxF[9])>>15);
          ch[9] = (short)(((int)pil[4]*rxF[9] + (int)pil[5]*rxF[8])>>15);
          ch[10] = ch[8]>>1;
          ch[11] = ch[9]>>1;
          ch[6] += ch[10];
          ch[7] += ch[11];
          /*    printf("rb %d: pil2 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[4],pil[5],rxF[8],rxF[9],ch[8],ch[9]);*/
          // ------------------------4th pilot------------------------
          ch[12] = (short)(((int)pil[6]*rxF[12] - (int)pil[7]*rxF[13])>>15);
          ch[13] = (short)(((int)pil[6]*rxF[13] + (int)pil[7]*rxF[12])>>15);
          ch[14] = ch[12]>>1;
          ch[15] = ch[13]>>1;
          ch[10] += ch[14];
          ch[11] += ch[15];
          /*    printf("rb %d: pil3 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[6],pil[7],rxF[12],rxF[13],ch[12],ch[13]);*/
          // ------------------------5th pilot------------------------
          ch[16] = (short)(((int)pil[8]*rxF[16] - (int)pil[9]*rxF[17])>>15);
          ch[17] = (short)(((int)pil[8]*rxF[17] + (int)pil[9]*rxF[16])>>15);
          ch[18] = ch[16]>>1;
          ch[19] = ch[17]>>1;
          ch[14] += ch[18];
          ch[15] += ch[19];
          /*    printf("rb %d: pil4 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[4],pil[5],rxF[16],rxF[17],ch[16],ch[17]);*/
          // ------------------------6th pilot------------------------
          ch[20] = (short)(((int)pil[10]*rxF[20] - (int)pil[11]*rxF[21])>>15);
          ch[21] = (short)(((int)pil[10]*rxF[21] + (int)pil[11]*rxF[20])>>15);
          ch[22] = ch[20]>>1;
          ch[23] = ch[21]>>1;
          ch[18] += ch[22];
          ch[19] += ch[23];
          /*    printf("rb %d: pil5 (%d,%d) x (%d,%d) = (%d,%d)\n",
           rb,pil[10],pil[11],rxF[20],rxF[21],ch[20],ch[21]);
           printf("ch11 (%d,%d)\n",ch[22],ch[23]);*/
          pil+=12;
          ch+=24;
          rxF+=24;
        }

        // Middle RB
        // ________________________First half of RB___________________
        // ------------------------1st pilot---------------------------
        chp = ch-24;
        ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
        ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
        ch[2] = ch[0]>>1;
        ch[3] = ch[1]>>1;

        chp[22] += ch[2];
        chp[23] += ch[3];
        /*  printf("rb %d: pil0 (%d,%d) x (%d,%d) = (%d,%d)\n",
          rb,pil[0],pil[1],rxF[0],rxF[1],ch[0],ch[1]);*/
        //printf("interp0: chp[0] (%d,%d) ch23 (%d,%d)\n",chp[22],chp[23],ch[2],ch[3]);
        // ------------------------2nd pilot------------------------
        ch[4] = (short)(((int)pil[2]*rxF[4] - (int)pil[3]*rxF[5])>>15);
        ch[5] = (short)(((int)pil[2]*rxF[5] + (int)pil[3]*rxF[4])>>15);
        ch[6] = ch[4]>>1;
        ch[7] = ch[5]>>1;
        ch[2] += ch[6];
        ch[3] += ch[7];
        /*  printf("rb %d: pil1 (%d,%d) x (%d,%d) = (%d,%d)\n",
               rb,pil[2],pil[3],rxF[4],rxF[5],ch[4],ch[5]);
               printf("interp1: ch23 (%d,%d) \n",ch[2],ch[3]);*/
        // ------------------------3rd pilot------------------------
        ch[8] = (short)(((int)pil[4]*rxF[8] - (int)pil[5]*rxF[9])>>15);
        ch[9] = (short)(((int)pil[4]*rxF[9] + (int)pil[5]*rxF[8])>>15);
        ch[10] = ch[8]>>1;
        ch[11] = ch[9]>>1;
        //ch[10] = (ch[8])/3;
        //ch[11] = (ch[9])/3;
        ch[6] += ch[10];
        ch[7] += ch[11];
        /*  printf("rb %d: pil2 (%d,%d) x (%d,%d) = (%d,%d)\n",
          rb,pil[4],pil[5],rxF[8],rxF[9],ch[8],ch[9]);*/
        // printf("Second half\n");

        // ________________________Second half of RB____________________

        rxF   = (short *)&rxdataF[aarx][((symbol_offset+2))];

        // 4th pilot
        ch[12] = (short)(((int)pil[6]*rxF[0] - (int)pil[7]*rxF[1])>>15);
        ch[13] = (short)(((int)pil[6]*rxF[1] + (int)pil[7]*rxF[0])>>15);
        ch[14] = ch[12]>>1;
        ch[15] = ch[13]>>1;
        ch[10] = (ch[8]/3)+(ch[12]<<1)/3;
        ch[11] = (ch[9]/3)+(ch[13]<<1)/3;
        /*  printf("rb %d: pil3 (%d,%d) x (%d,%d) = (%d,%d)\n",
          rb,pil[6],pil[7],rxF[0],rxF[1],ch[12],ch[13]);*/
        // ------------------------5th pilot------------------------
        ch[16] = (short)(((int)pil[8]*rxF[4] - (int)pil[9]*rxF[5])>>15);
        ch[17] = (short)(((int)pil[8]*rxF[5] + (int)pil[9]*rxF[4])>>15);
        ch[18] = ch[16]>>1;
        ch[19] = ch[17]>>1;
        ch[14] += ch[18];
        ch[15] += ch[19];
        /*  printf("rb %d: pil4 (%d,%d) x (%d,%d) = (%d,%d)\n",
          rb,pil[4],pil[5],rxF[4],rxF[5],ch[16],ch[17]);*/
        // ------------------------6th pilot------------------------
        ch[20] = (short)(((int)pil[10]*rxF[8] - (int)pil[11]*rxF[9])>>15);
        ch[21] = (short)(((int)pil[10]*rxF[9] + (int)pil[11]*rxF[8])>>15);
        ch[22] = ch[20]>>1;
        ch[23] = ch[21]>>1;
        ch[18] += ch[22];
        ch[19] += ch[23];
        /*        printf("rb %d: pil5 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[10],pil[11],rxF[8],rxF[9],ch[20],ch[21]);*/
        pil+=12;
        ch+=24;
        rxF+=12;

        // ________________________Second half of RBs____________________
        for (rb=0; rb<11; rb++) {
          // ------------------------1st pilot---------------------------
          ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
          ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
          ch[2] = ch[0]>>1;
          ch[3] = ch[1]>>1;
          /*    printf("rb %d: pil0 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[0],pil[1],rxF[0],rxF[1],ch[0],ch[1]);*/
          //if (rb>0) {
          ch[-2] += ch[2];
          ch[-1] += ch[3];
          //}
          // ------------------------2nd pilot------------------------
          ch[4] = (short)(((int)pil[2]*rxF[4] - (int)pil[3]*rxF[5])>>15);
          ch[5] = (short)(((int)pil[2]*rxF[5] + (int)pil[3]*rxF[4])>>15);
          ch[6] = ch[4]>>1;
          ch[7] = ch[5]>>1;
          ch[2] += ch[6];
          ch[3] += ch[7];
          /*    printf("rb %d: pil1 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[2],pil[3],rxF[4],rxF[5],ch[4],ch[5]);*/
          // ------------------------3rd pilot------------------------
          ch[8] = (short)(((int)pil[4]*rxF[8] - (int)pil[5]*rxF[9])>>15);
          ch[9] = (short)(((int)pil[4]*rxF[9] + (int)pil[5]*rxF[8])>>15);
          ch[10] = ch[8]>>1;
          ch[11] = ch[9]>>1;
          ch[6] += ch[10];
          ch[7] += ch[11];
          /*    printf("rb %d: pil2 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[4],pil[5],rxF[8],rxF[9],ch[8],ch[9]); */
          // ------------------------4th pilot------------------------
          ch[12] = (short)(((int)pil[6]*rxF[12] - (int)pil[7]*rxF[13])>>15);
          ch[13] = (short)(((int)pil[6]*rxF[13] + (int)pil[7]*rxF[12])>>15);
          ch[14] = ch[12]>>1;
          ch[15] = ch[13]>>1;
          ch[10] += ch[14];
          ch[11] += ch[15];
          /*    printf("rb %d: pil3 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[6],pil[7],rxF[12],rxF[13],ch[12],ch[13]);*/
          // ------------------------5th pilot------------------------
          ch[16] = (short)(((int)pil[8]*rxF[16] - (int)pil[9]*rxF[17])>>15);
          ch[17] = (short)(((int)pil[8]*rxF[17] + (int)pil[9]*rxF[16])>>15);
          ch[18] = ch[16]>>1;
          ch[19] = ch[17]>>1;
          ch[14] += ch[18];
          ch[15] += ch[19];
          /*    printf("rb %d: pil4 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[4],pil[5],rxF[16],rxF[17],ch[16],ch[17]);*/
          // ------------------------6th pilot------------------------
          ch[20] = (short)(((int)pil[10]*rxF[20] - (int)pil[11]*rxF[21])>>15);
          ch[21] = (short)(((int)pil[10]*rxF[21] + (int)pil[11]*rxF[20])>>15);
          ch[22] = ch[20]>>1;
          ch[23] = ch[21]>>1;
          ch[18] += ch[22];
          ch[19] += ch[23];
          /*    printf("rb %d: pil5 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[10],pil[11],rxF[20],rxF[21],ch[20],ch[21]);*/
          pil+=12;
          ch+=24;
          rxF+=24;
        }

        // ------------------------Last PRB ---------------------------
        // ------------------------1st pilot---------------------------
        ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
        ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
        ch[2] = ch[0]>>1;
        ch[3] = ch[1]>>1;
        ch[-2] += ch[2];
        ch[-1] += ch[3];
        /*  printf("rb %d: pil0 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[0],pil[1],rxF[0],rxF[1],ch[0],ch[1]);*/
        // ------------------------2nd pilot------------------------
        ch[4] = (short)(((int)pil[2]*rxF[4] - (int)pil[3]*rxF[5])>>15);
        ch[5] = (short)(((int)pil[2]*rxF[5] + (int)pil[3]*rxF[4])>>15);
        ch[6] = ch[4]>>1;
        ch[7] = ch[5]>>1;
        ch[2] += ch[6];
        ch[3] += ch[7];
        /*  printf("rb %d: pil1 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[2],pil[3],rxF[4],rxF[5],ch[4],ch[5]);  */
        // ------------------------3rd pilot------------------------
        ch[8] = (short)(((int)pil[4]*rxF[8] - (int)pil[5]*rxF[9])>>15);
        ch[9] = (short)(((int)pil[4]*rxF[9] + (int)pil[5]*rxF[8])>>15);
        ch[10] = ch[8]>>1;
        ch[11] = ch[9]>>1;
        ch[6] += ch[10];
        ch[7] += ch[11];
        /*  printf("rb %d: pil2 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[4],pil[5],rxF[8],rxF[9],ch[8],ch[9]);*/
        // ------------------------4th pilot------------------------
        ch[12] = (short)(((int)pil[6]*rxF[12] - (int)pil[7]*rxF[13])>>15);
        ch[13] = (short)(((int)pil[6]*rxF[13] + (int)pil[7]*rxF[12])>>15);
        ch[14] = ch[12]>>1;
        ch[15] = ch[13]>>1;
        ch[10] += ch[14];
        ch[11] += ch[15];
        /*  printf("rb %d: pil3 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[6],pil[7],rxF[12],rxF[13],ch[12],ch[13]);*/
        // ------------------------5th pilot------------------------
        ch[16] = (short)(((int)pil[8]*rxF[16] - (int)pil[9]*rxF[17])>>15);
        ch[17] = (short)(((int)pil[8]*rxF[17] + (int)pil[9]*rxF[16])>>15);
        ch[18] = ch[16]>>1;
        ch[19] = ch[17]>>1;
        ch[14] += ch[18];
        ch[15] += ch[19];
        /*  printf("rb %d: pil4 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[4],pil[5],rxF[16],rxF[17],ch[16],ch[17]);*/
        // ------------------------6th pilot------------------------
        ch[20] = (short)(((int)pil[10]*rxF[20] - (int)pil[11]*rxF[21])>>15);
        ch[21] = (short)(((int)pil[10]*rxF[21] + (int)pil[11]*rxF[20])>>15);
        ch[18] += ch[20]>>1;
        ch[19] += ch[21]>>1;
        /*  printf("rb %d: pil5 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[10],pil[11],rxF[20],rxF[21],ch[20],ch[21]);     */
      }

      //**********************************************************************
      // for l=2 and l=10
      if (l!=6) {
        // extrapolate last channel estimate
        for (rb=0; rb<12; rb++) {
          // ------------------------1st pilot---------------------------
          ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
          ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
          ch[2] = ch[0]>>1;
          ch[3] = ch[1]>>1;
          ch[-2] += ch[2];
          ch[-1] += ch[3];
          /*    printf("rb %d: pil0 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[0],pil[1],rxF[0],rxF[1],ch[0],ch[1]);*/
          // ------------------------2nd pilot------------------------
          ch[4] = (short)(((int)pil[2]*rxF[4] - (int)pil[3]*rxF[5])>>15);
          ch[5] = (short)(((int)pil[2]*rxF[5] + (int)pil[3]*rxF[4])>>15);
          ch[6] = ch[4]>>1;
          ch[7] = ch[5]>>1;
          ch[2] += ch[6];
          ch[3] += ch[7];
          /*    printf("rb %d: pil1 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[2],pil[3],rxF[4],rxF[5],ch[4],ch[5]);*/
          // ------------------------3rd pilot------------------------
          ch[8] = (short)(((int)pil[4]*rxF[8] - (int)pil[5]*rxF[9])>>15);
          ch[9] = (short)(((int)pil[4]*rxF[9] + (int)pil[5]*rxF[8])>>15);
          ch[10] = ch[8]>>1;
          ch[11] = ch[9]>>1;
          ch[6] += ch[10];
          ch[7] += ch[11];
          /*    printf("rb %d: pil2 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[4],pil[5],rxF[8],rxF[9],ch[8],ch[9]);*/

          // ------------------------4th pilot------------------------
          ch[12] = (short)(((int)pil[6]*rxF[12] - (int)pil[7]*rxF[13])>>15);
          ch[13] = (short)(((int)pil[6]*rxF[13] + (int)pil[7]*rxF[12])>>15);
          ch[14] = ch[12]>>1;
          ch[15] = ch[13]>>1;
          ch[10] += ch[14];
          ch[11] += ch[15];
          /*    printf("rb %d: pil3 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[6],pil[7],rxF[12],rxF[13],ch[12],ch[13]);*/
          // ------------------------5th pilot------------------------
          ch[16] = (short)(((int)pil[8]*rxF[16] - (int)pil[9]*rxF[17])>>15);
          ch[17] = (short)(((int)pil[8]*rxF[17] + (int)pil[9]*rxF[16])>>15);
          ch[18] = ch[16]>>1;
          ch[19] = ch[17]>>1;
          ch[14] += ch[18];
          ch[15] += ch[19];
          /*    printf("rb %d: pil4 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[4],pil[5],rxF[32],rxF[33],ch[16],ch[17]);*/
          // ------------------------6th pilot------------------------
          ch[20] = (short)(((int)pil[10]*rxF[20] - (int)pil[11]*rxF[21])>>15);
          ch[21] = (short)(((int)pil[10]*rxF[21] + (int)pil[11]*rxF[20])>>15);
          ch[22] = ch[20]>>1;
          ch[23] = ch[21]>>1;
          ch[18] += ch[22];
          ch[19] += ch[23];
          /*    printf("rb %d: pil5 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[10],pil[11],rxF[20],rxF[21],ch[20],ch[21]);*/
          pil+=12;
          ch+=24;
          rxF+=24;
        }

        // ------------------------middle PRB--------------------------
        // ------------------------1st pilot---------------------------

        ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
        ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
        ch[2] = ch[0]>>1;
        ch[3] = ch[1]>>1;
        /*  printf("rb %d: pil0 (%d,%d) x (%d,%d) = (%d,%d)\n",
          rb,pil[0],pil[1],rxF[0],rxF[1],ch[0],ch[1]);*/
        ch[-2] += ch[2];
        ch[-1] += ch[3];
        // ------------------------2nd pilot------------------------
        ch[4] = (short)(((int)pil[2]*rxF[4] - (int)pil[3]*rxF[5])>>15);
        ch[5] = (short)(((int)pil[2]*rxF[5] + (int)pil[3]*rxF[4])>>15);
        ch[6] = ch[4]>>1;
        ch[7] = ch[5]>>1;
        ch[2] += ch[6];
        ch[3] += ch[7];
        /*        printf("rb %d: pil1 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[2],pil[3],rxF[4],rxF[5],ch[4],ch[5]);*/
        // ------------------------3rd pilot------------------------
        ch[8] = (short)(((int)pil[4]*rxF[8] - (int)pil[5]*rxF[9])>>15);
        ch[9] = (short)(((int)pil[4]*rxF[9] + (int)pil[5]*rxF[8])>>15);
        ch[10] = ch[8]>>1;
        ch[11] = ch[9]>>1;
        ch[6] += ch[10];
        ch[7] += ch[11];
        /*        printf("rb %d: pil2 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[4],pil[5],rxF[8],rxF[9],ch[8],ch[9]);*/
        // printf("Second half\n");

        // ------------------------Second half of RBs---------------------//
        rxF   = (short *)&rxdataF[aarx][((symbol_offset+1))];
        // ---------------------------------------------------------------//
        // ------------------------4th pilot------------------------
        ch[12] = (short)(((int)pil[6]*rxF[0] - (int)pil[7]*rxF[1])>>15);
        ch[13] = (short)(((int)pil[6]*rxF[1] + (int)pil[7]*rxF[0])>>15);
        ch[14] = ch[12]>>1;
        ch[15] = ch[13]>>1;
        ch[10] = (ch[12]/3)+(ch[8]<<1)/3;
        ch[11] = (ch[13]/3)+(ch[9]<<1)/3;
        /*        printf("rb %d: pil3 (%d,%d) x (%d,%d) = (%d,%d)\n",
            rb,pil[6],pil[7],rxF[0],rxF[1],ch[12],ch[13]);*/
        // ------------------------5th pilot------------------------
        ch[16] = (short)(((int)pil[8]*rxF[4] - (int)pil[9]*rxF[5])>>15);
        ch[17] = (short)(((int)pil[8]*rxF[5] + (int)pil[9]*rxF[4])>>15);
        ch[18] = ch[16]>>1;
        ch[19] = ch[17]>>1;
        ch[14] += ch[18];
        ch[15] += ch[19];
        /*  printf("rb %d: pil4 (%d,%d) x (%d,%d) = (%d,%d)\n",
          rb,pil[4],pil[5],rxF[4],rxF[5],ch[8],ch[17]);*/
        // ------------------------6th pilot------------------------
        ch[20] = (short)(((int)pil[10]*rxF[8] - (int)pil[11]*rxF[9])>>15);
        ch[21] = (short)(((int)pil[10]*rxF[9] + (int)pil[11]*rxF[8])>>15);
        ch[22] = ch[20]>>1;
        ch[23] = ch[21]>>1;
        ch[18] += ch[22];
        ch[19] += ch[23];
        /*  printf("rb %d: pil5 (%d,%d) x (%d,%d) = (%d,%d)\n",
          rb,pil[10],pil[11],rxF[8],rxF[9],ch[20],ch[21]);*/
        pil+=12;
        ch+=24;
        rxF+=12;

        // ________________________Second half of RBs____________________
        for (rb=0; rb<11; rb++) {
          // ------------------------1st pilot---------------------------
          ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
          ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
          ch[2] = ch[0]>>1;
          ch[3] = ch[1]>>1;
          ch[-2] += ch[2];
          ch[-1] += ch[3];

          /*    printf("rb %d: pil0 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[0],pil[1],rxF[0],rxF[1],ch[0],ch[1]);*/
          // ------------------------2nd pilot------------------------
          ch[4] = (short)(((int)pil[2]*rxF[4] - (int)pil[3]*rxF[5])>>15);
          ch[5] = (short)(((int)pil[2]*rxF[5] + (int)pil[3]*rxF[4])>>15);
          ch[6] = ch[4]>>1;
          ch[7] = ch[5]>>1;
          ch[2] += ch[6];
          ch[3] += ch[7];
          /*    printf("rb %d: pil1 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[2],pil[3],rxF[4],rxF[5],ch[4],ch[5]);  */
          // ------------------------3rd pilot------------------------
          ch[8] = (short)(((int)pil[4]*rxF[8] - (int)pil[5]*rxF[9])>>15);
          ch[9] = (short)(((int)pil[4]*rxF[9] + (int)pil[5]*rxF[8])>>15);
          ch[10] = ch[8]>>1;
          ch[11] = ch[9]>>1;
          ch[6] += ch[10];
          ch[7] += ch[11];
          /*    printf("rb %d: pil2 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[4],pil[5],rxF[8],rxF[9],ch[8],ch[9]);*/
          // ------------------------4th pilot------------------------
          ch[12] = (short)(((int)pil[6]*rxF[12] - (int)pil[7]*rxF[13])>>15);
          ch[13] = (short)(((int)pil[6]*rxF[13] + (int)pil[7]*rxF[12])>>15);
          ch[14] = ch[12]>>1;
          ch[15] = ch[13]>>1;
          ch[10] += ch[14];
          ch[11] += ch[15];
          /*    printf("rb %d: pil3 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[6],pil[7],rxF[12],rxF[13],ch[12],ch[13]);*/
          // ------------------------5th pilot------------------------
          ch[16] = (short)(((int)pil[8]*rxF[16] - (int)pil[9]*rxF[17])>>15);
          ch[17] = (short)(((int)pil[8]*rxF[17] + (int)pil[9]*rxF[16])>>15);
          ch[18] = ch[16]>>1;
          ch[19] = ch[17]>>1;
          ch[14] += ch[18];
          ch[15] += ch[19];
          /*    printf("rb %d: pil4 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[4],pil[5],rxF[16],rxF[17],ch[16],ch[17]);*/
          // ------------------------6th pilot------------------------
          ch[20] = (short)(((int)pil[10]*rxF[20] - (int)pil[11]*rxF[21])>>15);
          ch[21] = (short)(((int)pil[10]*rxF[21] + (int)pil[11]*rxF[20])>>15);
          ch[22] = ch[20]>>1;
          ch[23] = ch[21]>>1;
          ch[18] += ch[22];
          ch[19] += ch[23];
          /*    printf("rb %d: pil5 (%d,%d) x (%d,%d) = (%d,%d)\n",
            13+rb,pil[10],pil[11],rxF[20],rxF[21],ch[20],ch[21]);*/
          pil+=12;
          ch+=24;
          rxF+=24;
        }

        // ------------------------Last PRB ---------------------------
        // ------------------------1st pilot---------------------------
        ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
        ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
        ch[2] = ch[0]>>1;
        ch[3] = ch[1]>>1;
        ch[-2] += ch[2];
        ch[-1] += ch[3];
        /*  printf("rb %d: pil0 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[0],pil[1],rxF[0],rxF[1],ch[0],ch[1]);*/
        // ------------------------2nd pilot------------------------
        ch[4] = (short)(((int)pil[2]*rxF[4] - (int)pil[3]*rxF[5])>>15);
        ch[5] = (short)(((int)pil[2]*rxF[5] + (int)pil[3]*rxF[4])>>15);
        ch[6] = ch[4]>>1;
        ch[7] = ch[5]>>1;
        ch[2] += ch[6];
        ch[3] += ch[7];
        /*  printf("rb %d: pil1 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[2],pil[3],rxF[4],rxF[5],ch[4],ch[5]);  */
        // ------------------------3rd pilot------------------------
        ch[8] = (short)(((int)pil[4]*rxF[8] - (int)pil[5]*rxF[9])>>15);
        ch[9] = (short)(((int)pil[4]*rxF[9] + (int)pil[5]*rxF[8])>>15);
        ch[10] = ch[8]>>1;
        ch[11] = ch[9]>>1;
        ch[6] += ch[10];
        ch[7] += ch[11];
        /*  printf("rb %d: pil2 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[4],pil[5],rxF[8],rxF[9],ch[8],ch[9]);*/
        // ------------------------4th pilot------------------------
        ch[12] = (short)(((int)pil[6]*rxF[12] - (int)pil[7]*rxF[13])>>15);
        ch[13] = (short)(((int)pil[6]*rxF[13] + (int)pil[7]*rxF[12])>>15);
        ch[14] = ch[12]>>1;
        ch[15] = ch[13]>>1;
        ch[10] += ch[14];
        ch[11] += ch[15];
        /*  printf("rb %d: pil3 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[6],pil[7],rxF[12],rxF[13],ch[12],ch[13]);*/
        // ------------------------5th pilot------------------------
        ch[16] = (short)(((int)pil[8]*rxF[16] - (int)pil[9]*rxF[17])>>15);
        ch[17] = (short)(((int)pil[8]*rxF[17] + (int)pil[9]*rxF[16])>>15);
        ch[18] = ch[16]>>1;
        ch[19] = ch[17]>>1;
        ch[14] += ch[18];
        ch[15] += ch[19];
        /*  printf("rb %d: pil4 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[4],pil[5],rxF[16],rxF[17],ch[16],ch[17]);*/
        // ------------------------6th pilot------------------------
        ch[20] = (short)(((int)pil[10]*rxF[20] - (int)pil[11]*rxF[21])>>15);
        ch[21] = (short)(((int)pil[10]*rxF[21] + (int)pil[11]*rxF[20])>>15);
        ch[22]= (ch[20]>>1)*3- ch[18];
        ch[23]= (ch[21]>>1)*3- ch[19];
        ch[18] += ch[20]>>1;
        ch[19] += ch[21]>>1;
        /*  printf("rb %d: pil5 (%d,%d) x (%d,%d) = (%d,%d)\n",
          13+rb,pil[10],pil[11],rxF[20],rxF[21],ch[20],ch[21]);*/
      }
    }

    //------------------------Temporal Interpolation ------------------------------
    if (l==6) {
      ch = (short *)&dl_ch_estimates[aarx][ch_offset];
      //  printf("Interpolating ch 2,6 => %d\n",ch_offset);
      ch_prev = (short *)&dl_ch_estimates[aarx][2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)];
      ch0 = (short *)&dl_ch_estimates[aarx][0*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)];
      memcpy(ch0,ch_prev,4*phy_vars_ue->lte_frame_parms.ofdm_symbol_size);

      ch1 = (short *)&dl_ch_estimates[aarx][1*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)];
      memcpy(ch1,ch_prev,4*phy_vars_ue->lte_frame_parms.ofdm_symbol_size);

      // 3/4 ch2 + 1/4 ch6 => ch3
      multadd_complex_vector_real_scalar(ch_prev,24576,ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      multadd_complex_vector_real_scalar(ch,8192,ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      // 1/2 ch2 + 1/2 ch6 => ch4
      multadd_complex_vector_real_scalar(ch_prev,16384,ch_prev+(4*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      multadd_complex_vector_real_scalar(ch,16384,ch_prev+(4*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      // 1/4 ch2 + 3/4 ch6 => ch5
      multadd_complex_vector_real_scalar(ch_prev,8192,ch_prev+(6*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size))),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      multadd_complex_vector_real_scalar(ch,24576,ch_prev+(6*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size))),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
    }

    if (l==10) {
      ch = (short *)&dl_ch_estimates[aarx][ch_offset];
      ch_prev = (short *)&dl_ch_estimates[aarx][6*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)];
      // 3/4 ch6 + 1/4 ch10 => ch7
      multadd_complex_vector_real_scalar(ch_prev,24576,ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      multadd_complex_vector_real_scalar(ch,8192,ch_prev+(2*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      // 1/2 ch6 + 1/2 ch10 => ch8
      multadd_complex_vector_real_scalar(ch_prev,16384,ch_prev+(4*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      multadd_complex_vector_real_scalar(ch,16384,ch_prev+(4*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      // 1/4 ch6 + 3/4 ch10 => ch9
      multadd_complex_vector_real_scalar(ch_prev,8192,ch_prev+(6*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size))),1,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      multadd_complex_vector_real_scalar(ch,24576,ch_prev+(6*((phy_vars_ue->lte_frame_parms.ofdm_symbol_size))),0,phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
      // 5/4 ch10 - 1/4 ch6 => ch11
      // Ch11
      ch_prev = (short *)&dl_ch_estimates[aarx][10*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)];
      ch11 = (short *)&dl_ch_estimates[aarx][11*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size)];
      memcpy(ch11,ch_prev,4*phy_vars_ue->lte_frame_parms.ofdm_symbol_size);
    }
  }

  // do ifft of channel estimate
  for (aa=0; aa<phy_vars_ue->lte_frame_parms.nb_antennas_rx*phy_vars_ue->lte_frame_parms.nb_antennas_tx; aa++) {
    if (phy_vars_ue->lte_ue_common_vars.dl_ch_estimates[eNB_offset][aa]) {
      switch (phy_vars_ue->lte_frame_parms.N_RB_DL) {
      case 6:
	idft128((int16_t*) &phy_vars_ue->lte_ue_common_vars.dl_ch_estimates[eNB_offset][aa][8],
		(int16_t*) phy_vars_ue->lte_ue_common_vars.dl_ch_estimates_time[eNB_offset][aa],
		1);
	break;
      case 25:
	idft512((int16_t*) &phy_vars_ue->lte_ue_common_vars.dl_ch_estimates[eNB_offset][aa][8],
		(int16_t*) phy_vars_ue->lte_ue_common_vars.dl_ch_estimates_time[eNB_offset][aa],
		1);
	break;
      case 50:
	idft1024((int16_t*) &phy_vars_ue->lte_ue_common_vars.dl_ch_estimates[eNB_offset][aa][8],
		(int16_t*) phy_vars_ue->lte_ue_common_vars.dl_ch_estimates_time[eNB_offset][aa],
		1);
	break;
      case 75:
	idft1536((int16_t*) &phy_vars_ue->lte_ue_common_vars.dl_ch_estimates[eNB_offset][aa][8],
		 (int16_t*) phy_vars_ue->lte_ue_common_vars.dl_ch_estimates_time[eNB_offset][aa],
		 1);
	break;
      case 100:
	idft2048((int16_t*) &phy_vars_ue->lte_ue_common_vars.dl_ch_estimates[eNB_offset][aa][8],
		(int16_t*) phy_vars_ue->lte_ue_common_vars.dl_ch_estimates_time[eNB_offset][aa],
		1);
	break;
      default:
	break;
      }
    }
  }

  return(0);
}

