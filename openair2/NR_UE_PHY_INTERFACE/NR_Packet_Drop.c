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
#include <stdio.h>
#include "openair2/NR_UE_PHY_INTERFACE/NR_Packet_Drop.h"
#include "executables/softmodem-common.h"
#include "NR_MAC_UE/mac_proto.h"

slot_rnti_mcs_s slot_rnti_mcs[NUM_NFAPI_SLOT];
void read_channel_param(const nfapi_nr_dl_tti_pdsch_pdu_rel15_t * pdu, int slot, int index)
{
  if (pdu == NULL)
  {
    LOG_E(NR_MAC,"PDU NULL\n");
    abort();
  }

  /* This function is executed for every pdsch pdu type in a dl_tti_request. We save
     the assocaited MCS and RNTI value for each. The 'index' passed in is a count of
     how many times we have called this function for a particular dl_tti_request. It
     allows us to save MCS/RNTI data in correct indicies when multiple pdsch pdu's are received.*/
  for (int i = 0; i < NUM_NFAPI_SLOT; i++)
  {
    slot_rnti_mcs[i].latest = false;
  }

  CHECK_INDEX(slot_rnti_mcs[slot].rnti, index);
  CHECK_INDEX(slot_rnti_mcs[slot].mcs, index);
  CHECK_INDEX(slot_rnti_mcs[slot].drop_flag, index);
  slot_rnti_mcs[slot].rnti[index] = pdu->rnti;
  slot_rnti_mcs[slot].mcs[index] = pdu->mcsIndex[0];
  slot_rnti_mcs[slot].rvIndex[index] = pdu->rvIndex[0];
  slot_rnti_mcs[slot].drop_flag[index] = false;
  slot_rnti_mcs[slot].num_pdus = index+1; //index starts at 0 so we incrament to get num of pdus
  slot_rnti_mcs[slot].latest = true;
  LOG_D(NR_MAC, "Adding MCS %d and RNTI %x for slot_rnti_mcs[%d]\n", slot_rnti_mcs[slot].mcs[index], slot_rnti_mcs[slot].rnti[index], slot);

  return;
}

extern nr_bler_struct nr_bler_data[NR_NUM_MCS];
float get_bler_val(uint8_t mcs, int sinr)
{
  // 4th col = dropped packets, 5th col = total packets
  float bler_val = 0.0;
  CHECK_INDEX(nr_bler_data, mcs);
  LOG_D(NR_MAC, "sinr %d min %d max %d\n", sinr,
                (int)(nr_bler_data[mcs].bler_table[0][0] * 10),
                (int)(nr_bler_data[mcs].bler_table[nr_bler_data[mcs].length - 1][0] * 10)
  );

  if (sinr < (int)(nr_bler_data[mcs].bler_table[0][0] * 10))
  {
    LOG_D(NR_MAC, "MCS %d table. SINR is smaller than lowest SINR, bler_val is set based on lowest SINR in table\n", mcs);
    bler_val = nr_bler_data[mcs].bler_table[0][4] / nr_bler_data[mcs].bler_table[0][5];
    return bler_val;
  }
  if (sinr > (int)(nr_bler_data[mcs].bler_table[nr_bler_data[mcs].length - 1][0] * 10))
  {
    LOG_D(NR_MAC, "MCS %d table. SINR is greater than largest SINR. bler_val is set based on largest SINR in table\n", mcs);
    bler_val = nr_bler_data[mcs].bler_table[(nr_bler_data[mcs].length - 1)][4] / nr_bler_data[mcs].bler_table[(nr_bler_data[mcs].length - 1)][5];
    return bler_val;
  }
  // Loop through bler table to find sinr_index
  for (int i = 0; i < nr_bler_data[mcs].length; i++)
  {
    int temp_sinr = (int)(nr_bler_data[mcs].bler_table[i][0] * 10);
    if (temp_sinr == sinr)
    {
      bler_val = nr_bler_data[mcs].bler_table[i][4] / nr_bler_data[mcs].bler_table[i][5];
      return bler_val;
    }
    // Linear interpolation when SINR is between indices
    if (temp_sinr > sinr)
    {
      float bler_val1 = nr_bler_data[mcs].bler_table[i - 1][4] / nr_bler_data[mcs].bler_table[i - 1][5];
      float bler_val2 = nr_bler_data[mcs].bler_table[i][4] / nr_bler_data[mcs].bler_table[i][5];
      LOG_D(NR_MAC, "sinr %d min %f max %f\n", sinr, bler_val1, bler_val2);
      return ((bler_val1 + bler_val2) / 2);
    }
  }
  LOG_E(NR_MAC, "NO SINR INDEX FOUND!\n");
  abort();

}

bool is_channel_modeling(void)
{
  /* TODO: For now we enable channel modeling based on the node_number.
     Replace with a command line option to enable/disable channel modeling.
     The LTE UE will crash when channel modeling is conducted for NSA
     mode. It does not crash for LTE mode. We have not implemented channel
     modeling for NSA mode yet. For now, we ensure only do do channel modeling
     in LTE/NR mode. */
  return get_softmodem_params()->node_number == 0 && !get_softmodem_params()->nsa;
}

bool should_drop_transport_block(int slot, uint16_t rnti)
{
  if (!is_channel_modeling())
  {
    return false;
  }

  /* We want to avoid dropping setup messages because this would be pathological. */
  NR_UE_MAC_INST_t *mac = get_mac_inst(0);
  if (mac->ra.ra_state < RA_SUCCEEDED)
  {
    LOG_D(NR_MAC, "Not dropping because MAC state: %d", mac->ra.ra_state);
    return false;
  }

  /* Get block error rate (bler_val) from table based on every saved
     MCS and SINR to be used as the cutoff rate for dropping packets.
     Generate random uniform vairable to compare against bler_val. */
  int num_pdus = slot_rnti_mcs[slot].num_pdus;
  assert(slot < 20 && slot >= 0);
  LOG_D(NR_MAC, "rnti: %x  num_pdus %d state %d slot %u sinr %f\n",
        rnti, num_pdus, mac->ra.ra_state, slot, slot_rnti_mcs[slot].sinr);
  assert(num_pdus > 0);
  CHECK_INDEX(slot_rnti_mcs[slot].rnti, num_pdus);
  CHECK_INDEX(slot_rnti_mcs[slot].mcs, num_pdus);
  CHECK_INDEX(slot_rnti_mcs[slot].drop_flag, num_pdus);
  int n = 0;
  uint8_t mcs = 99;
  for (n = 0; n < num_pdus; n++)
  {
    if (slot_rnti_mcs[slot].rnti[n] == rnti)
    {
      mcs = slot_rnti_mcs[slot].mcs[n];
    }
    if (mcs != 99)
    {
      /* Use MCS to get the bler value. Since there can be multiple MCS
         values for a particular slot, we verify that all PDUs are not
         flagged for drop before returning. If even one is flagged for drop
         we return immediately because we drop the entire packet. */

      LOG_D(NR_MAC, "rnti: %x mcs %u slot %u sinr %f\n",
        slot_rnti_mcs[slot].rnti[n], mcs, slot, slot_rnti_mcs[slot].sinr);

      float bler_val;
      if (slot_rnti_mcs[slot].rvIndex[n] != 0)
        bler_val = 0;
      else
        bler_val = get_bler_val(mcs, ((int)(slot_rnti_mcs[slot].sinr * 10)));
      double drop_cutoff = ((double) rand() / (RAND_MAX));
      assert(drop_cutoff <= 1);
      LOG_D(NR_MAC, "SINR = %f, Bler_val = %f, MCS = %"PRIu8"\n", slot_rnti_mcs[slot].sinr, bler_val, slot_rnti_mcs[slot].mcs[n]);
      if (drop_cutoff <= bler_val)
      {
        slot_rnti_mcs[slot].drop_flag[n] = true;
        LOG_T(NR_MAC, "We are dropping this packet. Bler_val = %f, MCS = %"PRIu8", slot = %d\n", bler_val, slot_rnti_mcs[slot].mcs[n], slot);
        return slot_rnti_mcs[slot].drop_flag[n];
      }
    }

  }

  if (mcs == 99)
  {
    LOG_E(NR_MAC, "NO MCS Found for rnti %x. slot_rnti_mcs[%d].mcs[%d] \n", rnti, slot, n);
    abort();
  }
  return false;
}

int get_mcs_from_sinr(nr_bler_struct *nr_bler_data, float sinr)
{
  if (sinr < (nr_bler_data[0].bler_table[0][0])) {
    LOG_I(NR_MAC, "The SINR found is smaller than first MCS table\n");
    return 0;
  }

  if (sinr > (nr_bler_data[NR_NUM_MCS-1].bler_table[nr_bler_data[NR_NUM_MCS-1].length - 1][0])) {
    LOG_I(NR_MAC, "The SINR found is larger than last MCS table\n");
    return NR_NUM_MCS-1;
  }

  for (int n = NR_NUM_MCS-1; n >= 0; n--) {
    float largest_sinr = (nr_bler_data[n].bler_table[nr_bler_data[n].length - 1][0]);
    float smallest_sinr = (nr_bler_data[n].bler_table[0][0]);
    if (sinr < largest_sinr && sinr > smallest_sinr) {
      LOG_I(NR_MAC, "The SINR found in MCS %d table\n", n);
      return n;
    }
  }
  LOG_E(NR_MAC, "Unable to get an MCS value.\n");
  abort();
}
