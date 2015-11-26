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
#include "defs.h"

int mult_cpx_matrix_h(short *x1[2][2],
                      short *x2[2][2],
                      short *y[2][2],
                      unsigned int N,
                      unsigned short output_shift,
                      short hermitian)
{

  if (hermitian) {
    // this computes x1^H*x2 and stores it in y
    mult_cpx_vector_h(x2[0][0],x1[0][0],y[0][0],N,output_shift,1);
    mult_cpx_vector_h(x2[0][1],x1[0][1],y[0][0],N,output_shift,1);
    mult_cpx_vector_h(x2[0][0],x1[1][0],y[1][0],N,output_shift,1);
    mult_cpx_vector_h(x2[0][1],x1[1][1],y[1][0],N,output_shift,1);
    mult_cpx_vector_h(x2[1][0],x1[0][0],y[0][1],N,output_shift,1);
    mult_cpx_vector_h(x2[1][1],x1[0][1],y[0][1],N,output_shift,1);
    mult_cpx_vector_h(x2[1][0],x1[1][0],y[1][1],N,output_shift,1);
    mult_cpx_vector_h(x2[1][1],x1[1][1],y[1][1],N,output_shift,1);
  } else {
    // this computes x1*x2^H and stores it in y
    mult_cpx_vector_h(x1[0][0],x2[0][0],y[0][0],N,output_shift,1);
    mult_cpx_vector_h(x1[0][1],x2[0][1],y[0][0],N,output_shift,1);
    mult_cpx_vector_h(x1[0][0],x2[1][0],y[0][1],N,output_shift,1);
    mult_cpx_vector_h(x1[0][1],x2[1][1],y[0][1],N,output_shift,1);
    mult_cpx_vector_h(x1[1][0],x2[0][0],y[1][0],N,output_shift,1);
    mult_cpx_vector_h(x1[1][1],x2[0][1],y[1][0],N,output_shift,1);
    mult_cpx_vector_h(x1[1][0],x2[1][0],y[1][1],N,output_shift,1);
    mult_cpx_vector_h(x1[1][1],x2[1][1],y[1][1],N,output_shift,1);
  }
}

int mult_cpx_matrix_vector(int *x1[2][2],
                           int *x2[2],
                           int *y[2],
                           unsigned int N,
                           unsigned short output_shift)
{

  Zero_Buffer(y[0],N*8);
  Zero_Buffer(y[1],N*8);

  // this computes x1*x2 and stores it in y (32 bit)
  mult_cpx_vector_add32((short*)x2[0],(short*)x1[0][0],(short*)y[0],N);
  mult_cpx_vector_add32((short*)x2[1],(short*)x1[0][1],(short*)y[0],N);
  mult_cpx_vector_add32((short*)x2[0],(short*)x1[1][0],(short*)y[1],N);
  mult_cpx_vector_add32((short*)x2[1],(short*)x1[1][1],(short*)y[1],N);

  // shift and pack
  shift_and_pack((short*)y[0],N,output_shift);
  shift_and_pack((short*)y[1],N,output_shift);

}

#ifdef MAIN_MM
#include <stdio.h>
#include <stdlib.h>
main ()
{
  short x1_00[256] __attribute__((aligned(16)));
  short x1_10[256] __attribute__((aligned(16)));
  short x1_01[256] __attribute__((aligned(16)));
  short x1_11[256] __attribute__((aligned(16)));
  short x2_0[256] __attribute__((aligned(16)));
  short x2_1[256] __attribute__((aligned(16)));
  short y_0[256] __attribute__((aligned(16)));
  short y_1[256] __attribute__((aligned(16)));

  int *x1[2][2];
  int *x2[2];
  int *y[2];
  int i,m,n;

  x1[0][0] = (int*)x1_00;
  x1[0][1] = (int*)x1_01;
  x1[1][0] = (int*)x1_10;
  x1[1][1] = (int*)x1_11;
  x2[0] = (int*)x2_0;
  x2[1] = (int*)x2_1;
  y[0] = (int*)y_0;
  y[1] = (int*)y_1;

  for(m=0; m<2; m++) {
    for(n=0; n<2; n++) {
      for(i=0; i<256; i+=4) {
        ((short*)x1[m][n])[i] = ((short) rand())/4;
        ((short*)x1[m][n])[i+1] = ((short) rand())/4;
        ((short*)x1[m][n])[i+2] = -((short*)x1[m][n])[i+1];
        ((short*)x1[m][n])[i+3] = ((short*)x1[m][n])[i];
      }
    }

    for(i=0; i<256; i+=4) {
      ((short*)x2[m])[i] = ((short) rand())/4;
      ((short*)x2[m])[i+1] = ((short) rand())/4;
      ((short*)x2[m])[i+2] = ((short*)x2[m])[i];
      ((short*)x2[m])[i+3] = ((short*)x2[m])[i+1];
    }

    Zero_Buffer(y[m],512);
  }

  /*
  input[0] = 100;
  input[1] = 200;
  input[2] = -200;
  input[3] = 100;
  input[4] = 1000;
  input[5] = 2000;
  input[6] = -2000;
  input[7] = 1000;
  input[8] = 100;
  input[9] = 200;
  input[10] = -200;
  input[11] = 100;
  input[12] = 1000;
  input[13] = 2000;
  input[14] = -2000;
  input[15] = 1000;

  input2[0] = 2;
  input2[1] = 1;
  input2[2] = 2;
  input2[3] = 1;
  input2[4] = 20;
  input2[5] = 10;
  input2[6] = 20;
  input2[7] = 10;
  input2[8] = 2;
  input2[9] = 1;
  input2[10] = 2;
  input2[11] = 1;
  input2[12] = 2000;
  input2[13] = 1000;
  input2[14] = 2000;
  input2[15] = 1000;

  x1[0][0] = (int*)input;
  x1[0][1] = (int*)input;
  x1[1][0] = (int*)input;
  x1[1][1] = (int*)input;

  x2[0] = (int*)input2;
  x2[1] = (int*)input2;

  y[0] = (int*)output;
  y[1] = (int*)output2;
  */

  mult_cpx_matrix_vector(x1,x2,y,64,15);

  //mult_cpx_vector_add32(x2[0],x1[0][0],y[0],64);

  for (i=0; i<128; i+=2)
    printf("i=%d, x1 = [%d+1i*%d %d+1i*%d; %d+1i*%d %d+1i*%d]; x2 = [%d+1i*%d; %d+1i*%d]; y = [%d+1i*%d; %d+1i*%d]; y_m= round(x1*x2./pow2(15)); y-y_m \n",
           i,
           ((short*)x1[0][0])[2*i],  ((short*)x1[0][0])[2*i+2],
           ((short*)x1[0][1])[2*i],  ((short*)x1[0][1])[2*i+2],
           ((short*)x1[1][0])[2*i],  ((short*)x1[1][0])[2*i+2],
           ((short*)x1[1][1])[2*i],  ((short*)x1[1][1])[2*i+2],
           ((short*)x2[0])[2*i],  ((short*)x2[0])[2*i+1],
           ((short*)x2[1])[2*i],  ((short*)x2[1])[2*i+1],
           ((short*)y[0])[2*i],  ((short*)y[0])[2*i+1],
           ((short*)y[1])[2*i],  ((short*)y[1])[2*i+1]);

  //((int*)y[0])[i],  ((int*)y[0])[i+1],
  //((int*)y[1])[i],  ((int*)y[1])[i+1]);

}

#endif
