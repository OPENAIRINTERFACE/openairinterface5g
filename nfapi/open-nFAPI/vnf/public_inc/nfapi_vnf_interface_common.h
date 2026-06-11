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

typedef struct vnf_s vnf_t;
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

typedef struct nfapi_vnf_config nfapi_vnf_config_t;

/*! The nfapi VNF configuration information
 */
typedef struct nfapi_vnf_config
{
	/*! A user define callback to override the default memory allocation */
	void* (*malloc)(size_t size);
	/*! A user define callback to override the default memory deallocation */
	void (*free)(void*);

	/*! The port the VNF P5 SCTP connection listens on
	 *
	 *  By default this will be set to 7701. However this can be changed if
	 *  required.
	 */
	int vnf_p5_port;

	// todo : for enabling ipv4/ipv6
	int vnf_ipv4;
	int vnf_ipv6;

	/*! List of connected pnfs */
	nfapi_vnf_pnf_info_t* pnf_list;

	/*! List of configured phys */
	nfapi_vnf_phy_info_t* phy_list;

	/*! Configuration options for the p4 p5 pack unpack functions */
	nfapi_p4_p5_codec_config_t codec_config;

	/*! Optional user defined data that will be avaliable in the callbacks*/
	void* user_data;

	int (*pnf_nr_connection_indication)(nfapi_vnf_config_t* config, int p5_idx);
	int (*pnf_connection_indication)(nfapi_vnf_config_t* config, int p5_idx);

	int (*pnf_disconnect_indication)(nfapi_vnf_config_t* config, int p5_idx);

	int (*pnf_nr_param_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_param_response_t* resp);
	int (*pnf_param_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_param_response_t* resp);

	int (*pnf_config_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_config_response_t* resp);
	int (*pnf_nr_config_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_config_response_t* resp);

	int (*pnf_start_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_start_response_t* resp);
	int (*pnf_nr_start_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_start_response_t* resp);

  int (*nr_stop_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_stop_indication_scf_t* ind);

	int (*pnf_stop_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_stop_response_t* resp);

	int (*param_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_param_response_t* resp);
	int (*nr_param_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_param_response_scf_t* resp);

	int (*nr_config_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_config_response_scf_t* resp);
	int (*config_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_config_response_t* resp);

	int (*start_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_start_response_t* resp);
	int (*nr_start_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_start_response_scf_t* resp);

	int (*nr_error_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_error_indication_scf_t* ind);

	int (*stop_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_stop_response_t* resp);

	int (*measurement_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_measurement_response_t* resp);

	int (*rssi_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_rssi_response_t* resp);
	int (*rssi_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_rssi_indication_t* ind);
	int (*cell_search_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_cell_search_response_t* resp);
	int (*cell_search_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_cell_search_indication_t* ind);
	int (*broadcast_detect_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_broadcast_detect_response_t* resp);
	int (*broadcast_detect_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_broadcast_detect_indication_t* ind);
	int (*system_information_schedule_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_schedule_response_t* resp);
	int (*system_information_schedule_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_schedule_indication_t* ind);
	int (*system_information_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_response_t* resp);
	int (*system_information_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_indication_t* ind);
	int (*nmm_stop_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nmm_stop_response_t* resp);

	int (*vendor_ext)(nfapi_vnf_config_t* config, int p5_idx, void* msg);

	void* (*allocate_p4_p5_vendor_ext)(uint16_t message_id, uint16_t* msg_size);
	void (*deallocate_p4_p5_vendor_ext)(void* header);

  int (*pack_func)(void* pMessageBuf,
                   uint32_t messageBufLen,
                   void* pPackedBuf,
                   uint32_t packedBufLen,
                   nfapi_p4_p5_codec_config_t* config);

  bool (*unpack_func)(void* pMessageBuf,
                      uint32_t messageBufLen,
                      void* pUnpackedBuf,
                      uint32_t unpackedBufLen,
                      nfapi_p4_p5_codec_config_t* config);

  bool (*hdr_unpack_func)(void* pMessageBuf,
                          uint32_t messageBufLen,
                          void* pUnpackedBuf,
                          uint32_t unpackedBufLen,
                          nfapi_p4_p5_codec_config_t* config);

  bool (*send_p5_msg)(vnf_t* vnf, uint16_t p5_idx, nfapi_nr_p4_p5_message_header_t* msg, uint32_t msg_len);


} nfapi_vnf_config_t;

nfapi_vnf_config_t* nfapi_vnf_config_create(void);
void nfapi_vnf_config_destory(nfapi_vnf_config_t* config);
int nfapi_vnf_start(nfapi_vnf_config_t* config);
int nfapi_vnf_stop(nfapi_vnf_config_t* config);
int nfapi_vnf_allocate_phy(nfapi_vnf_config_t* config, int p5_idx, uint16_t* phy_id);
int nfapi_vnf_vendor_extension(nfapi_vnf_config_t* config, int p5_idx, nfapi_p4_p5_message_header_t* msg);

//-----------------------------------------------------------------------------

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
