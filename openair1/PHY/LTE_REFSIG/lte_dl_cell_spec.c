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
#include <stdio.h>
#include <stdlib.h>
#endif

#include "defs.h"
#include "PHY/defs.h"

//extern unsigned int lte_gold_table[3][20][2][14];
//#define DEBUG_DL_CELL_SPEC



//Calibration
int lte_dl_cell_spec_SS(PHY_VARS_eNB *phy_vars_eNB,
                        int32_t *output,
                        short amp,
                        unsigned char Ns,
                        unsigned char l,//nb of sym per slot
                        unsigned char p)  //nb of antennas
{

  unsigned char nu,mprime,mprime_dword,mprime_qpsk_symb,m;
  unsigned short k,a;
  int32_t qpsk[4];

  a = (amp*ONE_OVER_SQRT2_Q15)>>15;
  ((short *)&qpsk[0])[0] = a;
  ((short *)&qpsk[0])[1] = a;
  ((short *)&qpsk[1])[0] = -a;
  ((short *)&qpsk[1])[1] = a;
  ((short *)&qpsk[2])[0] = a;
  ((short *)&qpsk[2])[1] = -a;
  ((short *)&qpsk[3])[0] = -a;
  ((short *)&qpsk[3])[1] = -a;


  if ((p==0) && (l==0) )
    nu = 0;
  else if ((p==0) && (l>0))
    nu = 3;
  else if ((p==1) && (l==0))
    nu = 3;
  else if ((p==1) && (l>0))
    nu = 0;
  else {
    printf("lte_dl_cell_spec: p %d, l %d -> ERROR\n",p,l);
    return(-1);
  }

  mprime = 110 - phy_vars_eNB->lte_frame_parms.N_RB_DL;

  k = (nu + phy_vars_eNB->lte_frame_parms.nushift);

  if (k > 6)//b
    k -=6;//b

  k+=phy_vars_eNB->lte_frame_parms.first_carrier_offset;

  for (m=0; m<phy_vars_eNB->lte_frame_parms.N_RB_DL<<1; m++) { // loop over pilots in one slot/symbol, 2*N_RB_DL pilots

    mprime_dword     = mprime>>4;
    mprime_qpsk_symb = mprime&0xf;

    // this is r_mprime from 3GPP 36-211 6.10.1.2
    output[k] = qpsk[(phy_vars_eNB->lte_gold_table[Ns][l][mprime_dword]>>(2*mprime_qpsk_symb))&3];
    //output[k] = (lte_gold_table[eNB_offset][Ns][l][mprime_dword]>>(2*mprime_qpsk_symb))&3;
#ifdef DEBUG_DL_CELL_SPEC
    printf("Ns %d, l %d, m %d,mprime_dword %d, mprime_qpsk_symbol %d\n",
              Ns,l,m,mprime_dword,mprime_qpsk_symb);
    printf("index = %d (k %d)\n",(phy_vars_eNB->lte_gold_table[Ns][l][mprime_dword]>>(2*mprime_qpsk_symb))&3,k);
#endif

    mprime++;
#ifdef DEBUG_DL_CELL_SPEC

    if (m<4)
      printf("Ns %d, l %d output[%d] = (%d,%d)\n",Ns,l,k,((short *)&output[k])[0],((short *)&output[k])[1]);

#endif
    k+=6;//b

    if (k >= phy_vars_eNB->lte_frame_parms.ofdm_symbol_size) {
      k++;  // skip DC carrier
      k-=phy_vars_eNB->lte_frame_parms.ofdm_symbol_size;
    }

    //    printf("** k %d\n",k);
  }

  return(0);
}


int lte_dl_cell_spec(PHY_VARS_eNB *phy_vars_eNB,
                     int32_t *output,
                     short amp,
                     unsigned char Ns,
                     unsigned char l,
                     unsigned char p)
{

  unsigned char nu,mprime,mprime_dword,mprime_qpsk_symb,m;
  unsigned short k,a;
  int32_t qpsk[4];

  a = (amp*ONE_OVER_SQRT2_Q15)>>15;
  ((short *)&qpsk[0])[0] = a;
  ((short *)&qpsk[0])[1] = a;
  ((short *)&qpsk[1])[0] = -a;
  ((short *)&qpsk[1])[1] = a;
  ((short *)&qpsk[2])[0] = a;
  ((short *)&qpsk[2])[1] = -a;
  ((short *)&qpsk[3])[0] = -a;
  ((short *)&qpsk[3])[1] = -a;

  if ((p==0) && (l==0) )
    nu = 0;
  else if ((p==0) && (l>0))
    nu = 3;
  else if ((p==1) && (l==0))
    nu = 3;
  else if ((p==1) && (l>0))
    nu = 0;
  else {
    printf("lte_dl_cell_spec: p %d, l %d -> ERROR\n",p,l);
    return(-1);
  }

  mprime = 110 - phy_vars_eNB->lte_frame_parms.N_RB_DL;

  k = (nu + phy_vars_eNB->lte_frame_parms.nushift);

  if (k > 5)
    k -=6;

  k+=phy_vars_eNB->lte_frame_parms.first_carrier_offset;

  DevAssert( Ns < 20 );
  DevAssert( l < 2 );
  DevAssert( mprime>>4 < 14 );

  for (m=0; m<phy_vars_eNB->lte_frame_parms.N_RB_DL<<1; m++) {

    mprime_dword     = mprime>>4;
    mprime_qpsk_symb = mprime&0xf;

    // this is r_mprime from 3GPP 36-211 6.10.1.2
    output[k] = qpsk[(phy_vars_eNB->lte_gold_table[Ns][l][mprime_dword]>>(2*mprime_qpsk_symb))&3];
    //output[k] = (lte_gold_table[eNB_offset][Ns][l][mprime_dword]>>(2*mprime_qpsk_symb))&3;
#ifdef DEBUG_DL_CELL_SPEC
    printf("Ns %d, l %d, m %d,mprime_dword %d, mprime_qpsk_symbol %d\n",
        Ns,l,m,mprime_dword,mprime_qpsk_symb);
    printf("index = %d (k %d)\n",(phy_vars_eNB->lte_gold_table[Ns][l][mprime_dword]>>(2*mprime_qpsk_symb))&3,k);
#endif

    mprime++;
#ifdef DEBUG_DL_CELL_SPEC

    if (m<4)
      printf("Ns %d, l %d output[%d] = (%d,%d)\n",Ns,l,k,((short *)&output[k])[0],((short *)&output[k])[1]);

#endif
    k+=6;

    if (k >= phy_vars_eNB->lte_frame_parms.ofdm_symbol_size) {
      k++;  // skip DC carrier
      k-=phy_vars_eNB->lte_frame_parms.ofdm_symbol_size;
    }

    //    printf("** k %d\n",k);
  }

  return(0);
}

int lte_dl_cell_spec_rx(PHY_VARS_UE *phy_vars_ue,
                        uint8_t eNB_offset,
                        int *output,
                        unsigned char Ns,
                        unsigned char l,
                        unsigned char p)
{


  unsigned char mprime,mprime_dword,mprime_qpsk_symb,m;
  unsigned short k=0;
  unsigned int qpsk[4];
  short pamp;

  // Compute the correct pilot amplitude, sqrt_rho_b = Q3.13
  pamp = ONE_OVER_SQRT2_Q15;

  // This includes complex conjugate for channel estimation

  ((short *)&qpsk[0])[0] = pamp;
  ((short *)&qpsk[0])[1] = -pamp;
  ((short *)&qpsk[1])[0] = -pamp;
  ((short *)&qpsk[1])[1] = -pamp;
  ((short *)&qpsk[2])[0] = pamp;
  ((short *)&qpsk[2])[1] = pamp;
  ((short *)&qpsk[3])[0] = -pamp;
  ((short *)&qpsk[3])[1] = pamp;

  mprime = 110 - phy_vars_ue->lte_frame_parms.N_RB_DL;

  for (m=0; m<phy_vars_ue->lte_frame_parms.N_RB_DL<<1; m++) {

    mprime_dword     = mprime>>4;
    mprime_qpsk_symb = mprime&0xf;

    // this is r_mprime from 3GPP 36-211 6.10.1.2
    output[k] = qpsk[(phy_vars_ue->lte_gold_table[eNB_offset][Ns][l][mprime_dword]>>(2*mprime_qpsk_symb))&3];
#ifdef DEBUG_DL_CELL_SPEC
    printf("Ns %d, l %d, m %d,mprime_dword %d, mprime_qpsk_symbol %d\n",
           Ns,l,m,mprime_dword,mprime_qpsk_symb);
    printf("index = %d (k %d)\n",(phy_vars_ue->lte_gold_table[eNB_offset][Ns][l][mprime_dword]>>(2*mprime_qpsk_symb))&3,k);
#endif

    mprime++;
#ifdef DEBUG_DL_CELL_SPEC

    if (m<4)
      printf("Ns %d l %d output[%d] = (%d,%d)\n",Ns,l,k,((short *)&output[k])[0],((short *)&output[k])[1]);

#endif
    k++;
    //    printf("** k %d\n",k);
  }

  return(0);
}


#ifdef LTE_DL_CELL_SPEC_MAIN



//extern int write_output(const char *,const char *,void *,int,int,char);
// flag change eren
extern int write_output(const char *,const char *,void *,int,int,char);
main()
{

  unsigned short Nid_cell=0;
  unsigned int Ncp = 0;
  int output00[1024];
  int output01[1024];
  int output10[1024];
  int output11[1024];

  memset(output00,0,1024*sizeof(int));
  memset(output01,0,1024*sizeof(int));
  memset(output10,0,1024*sizeof(int));
  memset(output11,0,1024*sizeof(int));

  lte_gold(Nid_cell,Ncp);

  lte_dl_cell_spec(output00,
                   ONE_OVER_SQRT2_Q15,
                   50,
                   Nid_cell,
                   Ncp,
                   0,
                   0,
                   0,
                   0);

  lte_dl_cell_spec(output10,
                   ONE_OVER_SQRT2_Q15,
                   50,
                   Nid_cell,
                   Ncp,
                   0,
                   1,
                   0,
                   0);

  lte_dl_cell_spec(output01,
                   ONE_OVER_SQRT2_Q15,
                   50,
                   Nid_cell,
                   Ncp,
                   0,
                   0,
                   1,
                   0);

  lte_dl_cell_spec(output11,
                   ONE_OVER_SQRT2_Q15,
                   50,
                   Nid_cell,
                   Ncp,
                   0,
                   1,
                   1,
                   0);


  write_output("dl_cell_spec00.m","dl_cs00",output00,1024,1,1);
  write_output("dl_cell_spec01.m","dl_cs01",output01,1024,1,1);
  write_output("dl_cell_spec10.m","dl_cs10",output10,1024,1,1);
  write_output("dl_cell_spec11.m","dl_cs11",output11,1024,1,1);
}

#endif
