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

/* file: 3gpplte_turbo_decoder.c
   purpose: Fixed-point routines for implementing max-logmap decoding of Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03
   Modified from TI C6x reference design.
   currently maintained in openairinterface.org by R. Knopp (knopp@eurecom.fr)
   Note: This is a reference design for the sse version (3gpplte_turbo_decoder_sse.c)

*/

#include "PHY/defs_common.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/sse_intrin.h"

/*
// 3gpp2 polynomials
#define M0T -m11
#define M0B m11
#define M1T m11
#define M1B -m11
#define M2T -m10
#define M2B m10
#define M3T m10
#define M3B -m10
#define M4T m10
#define M4B -m10
#define M5T -m10
#define M5B m10
#define M6T m11
#define M6B -m11
#define M7T -m11
#define M7B m11
*/

// 3gpplte polynomials
#define m00 (-m11)
#define m01 (-m10)

#define M0T m00
#define M0B m11
#define M1T m11
#define M1B m00
#define M2T m10
#define M2B m01
#define M3T m01
#define M3B m10
#define M4T m01
#define M4B m10
#define M5T m10
#define M5B m01
#define M6T m11
#define M6B m00
#define M7T m00
#define M7B m11

#define M0T_TERM m00
#define M1T_TERM m11
#define M2T_TERM m10
#define M3T_TERM m01
#define M4T_TERM m01
#define M5T_TERM m10
#define M6T_TERM m11
#define M7T_TERM m00
/*
// 3GPP2 AlphaBeta
#define ALPHA_BETA_1m00 alpha[(k-1)*STATES+0] + beta[k*STATES+0]
#define ALPHA_BETA_1m11 alpha[(k-1)*STATES+0] + beta[k*STATES+4]
#define ALPHA_BETA_2m11 alpha[(k-1)*STATES+1] + beta[k*STATES+0]
#define ALPHA_BETA_2m00 alpha[(k-1)*STATES+1] + beta[k*STATES+4]
#define ALPHA_BETA_1m01 alpha[(k-1)*STATES+2] + beta[k*STATES+1]
#define ALPHA_BETA_1m10 alpha[(k-1)*STATES+2] + beta[k*STATES+5]
#define ALPHA_BETA_2m10 alpha[(k-1)*STATES+3] + beta[k*STATES+1]
#define ALPHA_BETA_2m01 alpha[(k-1)*STATES+3] + beta[k*STATES+5]
#define ALPHA_BETA_3m10 alpha[(k-1)*STATES+4] + beta[k*STATES+2]
#define ALPHA_BETA_3m01 alpha[(k-1)*STATES+4] + beta[k*STATES+6]
#define ALPHA_BETA_4m01 alpha[(k-1)*STATES+5] + beta[k*STATES+2]
#define ALPHA_BETA_4m10 alpha[(k-1)*STATES+5] + beta[k*STATES+6]
#define ALPHA_BETA_3m11 alpha[(k-1)*STATES+6] + beta[k*STATES+3]
#define ALPHA_BETA_3m00 alpha[(k-1)*STATES+6] + beta[k*STATES+7]
#define ALPHA_BETA_4m00 alpha[(k-1)*STATES+7] + beta[k*STATES+3]
#define ALPHA_BETA_4m11 alpha[(k-1)*STATES+7] + beta[k*STATES+7]
*/

#define ALPHA_BETA_1m00 alpha[(k-1)*STATES+0] + beta[k*STATES+0]
#define ALPHA_BETA_1m11 alpha[(k-1)*STATES+0] + beta[k*STATES+4]
#define ALPHA_BETA_2m11 alpha[(k-1)*STATES+1] + beta[k*STATES+0]
#define ALPHA_BETA_2m00 alpha[(k-1)*STATES+1] + beta[k*STATES+4]
#define ALPHA_BETA_1m10 alpha[(k-1)*STATES+2] + beta[k*STATES+1]
#define ALPHA_BETA_1m01 alpha[(k-1)*STATES+2] + beta[k*STATES+5]
#define ALPHA_BETA_2m10 alpha[(k-1)*STATES+3] + beta[k*STATES+5]
#define ALPHA_BETA_2m01 alpha[(k-1)*STATES+3] + beta[k*STATES+1]
#define ALPHA_BETA_3m10 alpha[(k-1)*STATES+4] + beta[k*STATES+6]
#define ALPHA_BETA_3m01 alpha[(k-1)*STATES+4] + beta[k*STATES+2]
#define ALPHA_BETA_4m01 alpha[(k-1)*STATES+5] + beta[k*STATES+6]
#define ALPHA_BETA_4m10 alpha[(k-1)*STATES+5] + beta[k*STATES+2]
#define ALPHA_BETA_3m11 alpha[(k-1)*STATES+6] + beta[k*STATES+3]
#define ALPHA_BETA_3m00 alpha[(k-1)*STATES+6] + beta[k*STATES+7]
#define ALPHA_BETA_4m00 alpha[(k-1)*STATES+7] + beta[k*STATES+3]
#define ALPHA_BETA_4m11 alpha[(k-1)*STATES+7] + beta[k*STATES+7]


typedef char Binary;
typedef short llr_t; // internal decoder data is 16-bit fixed
typedef short channel_t;

#define LLR_MAX 32767
#define LLR_MIN -32768
#define LLRTOT 16
#define MAX 32767//16383
#define FRAME_LENGTH_MAX 6144
#define STATES 8

void log_map_s (llr_t* systematic,channel_t* y_parity, llr_t* ext,unsigned short frame_length,unsigned char term_flag,unsigned char F);
void compute_gamma_s(llr_t* m11,llr_t* m10,llr_t* systematic, channel_t* y_parity, unsigned short frame_length,unsigned char term_flag);
void compute_alpha_s(llr_t*alpha,llr_t* m11,llr_t* m10, unsigned short frame_length,unsigned char F);
void compute_beta_s(llr_t* beta,llr_t* m11,llr_t* m10,llr_t* alpha, unsigned short frame_length,unsigned char F);
void compute_ext_s(llr_t* alpha,llr_t* beta,llr_t* m11,llr_t* m10,llr_t* extrinsic, llr_t* ap, unsigned short frame_length);

// global variables
//
llr_t alpha[(FRAME_LENGTH_MAX+3+1)*8];
llr_t beta[(FRAME_LENGTH_MAX+3+1)*8];
llr_t m11[(FRAME_LENGTH_MAX+3)];
llr_t m10[(FRAME_LENGTH_MAX+3)];


void log_map_s(llr_t* systematic,channel_t* y_parity, llr_t* ext,unsigned short frame_length,unsigned char term_flag,unsigned char F)
{
#ifdef DEBUG_LOGMAP
  unsigned short i;


  unsigned short argmax;
  llr_t max;

  printf("Gamma ...\n");
#endif //DEBUG_LOGMAP

  compute_gamma_s(m11,m10,systematic,y_parity,frame_length,term_flag);

#ifdef DEBUG_LOGMAP

  for (i=0; i<frame_length+3; i++)
    printf("Position %d : (%d,%d,%d,%d)\n",i,m11[i],m10[i],-m11[i],-m10[i]);

  printf("\n");
  printf("Alpha ...\n");
#endif //DEBUG_LOGMAP

  compute_alpha_s(alpha,m11,m10,frame_length,F);

#ifdef DEBUG_LOGMAP

  for (i=0; i<frame_length+4; i++) {
    printf("Position %d : (%d,%d,%d,%d) (%d,%d,%d,%d,%d,%d,%d,%d) -> ",i,
           -m11[i],m10[i],m11[i],-m10[i],alpha[(i<<3) + 0],
           alpha[(i<<3) + 1],
           alpha[(i<<3) + 2],
           alpha[(i<<3) + 3],
           alpha[(i<<3) + 4],
           alpha[(i<<3) + 5],
           alpha[(i<<3) + 6],
           alpha[(i<<3) + 7]);
    argmax = 0;
    max = alpha[(i<<3)];

    if (max<alpha[(i<<3) + 1]) {
      argmax=1;
      max = alpha[(i<<3) + 1];
    }

    if (max<alpha[(i<<3) + 2]) {
      argmax=2;
      max = alpha[(i<<3) + 2];
    }

    if (max<alpha[(i<<3) + 3]) {
      argmax=3;
      max = alpha[(i<<3) + 3];
    }

    if (max<alpha[(i<<3) + 4]) {
      argmax=4;
      max = alpha[(i<<3) + 4];
    }

    if (max<alpha[(i<<3) + 5]) {
      argmax=5;
      max = alpha[(i<<3) + 5];
    }

    if (max<alpha[(i<<3) + 6]) {
      argmax=6;
      max = alpha[(i<<3) + 6];
    }

    if (max<alpha[(i<<3) + 7]) {
      argmax=7;
      max = alpha[(i<<3) + 7];
    }

    printf("argmax = %d\n",argmax);
  }

  printf("Beta ...\n");

#endif //DEBUG_LOGMAP

  compute_beta_s(beta,m11,m10,alpha,frame_length,F);

#ifdef DEBUG_LOGMAP

  for (i=0; i<=frame_length+3; i++) {
    printf("Position %d : (%d,%d,%d,%d,%d,%d,%d,%d)->",i,
           beta[(i<<3) + 0],
           beta[(i<<3) + 1],
           beta[(i<<3) + 2],
           beta[(i<<3) + 3],
           beta[(i<<3) + 4],
           beta[(i<<3) + 5],
           beta[(i<<3) + 6],
           beta[(i<<3) + 7]);

    argmax = 0;
    max = beta[(i<<3)];

    if (max<beta[(i<<3) + 1]) {
      argmax=1;
      max = beta[(i<<3) + 1];
    }

    if (max<beta[(i<<3) + 2]) {
      argmax=2;
      max = beta[(i<<3) + 2];
    }

    if (max<beta[(i<<3) + 3]) {
      argmax=3;
      max = beta[(i<<3) + 3];
    }

    if (max<beta[(i<<3) + 4]) {
      argmax=4;
      max = beta[(i<<3) + 4];
    }

    if (max<beta[(i<<3) + 5]) {
      argmax=5;
      max = beta[(i<<3) + 5];
    }

    if (max<beta[(i<<3) + 6]) {
      argmax=6;
      max = beta[(i<<3) + 6];
    }

    if (max<beta[(i<<3) + 7]) {
      argmax=7;
      max = beta[(i<<3) + 7];
    }

    printf("argmax = %d\n",argmax);
  }

  printf("Extrinsic ...\n");
#endif //DEBUG_LOGMAP

  compute_ext_s(alpha,beta,m11,m10,ext,systematic,frame_length);

#ifdef DEBUG_LOGMAP

  for (i=0; i<frame_length+3; i++)
    printf("Position %d : ext %d, ext+sys %d\n",i,
           ext[i],ext[i]+systematic[i]);

  printf("\n");
#endif //DEBUG_LOGMAP

}

inline int SAT_ADD(int a,int b,int m)
{

  if (a+b > m) {
#ifdef DEBUG_LOGMAP
    printf("***\n");
#endif
    return(m);
  }

  if (a+b < -m) {
#ifdef DEBUG_LOGMAP
    printf("***\n");
#endif
    return(-m);
  }

  return(a+b);
}

void compute_gamma_s(llr_t* m11,llr_t* m10,llr_t* systematic,channel_t* y_parity,
                     unsigned short frame_length,unsigned char term_flag)
{
  int k;

  for (k=0; k<frame_length; k++) {
    m11[k] = (systematic[k]+y_parity[k])/2;
    m10[k] = (systematic[k]-y_parity[k])/2;
    //      printf("gamma %d : (%d,%d) -> (%d,%d)\n",k,systematic[k],y_parity[k],m11[k],m10[k]);
  }

  // Compute metrics for trellis termination
  if (term_flag == 0) { // This is for the termination of the first code
    for (k=frame_length; k<frame_length+3; k++) {
      m11[k] = (systematic[k]+y_parity[k])/2;
      m10[k] = (systematic[k]-y_parity[k])/2;
      //      printf("gamma %d : (%d,%d) -> (%d,%d)\n",k,systematic[k],y_parity[k],m11[k],m10[k]);
    }
  } else {
    for (k=frame_length; k<frame_length+3; k++) {
      m11[k] = (systematic[k+3]+y_parity[k])/2;
      m10[k] = (systematic[k+3]-y_parity[k])/2;
      //      printf("gamma %d : (%d,%d) -> (%d,%d)\n",k,systematic[k],y_parity[k],m11[k],m10[k]);
    }
  }
}

short systematic0[6144],systematic1[6144],systematic2[6144],yparity1[6144],yparity2[6144];

void compute_alpha_s(llr_t* alpha,llr_t* m_11,llr_t* m_10,unsigned short frame_length,unsigned char F)
{

  int k,i;
  llr_t m11,m10;
  llr_t old0,old1,old2, old3, old4, old5, old6, old7;
  llr_t new0,new1,new2, new3, new4, new5, new6, new7;
  llr_t m_b0,m_b1,m_b2, m_b3, m_b4, m_b5, m_b6, m_b7;
  // initialize log_alpha[0][m]
  llr_t alpha_max;

  old0 = 0;
  old1 = -MAX/2;
  old2 = -MAX/2;
  old3 = -MAX/2;
  old4 = -MAX/2;
  old5 = -MAX/2;
  old6 = -MAX/2;
  old7 = -MAX/2;
  alpha[0*(frame_length)+0] = old0;
  alpha[0*(frame_length)+1] = old1;
  alpha[0*(frame_length)+2] = old2;
  alpha[0*(frame_length)+3] = old3;
  alpha[0*(frame_length)+4] = old4;
  alpha[0*(frame_length)+5] = old5;
  alpha[0*(frame_length)+6] = old6;
  alpha[0*(frame_length)+7] = old7;

  //
  // compute log_alpha[k][m]
  // Steady-state portion
  for (k=1; k<=F; k++)
    for (i=0; i<8; i++)
      alpha[i+(k*8)] = alpha[i];

  for (k=1; k<=frame_length; k++) {
    m11=m_11[k-1];
    m10=m_10[k-1];

    m_b0 = SAT_ADD(old1,M1T,MAX);
    m_b4 = SAT_ADD(old1,M1B,MAX);
    m_b1 = SAT_ADD(old3,M3T,MAX);
    m_b5 = SAT_ADD(old3,M3B,MAX);
    m_b2 = SAT_ADD(old5,M5T,MAX);
    m_b6 = SAT_ADD(old5,M5B,MAX);
    m_b3 = SAT_ADD(old7,M7T,MAX);
    m_b7 = SAT_ADD(old7,M7B,MAX);

    new0 = SAT_ADD(old0,M0T,MAX);
    new4 = SAT_ADD(old0,M0B,MAX);
    new1 = SAT_ADD(old2,M2T,MAX);
    new5 = SAT_ADD(old2,M2B,MAX);
    new2 = SAT_ADD(old4,M4T,MAX);
    new6 = SAT_ADD(old4,M4B,MAX);
    new3 = SAT_ADD(old6,M6T,MAX);
    new7 = SAT_ADD(old6,M6B,MAX);
    //      printf("Output %d (%d,%d)\n",k-1,systematic0[k-1],yparity1[k-1]);

    //      printf("(m_b0 %d,new0 %d),(m_b1 %d,new1 %d),(m_b2 %d,new2 %d),(m_b3 %d,new3 %d),(m_b4 %d,new4 %d),(m_b5 %d,new5 %d),(m_b6 %d,new6 %d),(m_b7 %d,new7 %d)\n",m_b0,new0,m_b1,new1,m_b2,new2,m_b3,new3,m_b4,new4,m_b5,new5,m_b6,new6,m_b7,new7);

    if (m_b0 > new0) new0=m_b0;

    alpha[k*STATES + 0] = new0;
    old0=new0;

    if (m_b4 > new4) new4=m_b4;

    alpha[k*STATES + 4] = new4;
    old4=new4;

    if (m_b1 > new1) new1=m_b1;

    alpha[k*STATES + 1] = new1;
    old1=new1;

    if (m_b5 > new5) new5=m_b5;

    alpha[k*STATES + 5] = new5;
    old5=new5;

    if (m_b2 > new2) new2=m_b2;

    alpha[k*STATES + 2] = new2;
    old2=new2;

    if (m_b6 > new6) new6=m_b6;

    alpha[k*STATES + 6] = new6;
    old6=new6;

    if (m_b3 > new3) new3=m_b3;

    alpha[k*STATES + 3] = new3;
    old3=new3;

    if (m_b7 > new7) new7=m_b7;

    alpha[k*STATES + 7] = new7;
    old7=new7;

    alpha_max = alpha[(STATES*k) + 0];

    if(alpha[(STATES*k) + 1]>alpha_max)
      alpha_max = alpha[(STATES*k) + 1];

    if(alpha[(STATES*k) + 2]>alpha_max)
      alpha_max = alpha[(STATES*k) + 2];

    if(alpha[(STATES*k) + 3]>alpha_max)
      alpha_max = alpha[(STATES*k) + 3];

    if(alpha[(STATES*k) + 4]>alpha_max)
      alpha_max = alpha[(STATES*k) + 4];

    if(alpha[(STATES*k) + 5]>alpha_max)
      alpha_max = alpha[(STATES*k) + 5];

    if(alpha[(STATES*k) + 6]>alpha_max)
      alpha_max = alpha[(STATES*k) + 6];

    if(alpha[(STATES*k) + 7]>alpha_max)
      alpha_max = alpha[(STATES*k) + 7];

    alpha[(STATES*k)+0]-=alpha_max;
    alpha[(STATES*k)+1]-=alpha_max;
    alpha[(STATES*k)+2]-=alpha_max;
    alpha[(STATES*k)+3]-=alpha_max;
    alpha[(STATES*k)+4]-=alpha_max;
    alpha[(STATES*k)+5]-=alpha_max;
    alpha[(STATES*k)+6]-=alpha_max;
    alpha[(STATES*k)+7]-=alpha_max;
    new0=alpha[(STATES*k)+0];
    new1=alpha[(STATES*k)+1];
    new2=alpha[(STATES*k)+2];
    new3=alpha[(STATES*k)+3];
    new4=alpha[(STATES*k)+4];
    new5=alpha[(STATES*k)+5];
    new6=alpha[(STATES*k)+6];
    new7=alpha[(STATES*k)+7];
  }

  for (k=frame_length+1; k<=frame_length+3; k++) {
    m11=m_11[k-1];
    m10=m_10[k-1];
    m_b0 = SAT_ADD(old1,M1T_TERM,MAX);
    m_b1 = SAT_ADD(old3,M3T_TERM,MAX);
    m_b2 = SAT_ADD(old5,M5T_TERM,MAX);
    m_b3 = SAT_ADD(old7,M7T_TERM,MAX);

    new0 = SAT_ADD(old0,M0T_TERM,MAX);
    new1 = SAT_ADD(old2,M2T_TERM,MAX);
    new2 = SAT_ADD(old4,M4T_TERM,MAX);
    new3 = SAT_ADD(old6,M6T_TERM,MAX);
#ifdef DEBUG_LOGMAP
    printf("alpha term: m00 %d, m10 %d : (%d,%d,%d,%d) (%d,%d,%d,%d) (%d,%d,%d,%d) (%d,%d,%d,%d)\n",
           m00,m10,old0,old1,m_b0,new0,old2,old3,m_b1,new1,old3,old4,m_b2,new2,old6,old7,m_b3,new3);
#endif //DEBUG_LOGMAP

    if (m_b0 > new0) new0=m_b0;

    alpha[k*STATES + 0] = new0;
    old0=new0;

    if (m_b2 > new2) new2=m_b2;

    alpha[k*STATES + 2] = new2;
    old2=new2;

    if (m_b1 > new1) new1=m_b1;

    alpha[k*STATES + 1] = new1;
    old1=new1;

    if (m_b3 > new3) new3=m_b3;

    alpha[k*STATES + 3] = new3;
    old3=new3;

    alpha_max = alpha[(STATES*k) + 0];

    if(alpha[(STATES*k) + 1]>alpha_max)
      alpha_max = alpha[(STATES*k) + 1];

    if(alpha[(STATES*k) + 2]>alpha_max)
      alpha_max = alpha[(STATES*k) + 2];

    if(alpha[(STATES*k) + 3]>alpha_max)
      alpha_max = alpha[(STATES*k) + 3];

    alpha[(STATES*k)+0]-=alpha_max;
    alpha[(STATES*k)+1]-=alpha_max;
    alpha[(STATES*k)+2]-=alpha_max;
    alpha[(STATES*k)+3]-=alpha_max;

    new0=alpha[(STATES*k)+0];
    new1=alpha[(STATES*k)+1];
    new2=alpha[(STATES*k)+2];
    new3=alpha[(STATES*k)+3];
  }
}

void compute_beta_s(llr_t* beta,llr_t *m_11,llr_t* m_10,llr_t* alpha,unsigned short frame_length,unsigned char F)
{
  int k;
  llr_t old0, old1, old2, old3, old4, old5, old6, old7;
  llr_t new0, new1, new2, new3, new4, new5, new6, new7;
  llr_t m_b0, m_b1, m_b2, m_b3, m_b4,m_b5, m_b6, m_b7;
  llr_t m11,m10;
  llr_t beta_max;

  //  int m_max;

  //  llr_t max1, temp1, max2, temp2;

  //  int m_max1, m_max2;

  //
  // initialize the first stage;
  // for state for which alpha iz maximum, set log_beta to 0,
  // set the other ones to MAX.

  /*
  m_max1 = 0;
  max1 = alpha[((frame_length+3)*STATES)+0];
  temp1 = alpha[((frame_length+3)*STATES)+1];
  if (temp1 > max1) { m_max1 = 1; max1 = temp1;};
  temp1 = alpha[((frame_length+3)*STATES)+2];
  if (temp1 > max1) { m_max1 = 2; max1 = temp1;};
  temp1 = alpha[((frame_length+3)*STATES)+3];
  if (temp1 > max1) { m_max1 = 3; max1 = temp1;};
  m_max2 = 7; max2 = alpha[((frame_length+3)*STATES)+7];
  temp2 = alpha[((frame_length+3)*STATES)+6];
  if (temp2 > max2) { m_max2 = 6; max2 = temp2;};
  temp2 = alpha[((frame_length+3)*STATES)+5];
  if (temp2 > max2) { m_max2 = 5; max2 = temp2;};
  temp2 = alpha[((frame_length+3)*STATES)+4];
  if (temp2 > max2) { m_max2 = 4; max2 = temp2;};
  if (max2 > max1) m_max1 = m_max2;


  if (m_max1==0) old0=0; else old0 = -MAX/2;
  if (m_max1==1) old1=0; else old1 = -MAX/2;
  if (m_max1==2) old2=0; else old2 = -MAX/2;
  if (m_max1==3) old3=0; else old3 = -MAX/2;
  if (m_max1==4) old4=0; else old4 = -MAX/2;
  if (m_max1==5) old5=0; else old5 = -MAX/2;
  if (m_max1==6) old6=0; else old6 = -MAX/2;
  if (m_max1==7) old7=0; else old7 = -MAX/2;
  */

  /*
  // Initialise zero state because of termination
  beta[(STATES*(frame_length+3)) + 0] = 0;
  beta[(STATES*(frame_length+3)) + 1] = -MAX/2;
  beta[(STATES*(frame_length+3)) + 2] = -MAX/2;
  beta[(STATES*(frame_length+3)) + 3] = -MAX/2;
  beta[(STATES*(frame_length+3)) + 4] = -MAX/2;
  beta[(STATES*(frame_length+3)) + 5] = -MAX/2;
  beta[(STATES*(frame_length+3)) + 6] = -MAX/2;
  beta[(STATES*(frame_length+3)) + 7] = -MAX/2;

  for (k=0;k<F;k++)
    for (i=0;i<8;i++)
      beta[i+(k*8)] = beta[i];

  old0 = 0;
  old1 = -MAX/2;
  old2 = -MAX/2;
  old3 = -MAX/2;
  old4 = -MAX/2;
  old5 = -MAX/2;
  old6 = -MAX/2;
  old7 = -MAX/2;

  //
  // compute beta[k][m]
   //
  for (k=(frame_length+2);k>=frame_length;k--)
    {
      m11=m_11[k];
      m10=m_10[k];
      new0 = SAT_ADD(old0,M0T_TERM,MAX);
      new1 = SAT_ADD(old0,M1T_TERM,MAX);
      new2 = SAT_ADD(old1,M2T_TERM,MAX);
      new3 = SAT_ADD(old1,M3T_TERM,MAX);
      new4 = SAT_ADD(old2,M4T_TERM,MAX);
      new5 = SAT_ADD(old2,M5T_TERM,MAX);
      new6 = SAT_ADD(old3,M6T_TERM,MAX);
      new7 = SAT_ADD(old3,M7T_TERM,MAX);

      beta[k*STATES + 0] = new0;
      old0=new0;

      beta[k*STATES + 1] = new1;
      old1=new1;

      beta[k*STATES + 2] = new2;
      old2=new2;

      beta[k*STATES + 3] = new3;
      old3=new3;

      beta[k*STATES + 4] = new4;
      old4=new4;

      beta[k*STATES + 5] = new5;
      old5=new5;

      beta[k*STATES + 6] = new6;
      old6=new6;

      beta[k*STATES + 7] = new7;
      old7=new7;
    }
  */


  beta[(STATES*(frame_length)) + 0] = alpha[(STATES*frame_length) + 0];
  beta[(STATES*(frame_length)) + 1] = alpha[(STATES*frame_length) + 1];
  beta[(STATES*(frame_length)) + 2] = alpha[(STATES*frame_length) + 2];
  beta[(STATES*(frame_length)) + 3] = alpha[(STATES*frame_length) + 3];
  beta[(STATES*(frame_length)) + 4] = alpha[(STATES*frame_length) + 4];
  beta[(STATES*(frame_length)) + 5] = alpha[(STATES*frame_length) + 5];
  beta[(STATES*(frame_length)) + 6] = alpha[(STATES*frame_length) + 6];
  beta[(STATES*(frame_length)) + 7] = alpha[(STATES*frame_length) + 7];


  old0 = beta[(STATES*frame_length)+0];
  old1 = beta[(STATES*frame_length)+1];
  old2 = beta[(STATES*frame_length)+2];
  old3 = beta[(STATES*frame_length)+3];
  old4 = beta[(STATES*frame_length)+4];
  old5 = beta[(STATES*frame_length)+5];
  old6 = beta[(STATES*frame_length)+6];
  old7 = beta[(STATES*frame_length)+7];

  for (k=(frame_length-1); k>=0; k--) {
    m11=m_11[k];
    m10=m_10[k];
    m_b0 = SAT_ADD(old4,M0B,MAX);
    m_b1 = SAT_ADD(old4,M1B,MAX);
    m_b2 = SAT_ADD(old5,M2B,MAX);
    m_b3 = SAT_ADD(old5,M3B,MAX);
    m_b4 = SAT_ADD(old6,M4B,MAX);
    m_b5 = SAT_ADD(old6,M5B,MAX);
    m_b6 = SAT_ADD(old7,M6B,MAX);
    m_b7 = SAT_ADD(old7,M7B,MAX);
    new0 = SAT_ADD(old0,M0T,MAX);
    new1 = SAT_ADD(old0,M1T,MAX);
    new2 = SAT_ADD(old1,M2T,MAX);
    new3 = SAT_ADD(old1,M3T,MAX);
    new4 = SAT_ADD(old2,M4T,MAX);
    new5 = SAT_ADD(old2,M5T,MAX);
    new6 = SAT_ADD(old3,M6T,MAX);
    new7 = SAT_ADD(old3,M7T,MAX);



    if (m_b0 > new0) new0=m_b0;

    beta[k*STATES + 0] = new0;
    old0=new0;

    if (m_b1 > new1) new1=m_b1;

    beta[k*STATES + 1] = new1;
    old1=new1;

    if (m_b2 > new2) new2=m_b2;

    beta[k*STATES + 2] = new2;
    old2=new2;

    if (m_b3 > new3) new3=m_b3;

    beta[k*STATES + 3] = new3;
    old3=new3;

    if (m_b4 > new4) new4=m_b4;

    beta[k*STATES + 4] = new4;
    old4=new4;

    if (m_b5 > new5) new5=m_b5;

    beta[k*STATES + 5] = new5;
    old5=new5;

    if (m_b6 > new6) new6=m_b6;

    beta[k*STATES + 6] = new6;
    old6=new6;

    if (m_b7 > new7) new7=m_b7;

    beta[k*STATES + 7] = new7;
    old7=new7;

    beta_max = beta[(STATES*k) + 0];

    if(beta[(STATES*k) + 1]>beta_max)
      beta_max = beta[(STATES*k) + 1];

    if(beta[(STATES*k) + 2]>beta_max)
      beta_max = beta[(STATES*k) + 2];

    if(beta[(STATES*k) + 3]>beta_max)
      beta_max = beta[(STATES*k) + 3];

    if(beta[(STATES*k) + 4]>beta_max)
      beta_max = beta[(STATES*k) + 4];

    if(beta[(STATES*k) + 5]>beta_max)
      beta_max = beta[(STATES*k) + 5];

    if(beta[(STATES*k) + 6]>beta_max)
      beta_max = beta[(STATES*k) + 6];

    if(beta[(STATES*k) + 7]>beta_max)
      beta_max = beta[(STATES*k) + 7];

    beta[(STATES*k)+0]-=beta_max;
    beta[(STATES*k)+1]-=beta_max;
    beta[(STATES*k)+2]-=beta_max;
    beta[(STATES*k)+3]-=beta_max;
    beta[(STATES*k)+4]-=beta_max;
    beta[(STATES*k)+5]-=beta_max;
    beta[(STATES*k)+6]-=beta_max;
    beta[(STATES*k)+7]-=beta_max;

    new0=beta[(STATES*k)+0];
    new1=beta[(STATES*k)+1];
    new2=beta[(STATES*k)+2];
    new3=beta[(STATES*k)+3];
    new4=beta[(STATES*k)+4];
    new5=beta[(STATES*k)+5];
    new6=beta[(STATES*k)+6];
    new7=beta[(STATES*k)+7];
    /*
    if (((k%(frame_length>>3))==0)&&(k>0)) {

    alpha[((k)*STATES)+0]=beta[((k)*STATES)+0]
    alpha[((k)*STATES)+1]=beta[((k)*STATES)+1];
    alpha[((k)*STATES)+2]=beta[((k)*STATES)+2];
    alpha[((k)*STATES)+3]=beta[((k)*STATES)+3];
    alpha[((k)*STATES)+4]=beta[((k)*STATES)+4];
    alpha[((k)*STATES)+5]=beta[((k)*STATES)+5];
    alpha[((k)*STATES)+6]=beta[((k)*STATES)+6];
    alpha[((k)*STATES)+7]=beta[((k)*STATES)+7];
    }
    */
  }
}
void compute_ext_s(llr_t* alpha,llr_t* beta,llr_t* m_11,llr_t* m_10,llr_t* ext, llr_t* systematic,unsigned short frame_length)
{
  int k;
  llr_t m11,m10;
  llr_t m00_1,m11_1,m00_2,m11_2,m00_3,m11_3,m00_4,m11_4;
  llr_t m01_1,m10_1,m01_2,m10_2,m01_3,m10_3,m01_4,m10_4;

  //
  // LLR computation
  //
  for (k=1; k<=(frame_length+3); k++) {
    m00_4 = ALPHA_BETA_4m00;
    m11_4 = ALPHA_BETA_4m11;
    m00_3 = ALPHA_BETA_3m00;
    m11_3 = ALPHA_BETA_3m11;
    m00_2 = ALPHA_BETA_2m00;
    m11_2 = ALPHA_BETA_2m11;
    m11_1 = ALPHA_BETA_1m11;
    m00_1 = ALPHA_BETA_1m00;
    m01_4 = ALPHA_BETA_4m01;
    m10_4 = ALPHA_BETA_4m10;
    m01_3 = ALPHA_BETA_3m01;
    m10_3 = ALPHA_BETA_3m10;
    m01_2 = ALPHA_BETA_2m01;
    m10_2 = ALPHA_BETA_2m10;
    m10_1 = ALPHA_BETA_1m10;
    m01_1 = ALPHA_BETA_1m01;

    if (m01_2 > m01_1) m01_1 = m01_2;

    if (m01_3 > m01_1) m01_1 = m01_3;

    if (m01_4 > m01_1) m01_1 = m01_4;

    if (m00_2 > m00_1) m00_1 = m00_2;

    if (m00_3 > m00_1) m00_1 = m00_3;

    if (m00_4 > m00_1) m00_1 = m00_4;

    if (m10_2 > m10_1) m10_1 = m10_2;

    if (m10_3 > m10_1) m10_1 = m10_3;

    if (m10_4 > m10_1) m10_1 = m10_4;

    if (m11_2 > m11_1) m11_1 = m11_2;

    if (m11_3 > m11_1) m11_1 = m11_3;

    if (m11_4 > m11_1) m11_1 = m11_4;

    m11=m_11[k-1];
    m10=m_10[k-1];
    m01_1 = SAT_ADD(m01_1,m01,MAX);
    m00_1 = SAT_ADD(m00_1,m00,MAX);
    m10_1 = SAT_ADD(m10_1,m10,MAX);
    m11_1 = SAT_ADD(m11_1,m11,MAX);

    if (m00_1 > m01_1) m01_1 = m00_1;

    if (m11_1 > m10_1) m10_1 = m11_1;

    ext[k-1] = SAT_ADD(m10_1,-SAT_ADD(m01_1,systematic[k-1],MAX),MAX);
#ifdef DEBUG_LOGMAP
    printf("Ext %d: m0 %d, m1 %d, syst %d ext %d\n",k-1,m01_1,m10_1,systematic[k-1],ext[k-1]);
#endif //DEBUG_LOGMAP

  };
}



unsigned char phy_threegpplte_turbo_decoder_scalar(llr_t *y,
    llr_t *y2,
    unsigned char *decoded_bytes,
    unsigned char *decoded_bytes2,
    unsigned short n,
    unsigned short f1,
    unsigned short f2,
    unsigned char max_iterations,
    unsigned char crc_type,
    unsigned char F,
    time_stats_t *init_stats,
    time_stats_t *alpha_stats,
    time_stats_t *beta_stats,
    time_stats_t *gamma_stats,
    time_stats_t *ext_stats,
    time_stats_t *intl1_stats,
    time_stats_t *intl2_stats)
{

  /*  y is a pointer to the input
    decoded_bytes is a pointer to the decoded output
    n is the size in bits of the coded block, with the tail */
  short ext[n],ext2[n];

  //  short systematic0[n],systematic1[n],systematic2[n],yparity1[n],yparity2[n];
  llr_t *yp = y;
  unsigned short i,pi;
  unsigned char iteration_cnt=0;
  uint32_t crc, oldcrc;
  unsigned char crc_len,temp;

  if (crc_type > 3) {
    fprintf(stderr,"Illegal crc length!\n");
    return 255;
  }

  switch (crc_type) {
  case CRC24_A:
  case CRC24_B:
    crc_len=3;
    break;

  case CRC16:
    crc_len=2;
    break;

  case CRC8:
    crc_len=1;
    break;

  default:
    crc_len=3;
  }

  for (i=0; i<n; i++) {
    systematic0[i] = *yp;
    yp++;
    yparity1[i] = *yp;
    yp++;
    yparity2[i] = *yp;
    yp++;
#ifdef DEBUG_LOGMAP
    printf("Position %d: (%d,%d,%d)\n",i,systematic0[i],yparity1[i],yparity2[i]);
#endif //DEBUG_LOGMAP

  }

  for (i=n; i<n+3; i++) {
    systematic0[i]= *yp ;
    yp++;
    yparity1[i] = *yp;
    yp++;
#ifdef DEBUG_LOGMAP
    printf("Term 1 (%d): %d %d\n",i,systematic0[i],yparity1[i]);
#endif //DEBUG_LOGMAP
  }

  for (i=n+3; i<n+6; i++) {
    systematic0[i]= *yp ;
    yp++;
    yparity2[i-3] = *yp;
    yp++;
#ifdef DEBUG_LOGMAP
    printf("Term 2 (%d): %d %d\n",i-3,systematic0[i],yparity2[i-3]);
#endif //DEBUG_LOGMAP
  }

#ifdef DEBUG_LOGMAP
  printf("\n");
#endif //DEBUG_LOGMAP



  // do log_map from first parity bit
  log_map_s(systematic0,yparity1,ext,n,0,F);


  while (iteration_cnt++ < max_iterations) {

#ifdef DEBUG_LOGMAP
    printf("\n*******************ITERATION %d\n\n",iteration_cnt);
#endif //DEBUG_LOGMAP

    threegpplte_interleaver_reset();
    pi=0;

    // compute input to second encoder by interleaving extrinsic info
    for (i=0; i<n; i++) { // steady-state portion
      systematic2[i] = (ext[pi] + systematic0[pi]);
      pi              = threegpplte_interleaver(f1,f2,n);
    }

    for (i=n; i<n+3; i++) { // termination
      systematic2[i] = (systematic0[i+8]);
    }

    // do log_map from second parity bit
    log_map_s(systematic2,yparity2,ext2,n,1,0);


    threegpplte_interleaver_reset();
    pi=0;

    for (i=0; i<n>>3; i++)
      decoded_bytes[i]=0;

    // compute input to first encoder and output
    for (i=0; i<n; i++) {
      systematic1[pi] = (ext2[i] + systematic0[pi]);
#ifdef DEBUG_LOGMAP
      printf("Second half %d: ext2[i] %d, systematic0[i] %d (e+s %d)\n",i,ext2[i],systematic0[pi],
             ext2[i]+systematic2[i]);
#endif //DEBUG_LOGMAP

      if ((systematic2[i] + ext2[i]) > 0)
        decoded_bytes[pi>>3] += (1 << (7-(pi&7)));

      pi              = threegpplte_interleaver(f1,f2,n);
    }

    for (i=n; i<n+3; i++) {
      systematic1[i] = (systematic0[i]);
#ifdef DEBUG_LOGMAP
      printf("Second half %d: ext2[i] %d, systematic0[i] %d (e+s %d)\n",i,ext2[i],systematic0[i],
             ext2[i]+systematic2[i]);
#endif //DEBUG_LOGMAP
    }


    // check status on output

    oldcrc= *((unsigned int *)(&decoded_bytes[(n>>3)-crc_len]));

    switch (crc_type) {

    case CRC24_A:
      oldcrc&=0x00ffffff;
      crc = crc24a(&decoded_bytes[F>>3],
                   n-24-F)>>8;
      temp=((uint8_t *)&crc)[2];
      ((uint8_t *)&crc)[2] = ((uint8_t *)&crc)[0];
      ((uint8_t *)&crc)[0] = temp;

      //           msg("CRC24_A = %x, oldcrc = %x (F %d)\n",crc,oldcrc,F);

      break;

    case CRC24_B:
      oldcrc&=0x00ffffff;
      crc = crc24b(decoded_bytes,
                   n-24)>>8;
      temp=((uint8_t *)&crc)[2];
      ((uint8_t *)&crc)[2] = ((uint8_t *)&crc)[0];
      ((uint8_t *)&crc)[0] = temp;

      //      msg("CRC24_B = %x, oldcrc = %x\n",crc,oldcrc);

      break;

    case CRC16:
      oldcrc&=0x0000ffff;
      crc = crc16(decoded_bytes,
                  n-16)>>16;

      break;

    case CRC8:
      oldcrc&=0x000000ff;
      crc = crc8(decoded_bytes,
                 n-8)>>24;
      break;
    }

    if (crc == oldcrc) {
      return(iteration_cnt);
    }

    // do log_map from first parity bit
    if (iteration_cnt < max_iterations)
      log_map_s(systematic1,yparity1,ext,n,0,F);
  }

  return(iteration_cnt);
}

#ifdef TEST_DEBUG

int test_logmap8()
{
  unsigned char test[8];
  //_declspec(align(16))  char channel_output[512];
  //_declspec(align(16))  unsigned char output[512],decoded_output[16], *inPtr, *outPtr;

  short channel_output[512];
  unsigned char output[512],decoded_output[16];
  unsigned int i;


  test[0] = 7;
  test[1] = 0xa5;
  test[2] = 0;
  test[3] = 0xfe;
  test[4] = 0x1a;
  test[5] = 0x0;
  //  test[5] = 0x33;
  //  test[6] = 0x99;
  //  test[7] = 0;


  threegpplte_turbo_encoder(test,
                            5,
                            output,
                            3,
                            10);

  for (i = 0; i < 132; i++) {
    channel_output[i] = 15*(2*output[i] - 1);
    //    printf("Position %d : %d\n",i,channel_output[i]);
  }

  memset(decoded_output,0,16);
  phy_threegpplte_turbo_decoder(channel_output,decoded_output,40,3,10,6,3);




}




void main()
{


  test_logmap8();

}

#endif // TEST_DEBUG


