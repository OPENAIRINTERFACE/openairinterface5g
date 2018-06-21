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

/*! \file PHY/NR_TRANSPORT/nr_dci.c
* \brief Implements DCI encoding/decoding (38.212/38.213/38.214). Current NR compliance V15.1 2018-06.
* \author Guy De Souza
* \date 2018
* \version 0.1
* \company Eurecom
* \email: desouza@eurecom.fr
* \note
* \warning
*/

#include "nr_dci.h"

uint8_t nr_get_dci_size(nr_dci_format_e format,
                        nr_rnti_type_e rnti,
                        NR_BWP_PARMS* bwp,
                        nfapi_nr_config_request_t* config)
{
  uint8_t size = 0;
  uint16_t N_RB = bwp->N_RB;

  switch(format) {

    case nr_dci_format_0_0:
      /// fixed: Format identifier 1, Hop flag 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2 --16
      size += 16;
      size += ceil( log2( (N_RB*(N_RB+1))>>2 ) ); // Freq domain assignment -- hopping scenario to be updated
      // Time domain assignment
      // UL/SUL indicator
      break;

    case nr_dci_format_0_1:
      /// fixed: Format identifier 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2, SRS request 2 --17
      size += 17;
      // Carrier indicator
      // UL/SUL indicator
      // BWP Indicator
      // Freq domain assignment
      // Time domain assignment
      // VRB to PRB mapping
      // Frequency Hopping flag
      // 1st DAI
      // 2nd DAI
      // SRS resource indicator
      // Precoding info and number of layers
      // Antenna ports
      // CSI request
      // CBGTI
      // PTRS - DMRS association
      // beta offset indicator
      // DMRS sequence init
      break;

    case nr_dci_format_1_0:
      /// fixed: Format identifier 1, VRB2PRB 1, MCS 5, NDI 1, RV 2, HARQ PID 4, DAI 2, PUCCH TPC 2, PUCCH RInd 3, PDSCH to HARQ TInd 3 --24
      size += 24;
      size += ceil( log2( (N_RB*(N_RB+1))>>2 ) ); // Freq domain assignment
      // Time domain assignment
      break;

    case nr_dci_format_1_1:
      // Carrier indicator
      size += 1; // Format identifier
      // BWP Indicator
      // Freq domain assignment
      // Time domain assignment
      // VRB to PRB mapping
      // PRB bundling size indicator
      // Rate matching indicator
      // ZP CSI-RS trigger
        /// TB1- MCS 5, NDI 1, RV 2
      size += 8;
      // TB2
      size += 4 ;  // HARQ PID
      // DAI
      size += 2; // TPC PUCCH
      size += 3; // PUCCH resource indicator
      size += 3; // PDSCH to HARQ timing indicator
      // Antenna ports
      // Tx Config Indication
      size += 2; // SRS request
      // CBGTI
      // CBGFI
      size += 1; // DMRS sequence init
      
      break;

    case nr_dci_format_2_0:
      break;

    case nr_dci_format_2_1:
      break;

    case nr_dci_format_2_2:
      break;

    case nr_dci_format_2_3:
      break;

  default:
    AssertFatal(1==0, "Invalid NR DCI format %d\n", format);
  }

  return size;
}


uint8_t nr_generate_dci_top(NR_DCI_ALLOC_t dci_alloc,
                            int32_t** txdataF,
                            int16_t amp,
                            NR_DL_FRAME_PARMS* frame_parms,
                            nfapi_config_request_t* config)
{
  return 0;
}
