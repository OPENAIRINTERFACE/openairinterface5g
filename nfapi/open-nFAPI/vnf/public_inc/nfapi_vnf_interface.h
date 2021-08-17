/*
 * Copyright 2017 Cisco Systems, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NFAPI_VNF_INTERFACE_H_
#define _NFAPI_VNF_INTERFACE_H_

#include "nfapi_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "nfapi_nr_interface.h"

#include "debug.h"

#include "netinet/in.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NFAPI_MAX_PACKED_MESSAGE_SIZE 8192

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

	/*! \brief Callback indicating that a pnf has established connection 
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 used to indicate this pnf p5 connection
	 * 
	 *  The client is expected to send the PNF_PARAM.request in response to 
	 *  the connection indication
	 * 
	 *  \todo Do we need to send the address information of the PNF?
	 */
	int (*pnf_nr_connection_indication)(nfapi_vnf_config_t* config, int p5_idx);
	int (*pnf_connection_indication)(nfapi_vnf_config_t* config, int p5_idx);
	
	/*! \brief Callback indicating that a pnf has lost connection 
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 used to indicate this pnf p5 connection
	 *
	 *  The p5_idx may be used for future new p5 connections
	 *
	 *  The client is responsiable for communicating to the p7 instance(s) that
	 *  may be associated with this p5 that they should be deleted using the 
	 *  nfapi_vnf_p7_del_pnf functon
	 * 
	 */
	int (*pnf_disconnect_indication)(nfapi_vnf_config_t* config, int p5_idx);

	// p5 interface functions
	
	/*! \brief A callback to handle the PNF_PARAM.resp
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded PNF_PARAM.response. This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 *  
	 *  The PNF_PARAM.resp contains the capability of the PNF identified by the
	 *  p5_idx. 
	 *  
	 *  The client is expected to send the PNF_CONFIG.request after receiving the
	 *  PNF_PARAM.resp. This can be done in the call back. 
	 *
	 *  It is expected that the client when building the PNF_CONFIG.request will
	 *  used the nfapi_vnf_allocate_phy() to allocate unique phy id for each FAPI
	 *  instance the client wishes to create.
	 *  
	 *  The resp may contain pointers to dyanmically allocated sub structures  
	 *  such as the vendor_extention. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */
	int (*pnf_nr_param_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_param_response_t* resp);
	int (*pnf_param_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_param_response_t* resp);
	
	/*! A callback for the PNF_CONFIG.resp 
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded PNF_CONFIG.response. This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 *  
	 *  The PNF_CONFIG.resp contains the result of the PNF_CONFIG.request for the 
	  * PNF identified by the p5_idx. 
	 *  
	 *  The client is expected to send the PNF_START.request after receiving the
	 *  PNF_PARAM.resp. This can be done in the call back. 
	 *  
	 *  The resp may contain pointers to dyanmically allocated sub structures  
	 *  such as the vendor_extention. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */
	int (*pnf_config_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_config_response_t* resp);
	int (*pnf_nr_config_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_config_response_t* resp);

	/*! A callback for the PNF_START.resp
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded PNF_START.response. This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 *  
	 *  The PNF_START.resp contains the result of the PNF_START.request for the 
	 *  PNF identified by the p5_idx. 
	 *  
	 *  The client is expected to send the PARAM.request for each FAPI instance 
	 *  that has been created in the PNF. 
	 *  
	 *  The resp may contain pointers to dyanmically allocated sub structures  
	 *  such as the vendor_extention. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */
	int (*pnf_start_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_start_response_t* resp);
	int (*pnf_nr_start_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_start_response_t* resp);

	/*! A callback for the PNF_STOP.resp
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded PNF_STOP.response. This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 *  
	 *  The PNF_STOP.response contains the result of the PNF_STOP.request for the 
     *  PNF identified by the p5_idx. 
	 *  
	 *  The resp may contain pointers to dyanmically allocated sub structures  
	 *  such as the vendor_extention. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */
	int (*pnf_stop_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_stop_response_t* resp);
	
	/*! A callback for the PARAM.resp
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded PARAM.resposne. This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 *  
	 *  The PARAM.request contains the capabilities of the FAPI instance identified
	 *  by the phy_id
	 *
	 *  The client is expected to send the CONFIG.request after receiving the
	 *  PARAM.response. This can be done in the call back. The PARAM.response 
	 *  contains the PNF P7 address (ipv4 or ipv6) and port. This information 
	 *  is used when calling the nfapi_vnf_p7_add_pnf()
	 * 
	 *  The client is responsible for identifing the VNF P7 ip address 
	 *  (ipv4 or ipv6) and port for the VNF P7 entity which will be sent to the
	 *  PNF P7 entity. That endpoint should be valid before send the 
	 * CONFIG.request.
	 * 
	 *  The resp may contain pointers to dyanmically allocated sub structures  
	 *  such as the vendor_extention. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */
	int (*param_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_param_response_t* resp);
	int (*nr_param_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_param_response_scf_t* resp);
	
	/*! A callback for the CONFIG.response
     *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded CONFIG.response This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 *  
	 *  The CONFIG.response contains the result of the CONFIG.request
	 *
	 * 
	 *  The resp may contain pointers to dyanmically allocated sub structures  
	 *  such as the vendor_extention. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */
	int (*nr_config_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_config_response_scf_t* resp);
	int (*config_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_config_response_t* resp);

	/*! A callback for the START.resp
     *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded START.response This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 *  
	 *  The START.response contains the result of the START.request
	 *
	 * 
	 *  The resp may contain pointers to dyanmically allocated sub structures  
	 *  such as the vendor_extention. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */
	int (*start_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_start_response_t* resp);
	int (*nr_start_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_start_response_scf_t* resp);
	
	/*! A callback for the STOP.resp
     *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded STOP.response This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 *  
	 *  The STOP.response contains the result of the STOP.request
	 *
	 * 
	 *  The resp may contain pointers to dyanmically allocated sub structures  
	 *  such as the vendor_extention. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*stop_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_stop_response_t* resp);
	 
	/*! A callback for the MEASUREMENT.resp 
     *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded MEASUREMENT.response This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 *  
	 *  The MEASUREMENT.response contains the result of the MEASUREMENT.request
	 * 
	 *  The resp may contain pointers to dyanmically allocated sub structures  
	 *  such as the vendor_extention. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*measurement_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_measurement_response_t* resp);

	// p4 interface functions
	/*! A callback for the RSSI.resp 
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded RSSI.response This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 */
	int (*rssi_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_rssi_response_t* resp);
	
	/*! A callback for the RSSI.indication
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded RSSI.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 */
	int (*rssi_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_rssi_indication_t* ind);
	
	/*! A callback for the CELL_SEARCH.response
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded CELL_SEARCH.response This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 */
	int (*cell_search_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_cell_search_response_t* resp);
	
	/*! A callback for the CELL_SEARCH.indication
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded CELL_SEARCH.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 */
	int (*cell_search_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_cell_search_indication_t* ind);
	
	/*! A callback for the BROADCAST_DETECT.response
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded BROADCAST_DETECT.response This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.	 */
	int (*broadcast_detect_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_broadcast_detect_response_t* resp);
	
	/*! A callback for the BROADCAST_DETECT.indication
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded BROADCAST_DETECT.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.	 
	 */
	int (*broadcast_detect_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_broadcast_detect_indication_t* ind);
	
	/*! A callback for the SYSTEM_INFORMATION_SCHEUDLE.response
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded SYSTEM_INFORMATION_SCHEUDLE.response This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.	 
	 */
	int (*system_information_schedule_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_schedule_response_t* resp);
	
	/*! A callback for the SYSTEM_INFORMATION_SCHEUDLE.indication
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded SYSTEM_INFORMATION_SCHEUDLE.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.	
	 */
	int (*system_information_schedule_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_schedule_indication_t* ind);
	
	/*! A callback for the SYSTEM_INFORMATION.response
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded SYSTEM_INFORMATION.response This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.	
	 */
	int (*system_information_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_response_t* resp);
	
	/*! A callback for the SYSTEM_INFORMATION.indication
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded SYSTEM_INFORMATION.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.	
	 */
	int (*system_information_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_indication_t* ind);
	
	/*! A callback for the NMM_STOP.response
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded NMM_STOP.response This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.		
	 */
	int (*nmm_stop_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nmm_stop_response_t* resp);
	

	/*! A callback for any vendor extension message received
	 *  \param config A pointer to the vnf configuration
	 *  \param p5_idx The p5 index used to indicate a particular pnf p5 connection
	 *  \param resp A data structure for the decoded vendor extention message 
	 *  \return not currently used.	
	 */
	int (*vendor_ext)(nfapi_vnf_config_t* config, int p5_idx, nfapi_p4_p5_message_header_t* msg);

	/*! A callback to allocate vendor extension messages
	 *  \param message_id The message is taken from the message header
	 *  \param msg_size The is the vendor extention message that has been allocated. 
	 *					The callee must set this value
	 *	\return A pointer to an allocated vendor extention message
	 */
	nfapi_p4_p5_message_header_t* (*allocate_p4_p5_vendor_ext)(uint16_t message_id, uint16_t* msg_size);
	
	/*! A callback to deallocate vendor extension messages
	 *  \param header A pointer to an allocated vendor extention message
	 */
	void (*deallocate_p4_p5_vendor_ext)(nfapi_p4_p5_message_header_t* header);




	
	
} nfapi_vnf_config_t;

/*! Creates and initialise the vnf config structure before use
 * \return A pointer to a vnf config structure
 */
nfapi_vnf_config_t* nfapi_vnf_config_create(void);

/*! Delete an vnf config
 */
void nfapi_vnf_config_destory(nfapi_vnf_config_t* config);


/*! Start the VNF library. 
 * \param config A pointer to a vnf config
 * \return 0 means success, -1 failure
 *
 * The config should be initailize with port the vnf should listen on and
 * the callback set to functions that will be called when a nFAPI message is 
 * recevied before calling nfapi_vnf_start.
 * 
 * This function will not return untill nfapi_vnf_stop is called
 */
int nfapi_nr_vnf_start(nfapi_vnf_config_t* config);

int nfapi_vnf_start(nfapi_vnf_config_t* config);

/*! Stop the VNF library. 
 * \param config A pointer to a vnf config
 * \return 0 means success, -1 failure
 * 
 * This function will cause the nfapi_vnf_start function to return
 */
int nfapi_vnf_stop(nfapi_vnf_config_t* config);

/*! Allocates a PHY ID for the PNF PHY instance managed by this VNF
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index return by the callbacks
 * \param phy_id A pointer to a phy_id that will be set by this function
 * \return  0 means success, -1 failure
 * 
 * Called before nfapi_vnf_config_req to allocate a vnf phy instance. This
 * function will return unqiue phy_id to be used for this identify the phy
 *
 */
int nfapi_vnf_allocate_phy(nfapi_vnf_config_t* config, int p5_idx, uint16_t* phy_id);

// P5 Request functions
//
/*! Send the PNF_PARAM.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a PNF_PARAM.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_pnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_param_request_t* req);
int nfapi_nr_vnf_pnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_param_request_t* req);

/*! Send the PNF_CONFIG.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a PNF_CONFIG.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_pnf_config_req(nfapi_vnf_config_t* config,int p5_idx, nfapi_pnf_config_request_t* req);
int nfapi_nr_vnf_pnf_config_req(nfapi_vnf_config_t* config,int p5_idx, nfapi_nr_pnf_config_request_t* req);

/*! Send the PNF_START.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a PNF_START.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_pnf_start_req(nfapi_vnf_config_t* config,int p5_idx, nfapi_pnf_start_request_t* req);
int nfapi_nr_vnf_pnf_start_req(nfapi_vnf_config_t* config,int p5_idx, nfapi_nr_pnf_start_request_t* req);

/*! Send the PNF_STOP.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a PNF_STOP.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_pnf_stop_req(nfapi_vnf_config_t* config,int p5_idx, nfapi_pnf_stop_request_t* req);

/*! Send the PARAM.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a PARAM.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_param_request_t* req);
int nfapi_nr_vnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_param_request_scf_t* req);

/*! Send the CONFIG.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a CONFIG.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_nr_vnf_config_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_config_request_scf_t* req);
int nfapi_vnf_config_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_config_request_t* req);

/*! Send the START.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a START.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_start_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_start_request_t* req);
int nfapi_nr_vnf_start_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_start_request_scf_t* req);

/*! Send the STOP.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a STOP.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_stop_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_stop_request_t* req);

/*! Send the MEASUREMENT.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a MEASUREMENT.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_measurement_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_measurement_request_t* req);

// P4 Request functions
/*! Send the RSSI.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a RSSI.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_rssi_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_rssi_request_t* req);

/*! Send the CELL_SEARCH.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a CELL_SEARCH.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_cell_search_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_cell_search_request_t* req);

/*! Send the BROADCAST_DETECT.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a BROADCAST_DETECT.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_broadcast_detect_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_broadcast_detect_request_t* req);

/*! Send the SYSTEM_INFORMATION_SCHEDULE.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a SYSTEM_INFORMATION_SCHEDULE.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_system_information_schedule_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_schedule_request_t* req);

/*! Send the SYSTEM_INFORMATION.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a SYSTEM_INFORMATION.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_system_information_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_request_t* req);

/*! Send the NMM_STOP.request
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param req A pointer to a NMM_STOP.request message structure
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_nmm_stop_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_nmm_stop_request_t* req);

/*! Send a vendor extension message
 * \param config A pointer to a vnf config
 * \param p5_idx The P5 index
 * \param msg A poiner to a vendor extention message
 * \return  0 means success, -1 failure
 */
int nfapi_vnf_vendor_extension(nfapi_vnf_config_t* config, int p5_idx, nfapi_p4_p5_message_header_t* msg);

//-----------------------------------------------------------------------------

/*! The nfapi VNF P7 connection information
 */
typedef struct nfapi_vnf_p7_config nfapi_vnf_p7_config_t;

/*! The nfapi VNF P7 configuration information
 */
typedef struct nfapi_vnf_p7_config
{
	/*! A user define callback to override the default memory allocation
	 * \param size Size of the memory block to allocate
	 * \return a pointer to a memory block
	 *
	 * If not set the vnf p7 library will use malloc
	 */
	void* (*malloc)(size_t size);
	
	/*! A user define callback to override the default memory deallocation
	 * \param ptr Pointer to a memory block to deallocate
	 *
	 * If not set the vnf p7 library will use free
	 */
	void (*free)(void*);

	/*! The port the vnf p7 will receive on */
	int port;

	/*! Flag to indicate of the pnf should use the P7 checksum */
	uint8_t checksum_enabled;

	/*! The maxium size of a P7 segement. If a message is large that this it
	 * will be segemented */
	uint16_t segment_size;
	uint16_t max_num_segments;

	/*! Configuration option for the p7 pack unpack functions*/
	nfapi_p7_codec_config_t codec_config;

	/* ! Call back to indicate the sync state with the PNF PHY
	 * \param config A pointer to the vnf p7 configuration
	 * \param sync Indicating if the pnf is in sync or not
	 * \return not currently used
	 *
	 * sync = 0  : in sync
	 * sync != 0 : out of sync
	 */
	int (*sync_indication)(struct nfapi_vnf_p7_config* config, uint8_t sync);

	/*! A callback for the subframe indication
	 * \param config A pointer to the vnf p7 configuration
	 * \param phy_id The ID for the PNF PHY instance
	 * \param sfn_sf The SFN SF number formated as per the FAPI specification
	 * \return not currently used
	 *
	 * This callback is an indication for the VNF to generate the downlink subframe messages
	 * for sfn/sf. This indicatoin is called every millisecond
	 *
	 * The VNF P7 Lib will adjust the subframe timing to 'catch-up' or 'slow-down' with the PNF PHY's
	 *
	 * \todo Need some way the tell the VNF how long it has
	 */
	
	int (*subframe_indication)(struct nfapi_vnf_p7_config* config, uint16_t phy_id, uint16_t sfn_sf);
	int (*slot_indication)(struct nfapi_vnf_p7_config* config, uint16_t phy_id, uint16_t sfn, uint16_t slot);

	/*! A callback for the HARQ.indication
     *  \param config A pointer to the vnf p7 configuration
	 *  \param ind A data structure for the decoded HARQ.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 * 
	 *  The ind may contain pointers to dyanmically allocated sub structures  
	 *  such as the pdu. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*harq_indication)(struct nfapi_vnf_p7_config* config, nfapi_harq_indication_t* ind);
	
	/*! A callback for the CRC.ind
     *  \param config A pointer to the vnf p7 configuration
	 *  \param ind A data structure for the decoded CRC.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 * 
	 *  The ind may contain pointers to dyanmically allocated sub structures  
	 *  such as the pdu. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*crc_indication)(struct nfapi_vnf_p7_config* config, nfapi_crc_indication_t* ind);
	
	/*! A callback for the RX_ULSCH.indication
     *  \param config A pointer to the vnf p7 configuration
	 *  \param ind A data structure for the decoded RX_ULSCH.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 * 
	 *  The ind may contain pointers to dyanmically allocated sub structures  
	 *  such as the pdu. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 *
	 *  Note that the rx_indication may hold one or many uplink pdus in the 
	 *  ind.rx_indication_body.rx_pdu_list
	 */	
	int (*rx_indication)(struct nfapi_vnf_p7_config* config, nfapi_rx_indication_t* ind);
	
	/*! A callback for the RACH.indication
     *  \param config A pointer to the vnf p7 configuration
	 *  \param ind A data structure for the decoded RACH.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 * 
	 *  The ind may contain pointers to dyanmically allocated sub structures  
	 *  such as the pdu. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*rach_indication)(struct nfapi_vnf_p7_config* config, nfapi_rach_indication_t* ind);
	
	/*! A callback for the SRS.indication
     *  \param config A pointer to the vnf p7 configuration
	 *  \param ind A data structure for the decoded SRS.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 * 
	 *  The ind may contain pointers to dyanmically allocated sub structures  
	 *  such as the pdu. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*srs_indication)(struct nfapi_vnf_p7_config* config, nfapi_srs_indication_t* ind);
	
	/*! A callback for the RX_SR.indication
     *  \param config A pointer to the vnf p7 configuration
	 *  \param ind A data structure for the decoded RX_SR.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 * 
	 *  The ind may contain pointers to dyanmically allocated sub structures  
	 *  such as the pdu. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*sr_indication)(struct nfapi_vnf_p7_config* config, nfapi_sr_indication_t* ind);
	
	/*! A callback for the RX_CQI.indication
     *  \param config A pointer to the vnf p7 configuration
	 *  \param ind A data structure for the decoded RX_CQI.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 * 
	 *  The ind may contain pointers to dyanmically allocated sub structures  
	 *  such as the pdu. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*cqi_indication)(struct nfapi_vnf_p7_config* config, nfapi_cqi_indication_t* ind);
	
	/*! A callback for the LBT_DL.indication
     *  \param config A pointer to the vnf p7 configuration
	 *  \param ind A data structure for the decoded LBT_DL.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 * 
	 *  The ind may contain pointers to dyanmically allocated sub structures  
	 *  such as the pdu. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*lbt_dl_indication)(struct nfapi_vnf_p7_config* config, nfapi_lbt_dl_indication_t* ind);
	
	/*! A callback for the NB_HARQ.indication
     *  \param config A pointer to the vnf p7 configuration
	 *  \param ind A data structure for the decoded LBT_DL.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 * 
	 *  The ind may contain pointers to dyanmically allocated sub structures  
	 *  such as the pdu. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*nb_harq_indication)(struct nfapi_vnf_p7_config* config, nfapi_nb_harq_indication_t* ind);	
	
	/*! A callback for the NRACH.indication
     *  \param config A pointer to the vnf p7 configuration
	 *  \param ind A data structure for the decoded LBT_DL.indication This will 
	 *              have been allocated on the stack. 
	 *  \return not currently used.
	 * 
	 *  The ind may contain pointers to dyanmically allocated sub structures  
	 *  such as the pdu. The dyanmically allocated structure will 
	 *  be deallocated on return. If the client wishes to 'keep' the structures 
	 *  then the substructure pointers should be set to 0 and then the client should
	 *  use the codec_config.deallocate function to release it at a future point
	 */	
	int (*nrach_indication)(struct nfapi_vnf_p7_config* config, nfapi_nrach_indication_t* ind);		

	//The NR indication functions below copy uplink information received at the VNF into the UL info struct
	int (*nr_slot_indication)(nfapi_nr_slot_indication_scf_t* ind);
	int (*nr_crc_indication)(nfapi_nr_crc_indication_t* ind);
	int (*nr_rx_data_indication)(nfapi_nr_rx_data_indication_t* ind);
	int (*nr_uci_indication)(nfapi_nr_uci_indication_t* ind);
	int (*nr_rach_indication)(nfapi_nr_rach_indication_t* ind);
	int (*nr_srs_indication)(nfapi_nr_srs_indication_t* ind);

	/*! A callback for any vendor extension messages
     *  \param config A pointer to the vnf p7 configuration
	 *  \param msg A data structure for the decoded vendor extention message allocated
	 *			   using the allocate_p7_vendor_ext callback
	 *  \return not currently used.
	 */	
	int (*vendor_ext)(struct nfapi_vnf_p7_config* config, nfapi_p7_message_header_t* msg);

	/*! Optional userdata that will be passed back in the callbacks*/
	void* user_data;
	
	/*! A callback to allocate a memory for a vendor extension message
	 *  \param message_id The message is taken from the p7 message header
	 *  \param msg_size The is the vendor extention message that has been allocated. 
	 *					The callee must set this value
	 *	\return A pointer to an allocated vendor extention message
	 */
	nfapi_p7_message_header_t* (*allocate_p7_vendor_ext)(uint16_t message_id, uint16_t* msg_size);
	
	/*! A callback to deallocate a vendor extension message
	 *  \param header A pointer to an allocated vendor extention message
	 */
	void (*deallocate_p7_vendor_ext)(nfapi_p7_message_header_t* header);


} nfapi_vnf_p7_config_t;

/*! Creates and initializes the nfapi_vnf_p7_config structure before use
 *  \return A pointer to an allocated vnf p7 configuration
 */
nfapi_vnf_p7_config_t* nfapi_vnf_p7_config_create(void);

/*! Cleanup and delete nfapi_vnf_p7_config structure
 *  \param config A pointer to an vnf p7 configuration structure
 *
 *  The pointer to the config will not long be valid after this call
 */
 
void nfapi_vnf_p7_config_destory(nfapi_vnf_p7_config_t* config);

/*! Start the VNF P7 library.
 *  \param config A pointer to an vnf p7 configuration structure
 *	\return A status value. 0 equal success, -1 indicates failure
 *
 * This function is blocking and will not return until the nfapi_vnf_p7_stop
 * function is called. 
 */
 
int nfapi_vnf_p7_start(nfapi_vnf_p7_config_t* config);
int nfapi_nr_vnf_p7_start(nfapi_vnf_p7_config_t* config);


/*! Stop the VNF P7 library. 
 *  \param config A pointer to an vnf p7 configuration structure
 *	\return A status value. 0 equal success, -1 indicates failure
 * 
 * This function will cause the nfapi_vnf_p7_start function to return
 */
int nfapi_vnf_p7_stop(nfapi_vnf_p7_config_t* config);

/*! Release a P7 message back to the vnf_p7 library. This should be used if the
 *  callback return 0 in the case where MAC wants to keep the message for
 *  futher processing.
 *  This function will release any pdu's is they are non-null. If the uplink
 *  PDU need to be kept then they pdu pointer should be set to 0 in the message
 *  and then the nfapi_vnf_p7_release_pdu message can be used to release the
 *  pdu later.
 */
int nfapi_vnf_p7_release_msg(nfapi_vnf_p7_config_t* config, nfapi_p7_message_header_t*);

/*! Release a P7 pdu's back to the vnf_p7 library.
 */
int nfapi_vnf_p7_release_pdu(nfapi_vnf_p7_config_t* config, void*);

/*! Add a vnf p7 instance to the vnf p7 module
 *  \param config A pointer to the vnf p7 configuration
 *  \param pnf_p7_addr The udp address the pnf p7 entity has chosen 
 *  \param pnf_p7_port The udp port the pnf p7 entity has chosen
 *  \param phy_id The unique phy id for the pnf p7 entity
 *  \return A status value. 0 equal success, -1 indicates failure
 *
 * This function should be used to each pnf p7 entity that is to be added to this
 * vnf p7 entity. Once added the vnf p7 entity will start establish sync with the
 * pnf p7 entity and that has been sucessfull will generate subframe indications for it
 */
int nfapi_vnf_p7_add_pnf(nfapi_vnf_p7_config_t* config, const char* pnf_p7_addr, int pnf_p7_port, int phy_id);

/*! Delete a vnf p7 instance to the vnf p7 module
 *  \param config A pointer to the vnf p7 configuration
 *  \param phy_id The unique phy id for the pnf p7 entity
 *  \return A status value. 0 equal success, -1 indicates failure
 *
 * This function should be used to remove a pnf p7 entity from the vnf p7 entity
 */
int nfapi_vnf_p7_del_pnf(nfapi_vnf_p7_config_t* config, int phy_id);

/*! Send the DL_CONFIG.request
 *  \param config A pointer to the vnf p7 configuration
 *  \param req A data structure for the decoded DL_CONFIG.request.
 *	\return A status value. 0 equal success, -1 indicates failure
 * 
 *  The caller is responsiable for memory management of any pointers set in the req, which
 *  may be released after this function call has returned or at a later pointer
 */
int nfapi_vnf_p7_dl_config_req(nfapi_vnf_p7_config_t* config, nfapi_dl_config_request_t* req);
int nfapi_vnf_p7_nr_dl_config_req(nfapi_vnf_p7_config_t* config, nfapi_nr_dl_tti_request_t* req);

/*! Send the UL_CONFIG.request
 *  \param config A pointer to the vnf p7 configuration
 *  \param req A data structure for the decoded UL_CONFIG.request.
 *	\return A status value. 0 equal success, -1 indicates failure
 * 
 *  The caller is responsiable for memory management of any pointers set in the req, which
 *  may be released after this function call has returned or at a later pointer
 */
int nfapi_vnf_p7_ul_config_req(nfapi_vnf_p7_config_t* config, nfapi_ul_config_request_t* req);
int nfapi_vnf_p7_ul_tti_req(nfapi_vnf_p7_config_t* config, nfapi_nr_ul_tti_request_t* req);
/*! Send the HI_DCI0.request
 *  \param config A pointer to the vnf p7 configuration
 *  \param req A data structure for the decoded HI_DCI0.request.
 *	\return A status value. 0 equal success, -1 indicates failure
 * 
 *  The caller is responsiable for memory management of any pointers set in the req, which
 *  may be released after this function call has returned or at a later pointer
 */
int nfapi_vnf_p7_hi_dci0_req(nfapi_vnf_p7_config_t* config, nfapi_hi_dci0_request_t* req);
int nfapi_vnf_p7_ul_dci_req(nfapi_vnf_p7_config_t* config, nfapi_nr_ul_dci_request_t* req);
/*! Send the TX.req
 *  \param config A pointer to the vnf p7 configuration
 *  \param req A data structure for the decoded HI_DCI0.request.
 *	\return A status value. 0 equal success, -1 indicates failure
 * 
 *  The caller is responsiable for memory management of any pointers set in the req, which
 *  may be released after this function call has returned or at a later pointer
 */
int nfapi_vnf_p7_tx_req(nfapi_vnf_p7_config_t* config, nfapi_tx_request_t* req);
int nfapi_vnf_p7_tx_data_req(nfapi_vnf_p7_config_t* config, nfapi_nr_tx_data_request_t* req);
/*! Send the LBT_DL_CONFIG.requst
 *  \param config A pointer to the vnf p7 configuration
 *  \param req A data structure for the decoded LBT_DL_CONFIG.request.
 *	\return A status value. 0 equal success, -1 indicates failure
 * 
 *  The caller is responsiable for memory management of any pointers set in the req, which
 *  may be released after this function call has returned or at a later pointer
 */
int nfapi_vnf_p7_lbt_dl_config_req(nfapi_vnf_p7_config_t* config, nfapi_lbt_dl_config_request_t* req);

/*! Send a vendor extension message
 *  \param config A pointer to the vnf p7 configuration
 *  \param msg A data structure for the decoded vendor extention message
 *	\return A status value. 0 equal success, -1 indicates failure
 * 
 *  The caller is responsiable for memory management of any pointers set in the req, which
 *  may be released after this function call has returned or at a later pointer
 */
int nfapi_vnf_p7_vendor_extension(nfapi_vnf_p7_config_t* config, nfapi_p7_message_header_t* msg);

/*! Send the RELEASE_RNTI.request
 *  \param config A pointer to the vnf p7 configuration
 *  \param req A data structure for the decoded RELEASE_RNTI.request.
 *  \return A status value. 0 equal success, -1 indicates failure
 *
 *  The caller is responsiable for memory management of any pointers set in the req, which
 *  may be released after this function call has returned or at a later pointer
 */
int nfapi_vnf_p7_ue_release_req(nfapi_vnf_p7_config_t* config, nfapi_ue_release_request_t* req);
#if defined(__cplusplus)
}
#endif

#endif // _NFAPI_PNF_INTERFACE_H_
