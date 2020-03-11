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
#include "assertions.h"
#include "openair1/PHY/CODING/nrLDPC_defs.h"
#include "ldpc_generate_coefficient.c"


int ldpc_encoder_orig(unsigned char *test_input,unsigned char *channel_input,int Zc,int Kb,short block_length, short BG,unsigned char gen_code)
{
  unsigned char c[22*384]; //padded input, unpacked, max size
  unsigned char d[68*384]; //coded output, unpacked, max size
  unsigned char channel_temp,temp;
  short *Gen_shift_values, *no_shift_values, *pointer_shift_values;

  short nrows = 46;//parity check bits
  short ncols = 22;//info bits


  int i,i1,i2,i3,i4,i5,temp_prime,var;
  int no_punctured_columns,removed_bit,rate=3;
  int nind=0;
  int indlist[1000];
  int indlist2[1000];

  //determine number of bits in codeword
  //if (block_length>3840)
     if (BG==1)
       {
         nrows=46; //parity check bits
         ncols=22; //info bits
         rate=3;
       }
       //else if (block_length<=3840)
      else if	(BG==2)
       {
         //BG=2;
         nrows=42; //parity check bits
         ncols=10; // info bits
         rate=5;
         }

  Gen_shift_values=choose_generator_matrix(BG,Zc);
  if (Gen_shift_values==NULL) {
    printf("ldpc_encoder_orig: could not find generator matrix\n");
    return(-1);
  }

  //printf("ldpc_encoder_orig: BG %d, Zc %d, Kb %d\n",BG, Zc, Kb);

  // load base graph of generator matrix
  if (BG==1)
  {
    no_shift_values=(short *) no_shift_values_BG1;
    pointer_shift_values=(short *) pointer_shift_values_BG1;
  }
  else if (BG==2)
  {
    no_shift_values=(short *) no_shift_values_BG2;
    pointer_shift_values=(short *) pointer_shift_values_BG2;
  }
  else {
    AssertFatal(0,"BG %d is not supported yet\n",BG);
  }
  
  no_punctured_columns=(int)((nrows-2)*Zc+block_length-block_length*rate)/Zc;
  removed_bit=(nrows-no_punctured_columns-2) * Zc+block_length-(block_length*rate);
  //printf("%d\n",no_punctured_columns);
  //printf("%d\n",removed_bit);
  // unpack input
  memset(c,0,sizeof(unsigned char) * ncols * Zc);
  memset(d,0,sizeof(unsigned char) * nrows * Zc);

  for (i=0; i<block_length; i++)
  {
    //c[i] = test_input[i/8]<<(i%8);
    //c[i]=c[i]>>7&1;
    c[i]=(test_input[i/8]&(128>>(i&7)))>>(7-(i&7));
  }

  // parity check part

  if (gen_code==1)
  {
    char fname[100];
    sprintf(fname,"ldpc_BG%d_Zc%d_byte.c",BG,Zc);
    FILE *fd=fopen(fname,"w");
    AssertFatal(fd!=NULL,"cannot open %s\n",fname);
    sprintf(fname,"ldpc_BG%d_Zc%d_16bit.c",BG,Zc);
    FILE *fd2=fopen(fname,"w");
    AssertFatal(fd2!=NULL,"cannot open %s\n",fname);

    int shift;
    char data_type[100];
    char xor_command[100];
    int mask;




    fprintf(fd,"#include \"PHY/sse_intrin.h\"\n");
    fprintf(fd2,"#include \"PHY/sse_intrin.h\"\n");

    if ((Zc&31)==0) {
      shift=5; // AVX2 - 256-bit SIMD
      mask=31;
      strcpy(data_type,"__m256i");
      strcpy(xor_command,"_mm256_xor_si256");
    }
    else if ((Zc&15)==0) {
      shift=4; // SSE4 - 128-bit SIMD
      mask=15;
      strcpy(data_type,"__m128i");
      strcpy(xor_command,"_mm_xor_si128");

    }
    else if ((Zc&7)==0) {
      shift=3; // MMX  - 64-bit SIMD
      mask=7;
      strcpy(data_type,"__m64");
      strcpy(xor_command,"_mm_xor_si64"); 
    }
    else {
      shift=0;                 // no SIMD
      mask=0;
      strcpy(data_type,"uint8_t");
      strcpy(xor_command,"scalar_xor");
      fprintf(fd,"#define scalar_xor(a,b) ((a)^(b))\n");
      fprintf(fd2,"#define scalar_xor(a,b) ((a)^(b))\n");
    }
    fprintf(fd,"// generated code for Zc=%d, byte encoding\n",Zc);
    fprintf(fd2,"// generated code for Zc=%d, 16bit encoding\n",Zc);
    fprintf(fd,"static inline void ldpc_BG%d_Zc%d_byte(uint8_t *c,uint8_t *d) {\n",BG,Zc);
    fprintf(fd2,"static inline void ldpc_BG%d_Zc%d_16bit(uint16_t *c,uint16_t *d) {\n",BG,Zc);
    fprintf(fd,"  %s *csimd=(%s *)c,*dsimd=(%s *)d;\n\n",data_type,data_type,data_type);
    fprintf(fd2,"  %s *csimd=(%s *)c,*dsimd=(%s *)d;\n\n",data_type,data_type,data_type);
    fprintf(fd,"  %s *c2,*d2;\n\n",data_type);
    fprintf(fd2,"  %s *c2,*d2;\n\n",data_type);
    fprintf(fd,"  int i2;\n");
    fprintf(fd2,"  int i2;\n");
    fprintf(fd,"  for (i2=0; i2<%d; i2++) {\n",Zc>>shift);
    if (shift > 0)
      fprintf(fd2,"  for (i2=0; i2<%d; i2++) {\n",Zc>>(shift-1));
    for (i2=0; i2 < 1; i2++)
    {
      //t=Kb*Zc+i2;
    
      // calculate each row in base graph
     

      fprintf(fd,"     c2=&csimd[i2];\n");
      fprintf(fd,"     d2=&dsimd[i2];\n");
      fprintf(fd2,"     c2=&csimd[i2];\n");
      fprintf(fd2,"     d2=&dsimd[i2];\n");

      for (i1=0; i1 < nrows; i1++)

      {
        channel_temp=0;
        fprintf(fd,"\n//row: %d\n",i1);
        fprintf(fd2,"\n//row: %d\n",i1);
	fprintf(fd,"     d2[%d]=",(Zc*i1)>>shift);
	fprintf(fd2,"     d2[%d]=",(Zc*i1)>>(shift-1));

        nind=0;

        for (i3=0; i3 < ncols; i3++)
        {
          temp_prime=i1 * ncols + i3;


	  for (i4=0; i4 < no_shift_values[temp_prime]; i4++)
	    {
	          
	      var=(int)((i3*Zc + (Gen_shift_values[ pointer_shift_values[temp_prime]+i4 ]+1)%Zc)/Zc);
	      int index =var*2*Zc + (i3*Zc + (Gen_shift_values[ pointer_shift_values[temp_prime]+i4 ]+1)%Zc) % Zc;
	      
	      indlist[nind] = ((index&mask)*((2*Zc)>>shift)*Kb)+(index>>shift);
	      indlist2[nind++] = ((index&(mask>>1))*((2*Zc)>>(shift-1))*Kb)+(index>>(shift-1));
	      
	    }
	  

        }
	for (i4=0;i4<nind-1;i4++) {
	  fprintf(fd,"%s(c2[%d],",xor_command,indlist[i4]);
	  fprintf(fd2,"%s(c2[%d],",xor_command,indlist2[i4]);
	}
	fprintf(fd,"c2[%d]",indlist[i4]);
	fprintf(fd2,"c2[%d]",indlist2[i4]);
	for (i4=0;i4<nind-1;i4++) { fprintf(fd,")"); fprintf(fd2,")"); }
	fprintf(fd,";\n");
	fprintf(fd2,";\n");

      }
      fprintf(fd,"  }\n}\n");
      fprintf(fd2,"  }\n}\n");
    }
    fclose(fd);
    fclose(fd2);
  }
  else if(gen_code==0)
  {
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
  }

  // information part and puncture columns
  memcpy(&channel_input[0], &c[2*Zc], (block_length-2*Zc)*sizeof(unsigned char));
  memcpy(&channel_input[block_length-2*Zc], &d[0], ((nrows-no_punctured_columns) * Zc-removed_bit)*sizeof(unsigned char));
  //memcpy(channel_input,c,Kb*Zc*sizeof(unsigned char));
  return 0;
}


int nrLDPC_encod(unsigned char **test_input,unsigned char **channel_input,int Zc,int Kb,short block_length, short BG, encoder_implemparams_t *impp) {
  return ldpc_encoder_orig(test_input[0],channel_input[0],Zc,Kb,block_length,BG,impp->gen_code);
}