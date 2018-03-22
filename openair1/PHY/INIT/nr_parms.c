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

#include "defs.h"
#include "defs_NR.h"
#include "log.h"


int nr_init_frame_parms(LTE_DL_FRAME_PARMS *frame_parms)
{

#if DISABLE_LOG_X
  printf("Initializing frame parms for mu %d, N_RB_DL %d, Ncp %d, osf %d\n",frame_parms->N_RB_DL,frame_parms->Ncp,osf);
#else
  LOG_I(PHY,"Initializing frame parms for mu %d, N_RB_DL %d, Ncp %d, osf %d\n",frame_parms->N_RB_DL,frame_parms->Ncp,osf);
#endif

  if (frame_parms->Ncp == EXTENDED)
    AssertFatal(frame_parms->mu == NR_MU_2,"Invalid cyclic prefix %d for numerology index %d\n", frame_parms->Ncp, frame_parms->mu);

  switch(frame_parms->mu) {

    case NR_MU_0: //15kHz scs
      frame_parms->scs = nr_subcarrier_spacing[NR_MU_0];
      break;

    case NR_MU_1: //30kHz scs
      frame_parms->scs = nr_subcarrier_spacing[NR_MU_1];

      switch(frame_parms->N_RB_DL){
        case 11:
        case 24:
        case 38:
        case 78:
        case 51:
        case 65:

        case 106: //40 MHz
          frame_parms->ofdm_symbol_size = 2048;
          frame_parms->samples_per_tti = 30720;
          frame_parms->first_carrier_offset = 1412; //2048 - 636
          frame_parms->nb_prefix_samples0 = 160;
          frame_parms->nb_prefix_samples = 144;
          break;

        case 133:
        case 162:
        case 189:

        case 217: //80 MHz
          if (frame_parms->threequarter_fs) {
            frame_parms->ofdm_symbol_size = 3072;
            frame_parms->samples_per_tti = 46080;
            frame_parms->first_carrier_offset = 1770; //3072 - 1302
            frame_parms->nb_prefix_samples0 = 240;
            frame_parms->nb_prefix_samples = 216;
          }
          else {
            frame_parms->ofdm_symbol_size = 4096;
            frame_parms->samples_per_tti = 61440;
            frame_parms->first_carrier_offset = 2794; //4096 - 1302
            frame_parms->nb_prefix_samples0 = 320;
            frame_parms->nb_prefix_samples = 288;
          }
          break;

        case 245:
        case 273:
      default:
        AssertFatal(1==0,"Number of resource blocks %d undefined for mu %d, frame parms = %p\n", frame_parms->N_RB_DL, frame_parms->mu, frame_parms);
      }
      break;

    case NR_MU_2: //60kHz scs
      frame_parms->scs = nr_subcarrier_spacing[NR_MU_2];

      switch(frame_parms->N_RB_DL){ //FR1 bands only
        case 11:
        case 18:
        case 38:
        case 24:
        case 31:
        case 51:
        case 65:
        case 79:
        case 93:
        case 107:
        case 121:
        case 135:
      default:
        AssertFatal(1==0,"Number of resource blocks %d undefined for mu %d, frame parms = %p\n", frame_parms->N_RB_DL, frame_parms->mu, frame_parms);
      }
      break;

    case NR_MU_3:
      frame_parms->scs = nr_subcarrier_spacing[NR_MU_3];
      break;

    case NR_MU_4:
      frame_parms->scs = nr_subcarrier_spacing[NR_MU_4];
      break;

  default:
    AssertFatal(1==0,"Invalid numerology index %d", frame_parms->mu);
  }

  return 0;
}

void nr_dump_frame_parms(LTE_DL_FRAME_PARMS *frame_parms)
{
  LOG_I(PHY,"frame_parms->mu=%d\n",frame_parms->mu);
  LOG_I(PHY,"frame_parms->scs=%d\n",frame_parms->scs);
  LOG_I(PHY,"frame_parms->N_RB_DL=%d\n",frame_parms->N_RB_DL);
  LOG_I(PHY,"frame_parms->ofdm_symbol_size=%d\n",frame_parms->ofdm_symbol_size);
  LOG_I(PHY,"frame_parms->samples_per_tti=%d\n",frame_parms->samples_per_tti);
  LOG_I(PHY,"frame_parms->nb_prefix_samples0=%d\n",frame_parms->nb_prefix_samples0);
  LOG_I(PHY,"frame_parms->nb_prefix_samples=%d\n",frame_parms->nb_prefix_samples);
}
