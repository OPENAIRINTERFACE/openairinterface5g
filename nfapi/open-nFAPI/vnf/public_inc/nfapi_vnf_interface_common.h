/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#ifndef _NFAPI_VNF_INTERFACE_COMMON_H_
#define _NFAPI_VNF_INTERFACE_COMMON_H_

#include "nfapi_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "nfapi_nr_interface.h"
#include "openair2/PHY_INTERFACE/queue_t.h"

#include "debug.h"

#include "netinet/in.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct vnf_p7_s vnf_p7_t;

/*! The nfapi VNF phy configuration information
 */
typedef struct nfapi_vnf_phy_info
{
	/*! The P5 Index */
	int p5_idx;	//which p5 connection
	/*! The PHY ID */
	int phy_id; //phy_id

	/*! Timing window */
	uint8_t timing_window;
	/*! Timing info mode */
	uint8_t timing_info_mode;
	/*! Timing info period */
	uint8_t timing_info_period;

	/*! P7 UDP socket information for the pnf */
	struct sockaddr_in p7_pnf_address;
	/*! P7 UDP socket information for the vnf */
	struct sockaddr_in p7_vnf_address;

	struct nfapi_vnf_phy_info* next;
} nfapi_vnf_phy_info_t;

/*! The nfapi VNF pnf configuration information
 */
typedef struct nfapi_vnf_pnf_info
{
	/*! The P5 Index */
	int p5_idx;
	/*! The P5 socket */
	int p5_sock;
	/*! Flag indicating if the pnf is connected */
	uint8_t connected;

	/*! P5 SCTP socket information for the pnf */
	struct sockaddr_in p5_pnf_sockaddr;

	/*! Flag indicating if this pnf should be deleted */
	uint8_t to_delete;

	struct nfapi_vnf_pnf_info* next;

} nfapi_vnf_pnf_info_t;

/*! The nfapi VNF P7 connection information
 */
typedef struct nfapi_vnf_p7_config nfapi_vnf_p7_config_t;

/*! The nfapi VNF P7 configuration information
 */
typedef struct nfapi_vnf_p7_config
{
	void* (*malloc)(size_t size);
	void (*free)(void*);

	int port;
	uint8_t checksum_enabled;
	uint32_t segment_size;
	uint16_t max_num_segments;

	nfapi_p7_codec_config_t codec_config;

	int (*sync_indication)(struct nfapi_vnf_p7_config* config, uint8_t sync);
	int (*subframe_indication)(struct nfapi_vnf_p7_config* config, uint16_t phy_id, uint16_t sfn_sf);
	int (*slot_indication)(struct nfapi_vnf_p7_config* config, uint16_t phy_id, uint16_t sfn, uint16_t slot);

	int (*harq_indication)(struct nfapi_vnf_p7_config* config, nfapi_harq_indication_t* ind);
	int (*crc_indication)(struct nfapi_vnf_p7_config* config, nfapi_crc_indication_t* ind);
	int (*rx_indication)(struct nfapi_vnf_p7_config* config, nfapi_rx_indication_t* ind);
	int (*rach_indication)(struct nfapi_vnf_p7_config* config, nfapi_rach_indication_t* ind);
	int (*srs_indication)(struct nfapi_vnf_p7_config* config, nfapi_srs_indication_t* ind);
	int (*sr_indication)(struct nfapi_vnf_p7_config* config, nfapi_sr_indication_t* ind);
	int (*cqi_indication)(struct nfapi_vnf_p7_config* config, nfapi_cqi_indication_t* ind);
	int (*lbt_dl_indication)(struct nfapi_vnf_p7_config* config, nfapi_lbt_dl_indication_t* ind);
	int (*nb_harq_indication)(struct nfapi_vnf_p7_config* config, nfapi_nb_harq_indication_t* ind);
	int (*nrach_indication)(struct nfapi_vnf_p7_config* config, nfapi_nrach_indication_t* ind);

	int (*nr_slot_indication)(nfapi_nr_slot_indication_scf_t* ind);
	int (*nr_crc_indication)(nfapi_nr_crc_indication_t* ind);
	int (*nr_rx_data_indication)(nfapi_nr_rx_data_indication_t* ind);
	int (*nr_uci_indication)(nfapi_nr_uci_indication_t* ind);
	int (*nr_rach_indication)(nfapi_nr_rach_indication_t* ind);
	int (*nr_srs_indication)(nfapi_nr_srs_indication_t* ind);
        /* A callback for SRS ToA vendor extension message
         * \param ind A data structure for the SRS ToA vendor extension
         * \return not currently used.
         */
	int (*nr_srs_toa_vendor_ext_indication)(nfapi_nr_srs_toa_vendor_ext_indication_t* ind);
   

	int (*vendor_ext)(struct nfapi_vnf_p7_config* config, void* msg);
	void* user_data;
	void* (*allocate_p7_vendor_ext)(uint16_t message_id, uint16_t* msg_size);
	void (*deallocate_p7_vendor_ext)(void* header);

	int (*pack_func)(void* pMessageBuf, void* pPackedBuf, uint32_t packedBufLen, nfapi_p7_codec_config_t* config);

	bool (*unpack_func)(void* pMessageBuf,
					   uint32_t messageBufLen,
					   void* pUnpackedBuf,
					   uint32_t unpackedBufLen,
					   nfapi_p7_codec_config_t* config);

	bool (*hdr_unpack_func)(void* pMessageBuf,
						   uint32_t messageBufLen,
						   void* pUnpackedBuf,
						   uint32_t unpackedBufLen,
						   nfapi_p7_codec_config_t* config);

  bool (*send_p7_msg)(vnf_p7_t* vnf_p7, nfapi_nr_p7_message_header_t* header);
} nfapi_vnf_p7_config_t;

typedef nfapi_vnf_p7_config_t nfapi_lte_vnf_p7_config_t;
typedef nfapi_vnf_p7_config_t nfapi_nr_vnf_p7_config_t;

nfapi_vnf_p7_config_t* nfapi_vnf_p7_config_create(void);
void nfapi_vnf_p7_config_destory(nfapi_vnf_p7_config_t* config);
int nfapi_vnf_p7_stop(nfapi_vnf_p7_config_t* config);
int nfapi_vnf_p7_add_pnf(nfapi_vnf_p7_config_t* config, const char* pnf_p7_addr, int pnf_p7_port, int phy_id, int mu);
int nfapi_vnf_p7_del_pnf(nfapi_vnf_p7_config_t* config, int phy_id);
int nfapi_vnf_p7_vendor_extension(nfapi_vnf_p7_config_t* config, nfapi_p7_message_header_t* msg);
int nfapi_vnf_p7_release_pdu(nfapi_vnf_p7_config_t* config, void*);

#if defined(__cplusplus)
}
#endif

#endif // _NFAPI_VNF_INTERFACE_COMMON_H_
