/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "nr_mac_gNB.h"
#include "mac_proto.h"

nr_cell_sched_t *nr_mac_get_cell_by_phy_id(gNB_MAC_INST *mac, uint16_t phy_id)
{
  AssertFatal(phy_id < NR_MAX_CELLS, "Invalid phy_id %d\n", phy_id);
  AssertFatal(mac->cells[phy_id].common_channels.ServingCellConfigCommon != NULL, "Cell %d not configured!\n", phy_id);
  return &mac->cells[phy_id];
}

//TS 38.473 §9.3.1.12 NR CGI defined by PLMN Identity and NR Cell Identity
nr_cell_sched_t *nr_mac_get_cell_by_cgi(gNB_MAC_INST *mac, plmn_id_t plmn, uint64_t nr_cellid)
{
  for (int i = 0; i < NR_MAX_CELLS; i++) {
    nr_cell_sched_t *cell = &mac->cells[i];
    if (cell->common_channels.ServingCellConfigCommon == NULL)
      continue;
    if (cell->nr_cellid == nr_cellid && cell->plmn.mcc == plmn.mcc && cell->plmn.mnc == plmn.mnc
        && cell->plmn.mnc_digit_length == plmn.mnc_digit_length)
      return cell;
  }
  return NULL;
}
