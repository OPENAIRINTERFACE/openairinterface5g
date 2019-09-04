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

#include "flexran_agent_rrc_internal.h"
#include "flexran_agent_ran_api.h"

int update_rrc_reconfig(mid_t mod_id, rnti_t rnti, Protocol__FlexRrcTriggering *trigg) {

  // Measurement info reconfiguration

  if (trigg->meas_info) {

    /* Set serving cell frequency offset */
    if (trigg->meas_info->has_offset_freq_serving) {
      if (flexran_set_rrc_ofp(mod_id, rnti, trigg->meas_info->offset_freq_serving) < 0) {
        LOG_E(FLEXRAN_AGENT, "Cannot set Serving cell frequency offset\n");
        return -1;
      }
    }

    /* Set neighbouring cell frequency offset */
    if (trigg->meas_info->has_offset_freq_neighbouring) {
      if (flexran_set_rrc_ofn(mod_id, rnti, trigg->meas_info->offset_freq_neighbouring) < 0) {
        LOG_E(FLEXRAN_AGENT, "Cannot set Neighbouring cell frequency offset\n");
        return -1;
      }
    }

    if (trigg->meas_info->n_cell_individual_offset > 0) {
      /* Set the serving cell offset */
      if (flexran_set_rrc_ocp(mod_id, rnti, trigg->meas_info->cell_individual_offset[0]) < 0) {
        LOG_E(FLEXRAN_AGENT, "Cannot set Serving cell offset\n");
        return -1;
      }

      /* Set the neighbouring cell offset */
      for (int i=0; i<(trigg->meas_info->n_cell_individual_offset-1); i++) {
        if (flexran_set_rrc_ocn(mod_id, rnti, i, trigg->meas_info->cell_individual_offset[i+1]) < 0) {
          LOG_E(FLEXRAN_AGENT, "Cannot set Neighbouring cell offset\n");
          return -1;
        }
      }
    }

    if (trigg->meas_info->has_offset_freq_neighbouring) {
      if (flexran_set_rrc_ofn(mod_id, rnti, trigg->meas_info->offset_freq_neighbouring) < 0)  {
        LOG_E(FLEXRAN_AGENT, "Cannot set Neighbouring cell frequency offset\n");
        return -1;
      }
    }


    /* Set rsrp filter coefficient */
    if (trigg->meas_info->has_filter_coefficient_rsrp) {
      if (flexran_set_filter_coeff_rsrp(mod_id, rnti, trigg->meas_info->filter_coefficient_rsrp) < 0) {
        LOG_E(FLEXRAN_AGENT, "Cannot set RSRP filter coefficient\n");
        return -1;
      }
    }

    /* Set rsrq filter coefficient */
    if (trigg->meas_info->has_filter_coefficient_rsrq) {
      if (flexran_set_filter_coeff_rsrq(mod_id, rnti, trigg->meas_info->filter_coefficient_rsrq) < 0) {
        LOG_E(FLEXRAN_AGENT, "Cannot set RSRQ filter coefficient\n");
        return -1;
      }
    }

    if (trigg->meas_info->event) {

      /* Set Periodic event parameters */
      if (trigg->meas_info->event->periodical) {

      /* Set Periodic event maximum number of reported cells */
        if (trigg->meas_info->event->periodical->has_max_report_cells) {
          if (flexran_set_rrc_per_event_maxReportCells(mod_id, rnti, trigg->meas_info->event->periodical->max_report_cells) < 0) {
            LOG_E(FLEXRAN_AGENT, "Cannot set Periodic event max\n");
            return -1;
          }
        }
      }

      /* Set A3 event parameters */
      if (trigg->meas_info->event->a3) {

      /* Set A3 event a3 offset */
        if (trigg->meas_info->event->a3->has_a3_offset) {
          if (flexran_set_rrc_a3_event_a3_offset(mod_id, rnti, trigg->meas_info->event->a3->a3_offset) < 0) {
            LOG_E(FLEXRAN_AGENT, "Cannot set A3 event offset\n");
            return -1;
          }
        }

      /* Set A3 event report on leave */
        if (trigg->meas_info->event->a3->has_report_on_leave) {
          if (flexran_set_rrc_a3_event_reportOnLeave(mod_id, rnti, trigg->meas_info->event->a3->report_on_leave) < 0) {
            LOG_E(FLEXRAN_AGENT, "Cannot set A3 event report on leave\n");
            return -1;
          }
        }

      /* Set A3 event hysteresis */
        if (trigg->meas_info->event->a3->has_hysteresis) {
          if (flexran_set_rrc_a3_event_hysteresis(mod_id, rnti, trigg->meas_info->event->a3->hysteresis) < 0) {
            LOG_E(FLEXRAN_AGENT, "Cannot set A3 event hysteresis\n");
            return -1;
          }
        }

      /* Set A3 event time to trigger */
        if (trigg->meas_info->event->a3->has_time_to_trigger) {
          if (flexran_set_rrc_a3_event_timeToTrigger(mod_id, rnti, trigg->meas_info->event->a3->time_to_trigger) < 0) {
            LOG_E(FLEXRAN_AGENT, "Cannot set A3 event time to trigger\n");
            return -1;
          }
        }

      /* Set A3 event maximum number of reported cells */
        if (trigg->meas_info->event->a3->has_max_report_cells) {
          if (flexran_set_rrc_a3_event_maxReportCells(mod_id, rnti, trigg->meas_info->event->a3->max_report_cells) < 0) {
            LOG_E(FLEXRAN_AGENT, "Cannot set A3 event max report cells\n");
            return -1;
          }
        }
      }
    }
  }
  return 0;
}
