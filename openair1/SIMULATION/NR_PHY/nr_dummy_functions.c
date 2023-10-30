#include "nfapi/oai_integration/vendor_ext.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface_scf.h"
#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"
#include "openair2/NR_UE_PHY_INTERFACE/NR_IF_Module.h"
#include "openair1/PHY/defs_nr_UE.h"


int oai_nfapi_dl_tti_req(nfapi_nr_dl_tti_request_t *dl_config_req) { return (0); }
int oai_nfapi_tx_data_req(nfapi_nr_tx_data_request_t *tx_data_req) { return (0); }
int oai_nfapi_ul_dci_req(nfapi_nr_ul_dci_request_t *ul_dci_req) { return (0); }
int oai_nfapi_ul_tti_req(nfapi_nr_ul_tti_request_t *ul_tti_req) { return (0); }
int oai_nfapi_nr_crc_indication(nfapi_nr_crc_indication_t *ind) { return (0); }
int oai_nfapi_nr_srs_indication(nfapi_nr_srs_indication_t *ind) { return (0); }
int oai_nfapi_nr_uci_indication(nfapi_nr_uci_indication_t *ind) { return (0); }
int oai_nfapi_nr_rach_indication(nfapi_nr_rach_indication_t *ind) { return (0); }
int oai_nfapi_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind) { return 0; }

int pack_nr_srs_beamforming_report(void *pMessageBuf, void *pPackedBuf, uint32_t packedBufLen) { return 0; }
int unpack_nr_srs_beamforming_report(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen) { return 0; }
int pack_nr_srs_normalized_channel_iq_matrix(void *pMessageBuf, void *pPackedBuf, uint32_t packedBufLen) { return 0; }
int unpack_nr_srs_normalized_channel_iq_matrix(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen) { return 0; }

int32_t get_uldl_offset(int nr_bandP) { return (0); }

void configure_nr_nfapi_pnf(char *vnf_ip_addr, int vnf_p5_port, char *pnf_ip_addr, int pnf_p7_port, int vnf_p7_port) {}
void configure_nr_nfapi_vnf(char *vnf_addr, int vnf_p5_port, char *pnf_ip_addr, int pnf_p7_port, int vnf_p7_port) {}
int nfapi_nr_p7_message_pack(void *pMessageBuf, void *pPackedBuf, uint32_t packedBufLen, nfapi_p7_codec_config_t* config) { return 0; }
int nfapi_nr_p7_message_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p7_codec_config_t* config) { return 0; }
int nfapi_p7_message_header_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p7_codec_config_t *config) { return 0; }

void nr_mac_rrc_sync_ind(const module_id_t module_id,
                         const frame_t frame,
                         const bool in_sync) {}

void nr_mac_rrc_msg3_ind(const module_id_t mod_id, int rnti) {}

void nr_mac_rrc_ra_ind(const module_id_t mod_id, int frame, bool success) {}

void rrc_data_ind(const protocol_ctxt_t *const ctxt_pP,
                  const rb_id_t                Srb_id,
                  const sdu_size_t             sdu_sizeP,
                  const uint8_t   *const       buffer_pP) { }

typedef uint32_t channel_t;
int8_t nr_mac_rrc_data_ind_ue(const module_id_t module_id,
                              const int CC_id,
                              const uint8_t gNB_index,
                              const frame_t frame,
                              const int slot,
                              const rnti_t rnti,
                              const channel_t channel,
                              const uint8_t* pduP,
                              const sdu_size_t pdu_len) { return 0; }
void *rrc_nrue(void *notUsed)
{
  return NULL;
}
