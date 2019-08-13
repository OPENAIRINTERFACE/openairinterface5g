/*
   Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
   contributor license agreements.  See the NOTICE file distributed with
   this work for additional information regarding copyright ownership.
   The OpenAirInterface Software Alliance licenses this file to You under
   the OAI Public License, Version 1.1  (the "License"); you may not use this file
   except in compliance with the License.
   You may obtain a copy of the License at

        http://www.openairinterface.org/?page_id=698

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
  -------------------------------------------------------------------------------
   For more information about the OpenAirInterface (OAI) Software Alliance:
        contact@openairinterface.org
*/

/*!\brief Initilization and reconfiguration routines for LTE PHY */
#include "phy_init.h"
#include "PHY/phy_extern.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"

void init_sss(void);

void generate_64qam_table(void) {
  int a,b,c,index;

  for (a=-1; a<=1; a+=2)
    for (b=-1; b<=1; b+=2)
      for (c=-1; c<=1; c+=2) {
        index = (1+a)*2 + (1+b) + (1+c)/2;
        qam64_table[index] = -a*(QAM64_n1 + b*(QAM64_n2 + (c*QAM64_n3))); // 0 1 2
      }
}

void generate_16qam_table(void) {
  int a,b,index;

  for (a=-1; a<=1; a+=2)
    for (b=-1; b<=1; b+=2) {
      index = (1+a) + (1+b)/2;
      qam16_table[index] = -a*(QAM16_n1 + (b*QAM16_n2));
    }
}

void generate_qpsk_table(void) {
  int a,index;

  for (a=-1; a<=1; a+=2) {
    index = (1+a)/2;
    qpsk_table[index] = -a*QPSK;
  }
}

void init_lte_top(LTE_DL_FRAME_PARMS *frame_parms) {
  ccodelte_init();
  ccodelte_init_inv();
  phy_generate_viterbi_tables_lte();
  load_codinglib();
  generate_ul_ref_sigs();
  generate_ul_ref_sigs_rx();
  generate_64qam_table();
  generate_16qam_table();
  generate_qpsk_table();
  generate_RIV_tables();
  init_unscrambling_lut();
  init_scrambling_lut();
  //set_taus_seed(1328);
  init_sss();
}

void free_lte_top(void) {
  free_codinglib();
  /* free_ul_ref_sigs() is called in phy_free_lte_eNB() */
}


/*
 * @}*/
