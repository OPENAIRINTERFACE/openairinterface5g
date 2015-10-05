/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

#include <stdlib.h>
#include <string.h>

#if defined(NAS_BUILT_IN_EPC)
#include "assertions.h"
#include "tree.h"
#include "emmData.h"
#include "nas_log.h"
#include "security_types.h"

static inline
int emm_data_ctxt_compare_ueid(struct emm_data_context_s *p1,
                               struct emm_data_context_s *p2);

RB_PROTOTYPE(emm_data_context_map, emm_data_context_s, entries,
             emm_data_ctxt_compare_ueid);

/* Generate functions used for the MAP */
RB_GENERATE(emm_data_context_map, emm_data_context_s, entries,
            emm_data_ctxt_compare_ueid);

static inline
int emm_data_ctxt_compare_ueid(struct emm_data_context_s *p1,
                               struct emm_data_context_s *p2)
{
  if (p1->ueid > p2->ueid) {
    return 1;
  }

  if (p1->ueid < p2->ueid) {
    return -1;
  }

  /* Matching reference -> return 0 */
  return 0;
}

struct emm_data_context_s *emm_data_context_get(
  emm_data_t *emm_data, unsigned int _ueid)
{
  struct emm_data_context_s reference;

  DevAssert(emm_data != NULL);
  DevCheck(_ueid > 0, _ueid, 0, 0);

  memset(&reference, 0, sizeof(struct emm_data_context_s));
  reference.ueid = _ueid;
  return RB_FIND(emm_data_context_map, &emm_data->ctx_map, &reference);
}

struct emm_data_context_s *emm_data_context_remove(
  emm_data_t *emm_data, struct emm_data_context_s *elm)
{
  LOG_TRACE(INFO, "EMM-CTX - Remove in context %p UE id "NAS_UE_ID_FMT" ", elm, elm->ueid);
  return RB_REMOVE(emm_data_context_map, &emm_data->ctx_map, elm);
}

void emm_data_context_add(emm_data_t *emm_data, struct emm_data_context_s *elm)
{
  LOG_TRACE(INFO, "EMM-CTX - Add in context %p UE id "NAS_UE_ID_FMT" ", elm, elm->ueid);
  RB_INSERT(emm_data_context_map, &emm_data->ctx_map, elm);
}

void emm_data_context_dump(struct emm_data_context_s *elm_pP)
{
  if (elm_pP != NULL) {
	    char imsi_str[16];
	    char guti_str[22];
	    int k, size, remaining_size;
        char key_string[KASME_LENGTH_OCTETS*2];
	    LOG_TRACE(INFO, "EMM-CTX: ue id:           "NAS_UE_ID_FMT" (UE identifier)",                     elm_pP->ueid);
	    LOG_TRACE(INFO, "         is_dynamic:       %u      (Dynamically allocated context indicator)", elm_pP->is_dynamic);
	    LOG_TRACE(INFO, "         is_attached:      %u      (Attachment indicator)",                    elm_pP->is_attached);
	    LOG_TRACE(INFO, "         is_emergency:     %u      (Emergency bearer services indicator)",     elm_pP->is_emergency);
	    NAS_IMSI2STR(elm_pP->imsi, imsi_str, 16);
	    LOG_TRACE(INFO, "         imsi:             %s      (The IMSI provided by the UE or the MME)",  imsi_str);
	    LOG_TRACE(INFO, "         imei:             TODO    (The IMEI provided by the UE)");
	    LOG_TRACE(INFO, "         guti_is_new:      %u      (New GUTI indicator)",                      elm_pP->guti_is_new);
	    GUTI2STR(elm_pP->guti, guti_str, 22);
	    LOG_TRACE(INFO, "         guti:             %s      (The GUTI assigned to the UE)",             guti_str);
	    GUTI2STR(elm_pP->old_guti, guti_str, 22);
	    LOG_TRACE(INFO, "         old_guti:         %s      (The old GUTI)",                            guti_str);
	    LOG_TRACE(INFO, "         n_tacs:           %u      (Number of consecutive tracking areas the UE is registered to)", elm_pP->n_tacs);
	    LOG_TRACE(INFO, "         tac:              0x%04x  (Code of the first tracking area the UE is registered to)",      elm_pP->n_tacs);
	    LOG_TRACE(INFO, "         ksi:              %u      (Security key set identifier provided by the UE)",               elm_pP->ksi);

	    LOG_TRACE(INFO, "         auth_vector:              (EPS authentication vector)");
	    LOG_TRACE(INFO, "             kasme: "KASME_FORMAT""KASME_FORMAT, KASME_DISPLAY_1(elm_pP->vector.kasme), KASME_DISPLAY_2(elm_pP->vector.kasme));
	    LOG_TRACE(INFO, "             rand:  "RAND_FORMAT, RAND_DISPLAY(elm_pP->vector.rand));
	    LOG_TRACE(INFO, "             autn:  "AUTN_FORMAT, AUTN_DISPLAY(elm_pP->vector.autn));
        for (k = 0; k < XRES_LENGTH_MAX; k++) {sprintf(&key_string[k * 3], "%02x,", elm_pP->vector.xres[k]);}
        key_string[k * 3 - 1] = '\0';
	    LOG_TRACE(INFO, "             xres:  %s\n", key_string);

	    if (elm_pP->security != NULL) {
		  LOG_TRACE(INFO, "         security context:          (Current EPS NAS security context)");
	      LOG_TRACE(INFO, "             type:  %s              (Type of security context)",
	    		(elm_pP->security->type == EMM_KSI_NOT_AVAILABLE)?"KSI_NOT_AVAILABLE":(elm_pP->security->type == EMM_KSI_NATIVE)?"KSI_NATIVE":"KSI_MAPPED");
	      LOG_TRACE(INFO, "             eksi:  %u              (NAS key set identifier for E-UTRAN)", elm_pP->security->eksi);

	      if (elm_pP->security->kasme.length > 0) {
	    	size = 0;
		    size = 0;remaining_size=KASME_LENGTH_OCTETS*2;
	        for (k = 0; k < elm_pP->security->kasme.length; k++) {
	          size +=snprintf(&key_string[size], remaining_size, "0x%x ", elm_pP->security->kasme.value[k]);
	          remaining_size -= size;
	        }
	      } else {
	          size +=snprintf(&key_string[0], remaining_size, "None");
	      }
		  LOG_TRACE(INFO, "             kasme: %s     (ASME security key (native context))", key_string);
	      if (elm_pP->security->knas_enc.length > 0) {
		    size = 0;
		    size = 0;remaining_size=KASME_LENGTH_OCTETS*2;
	        for (k = 0; k < elm_pP->security->knas_enc.length; k++) {
	          size +=snprintf(&key_string[size], remaining_size, "0x%x ", elm_pP->security->knas_enc.value[k]);
	          remaining_size -= size;
	        }
	      } else {
	          size +=snprintf(&key_string[0], remaining_size, "None");
	      }
		  LOG_TRACE(INFO, "             knas_enc: %s     (NAS cyphering key)", key_string);
	      if (elm_pP->security->knas_int.length > 0) {
		    size = 0;remaining_size=KASME_LENGTH_OCTETS*2;
	        for (k = 0; k < elm_pP->security->knas_int.length; k++) {
	          size +=snprintf(&key_string[size], remaining_size, "0x%x ", elm_pP->security->knas_int.value[k]);
	          remaining_size -= size;
	        }
	      } else {
	          size +=snprintf(&key_string[0], remaining_size, "None");
	      }
		  LOG_TRACE(INFO, "             knas_int: %s     (NAS integrity key)", key_string);
	      LOG_TRACE(INFO, "             dl_count.overflow: %u     ", elm_pP->security->dl_count.overflow);
	      LOG_TRACE(INFO, "             dl_count.seq_num:  %u     ", elm_pP->security->dl_count.seq_num);
	      LOG_TRACE(INFO, "             ul_count.overflow: %u     ", elm_pP->security->ul_count.overflow);
	      LOG_TRACE(INFO, "             ul_count.seq_num:  %u     ", elm_pP->security->ul_count.seq_num);
	      LOG_TRACE(INFO, "             TODO  capability");
	      LOG_TRACE(INFO, "             selected_algorithms.encryption:  %x     ", elm_pP->security->selected_algorithms.encryption);
	      LOG_TRACE(INFO, "             selected_algorithms.integrity:   %x     ", elm_pP->security->selected_algorithms.integrity);
	    } else {
		  LOG_TRACE(INFO, "         No security context");
	    }
	    LOG_TRACE(INFO, "         _emm_fsm_status     %u   ", elm_pP->_emm_fsm_status);
	    LOG_TRACE(INFO, "         TODO  esm_data_ctx");
  }
}

void emm_data_context_dump_all(void)
{
  struct emm_data_context_s *elm_p = NULL;
  LOG_TRACE(INFO, "EMM-CTX - Dump all contexts:");
  RB_FOREACH(elm_p, emm_data_context_map,&_emm_data.ctx_map) {
	  emm_data_context_dump(elm_p);
  }
}
#endif
