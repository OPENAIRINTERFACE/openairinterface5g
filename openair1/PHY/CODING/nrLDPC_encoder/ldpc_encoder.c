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

/*!\file ldpc_encoder.c
 * \brief Defines the LDPC encoder
 * \author Florian Kaltenberger, Raymond Knopp, Kien le Trung (Eurecom)
 * \email openair_tech@eurecom.fr
 * \date 27-03-2018
 * \version 1.0
 * \note
 * \warning
 */



#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <types.h>
#include "defs.h"

short *choose_generator_matrix(short BG,short Zc);
extern short no_shift_values_BG1[1012],pointer_shift_values_BG1[1012],no_shift_values_BG2[2109],pointer_shift_values_BG2[2019];

int encode_parity_check_part_orig(unsigned char *c,unsigned char *d, short BG,short Zc,short Kb,short block_length)
{
  short *Gen_shift_values=choose_generator_matrix(BG,Zc);
  short *no_shift_values, *pointer_shift_values;
  int no_punctured_columns;
  short nrows,ncols;
  int i1,i2,i3,i4,i5,temp_prime;
  unsigned char channel_temp,temp;

  if (BG==1)
  {
    no_shift_values=(short *) no_shift_values_BG1;
    pointer_shift_values=(short *) pointer_shift_values_BG1;
      nrows=46; //parity check bits
      ncols=22; //info bits
  }
  else if (BG==2)
  {
    no_shift_values=(short *) no_shift_values_BG2;
    pointer_shift_values=(short *) pointer_shift_values_BG2;
      nrows=42; //parity check bits
      ncols=10; //info bits
  }
  else {
    printf("problem with BG\n");
    return(-1);
  }
  

  no_punctured_columns=(int)((nrows-2)*Zc+block_length-block_length*3)/Zc;

  //printf("no_punctured_columns = %d\n",no_punctured_columns);

  for (i2=0; i2 < Zc; i2++)
  {
    //t=Kb*Zc+i2;

    //rotate matrix here
    for (i5=0; i5 < Kb; i5++)
    {
      temp = c[i5*Zc];
      memmove(&c[i5*Zc], &c[i5*Zc+1], (Zc-1)*sizeof(unsigned char));
      c[i5*Zc+Zc-1] = temp;
    }

    // calculate each row in base graph
    for (i1=0; i1 < nrows-no_punctured_columns; i1++)
    {
      channel_temp=0;
      for (i3=0; i3 < Kb; i3++)
      {
        temp_prime=i1 * ncols + i3;

        for (i4=0; i4 < no_shift_values[temp_prime]; i4++)
        {
          channel_temp = channel_temp ^ c[ i3*Zc + Gen_shift_values[ pointer_shift_values[temp_prime]+i4 ] ];
        }
      }
      d[i2+i1*Zc]=channel_temp;
      //channel_input[t+i1*Zc]=channel_temp;
    }
  }
  return(0);
}




