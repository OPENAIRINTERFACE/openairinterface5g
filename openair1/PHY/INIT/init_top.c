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

/*!\brief Initilization and reconfiguration routines for LTE PHY */
#include "defs.h"
#include "PHY/extern.h"
#include "PHY/CODING/extern.h"
void init_lte_top(LTE_DL_FRAME_PARMS *frame_parms)
{

  crcTableInit();

  ccodedot11_init();
  ccodedot11_init_inv();

  ccodelte_init();
  ccodelte_init_inv();



  phy_generate_viterbi_tables();
  phy_generate_viterbi_tables_lte();

  load_codinglib();
  lte_sync_time_init(frame_parms);

  generate_ul_ref_sigs();
  generate_ul_ref_sigs_rx();

  generate_64qam_table();
  generate_16qam_table();
  generate_RIV_tables();

  init_unscrambling_lut();
  init_scrambling_lut();
  //set_taus_seed(1328);


}

void free_lte_top(void)
{
  free_codinglib();
  lte_sync_time_free();

  /* free_ul_ref_sigs() is called in phy_free_lte_eNB() */
}


/*
 * @}*/
