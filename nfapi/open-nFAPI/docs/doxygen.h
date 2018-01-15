/*! \mainpage Open Network Function Application Platform Interface (Open-nFAPI)
 *
 * \section intro_sec Introduction
 *
 * Open-nFAPI is implementation of the Small Cell Forum's network femto API or nFAPI for short. 
 * nFAPI defines a network protocol that is used to connect a Physical Network Function (PNF) 
 * running LTE Layer 1 to a Virtual Network Function (VNF) running LTE Layer 2 and above. The specification
 * can be found at http://scf.io/documents/082.
 *
 * The aim of Open-nFAPI is to provide an open interface between LTE Layer 1 and Layer 2 to allow for
 * interoperability between the PNF and VNF & also to facilitate the sharing of PNF's between
 * different VNF's
 *
 * Open-nFAPI implements the P4, P5 and P7 interfaces as defined by the nFAPI specification. 
 * - The P5 interface allows the VNF to query and configure the 'resources' of the PNF; slicing it into
 * 1 or more phy instances.  
 * - The P7 interface is used to send the subframe information between the PNF and VNF for 1 or more phy instances
 * - The P4 interface allows the VNF to request the PNF phy instances to perform measurements of the surrounding network
 *
 * The remaining interfaces are currently outside of the scope of this project.
 *
 * The best place to start is by reviewing the detailed nFAPI call flow which can be \ref nfapi_call_flow "here"
 * 
 * \section contrib Contibuting
 *
 * The Small Cell Forum cordially requests that any derivative work that looks to extend 
 * the nFAPI libraries use the specified vendor extension techniques, so ensuring 
 *the widest interoperability of the baseline nFAPI specification in those derivative works.
 * 
 * \section install_sec Installation
 *
 * \subsection step1 Step 1: Opening the box
 *
 * etc...
 *
 * \section dir_struct Directory Structure
 * \code
 
 *     nfapi
 *     |- common                    Common functions for the nfapi project
 *     |  |- src
 *     |  |- public-inc
 *     |- nfapi                     The NFAPI lib
 *     |  |- inc
 *     |  |- src
 *     |  |- public-inc             Public interface for the nfapi library
 *     |  |- tests                  Unit test for the nfapi lib
 *     |- pnf                       The PNF lib
 *     |  |- inc
 *     |  |- src
 *     |  |- public-inc             Public interface for the pnf library
 *     |  |- tests                  Unit test for the pnf lib
 *     |- vnf                       The VNF lib
 *     |  |- inc
 *     |  |- src
 *     |  |- public-inc             Public interface for the vnf library
 *     |  |- tests                  Unit test for the vnf lib
 *     |- sim_common                Common functions for the nfapi simulators
 *     |  |- inc
 *     |  |- src
 *     |- pnf_sim                   The PNF simulator
 *     |  |- inc
 *     |  |- src
 *     |- vnf_sim                   The VNF simulator
 *     |  |- inc
 *     |  |- src
 *     |- integration_tests         Integration tests that run both VNF & PNF simulators
 *     |  |- inc
 *     |  |- src
 *     |- docs                      Documentation
 *     |- xml                       Xml configuration for the simulator
 
 * \endcode
 * \section building Building
 * To build the nfapi project and run the unit test you will need to 
 * \code
 *     autoreconf -i
 *     ./configure
 *     make
 *     make check
 * \endcode
 * The following dependencies will be required
 * - Boost. Need to build the simulators
 * - STCP. Need to run the simulators
 * - 
 * \section simulators  Running the simulators
 * Once you have build the nfapi project you can run the PNF/VNF simulators on either the same
 * of seperate linux machines. 
 * 
 * To run the VNF from the vnf_sim directory.
 * \code
 *     ./vnfsim 4242 ../xml/vnf_A.xml
 * [MAC] Rx Data from 8891
 * [MAC] Tx Data to 10.231.16.80.8892
 * Calling nfapi_vnf_start
 * 2035.854438: 0x04:  773068608: nfapi_vnf_start()
 * 2035.854450: 0x04:  773068608: Starting P5 VNF connection on port 4242
 * 2035.854472: 0x04:  773068608: P5 socket created... 3
 * 2035.854478: 0x03:  773068608: VNF Setting the SCTP_INITMSG
 * 2035.854481: 0x04:  773068608: IPV4 binding to port 4242
 * 2035.854485: 0x04:  773068608: bind succeeded..3.
 * 2035.854497: 0x04:  773068608: listen succeeded...
 * \endcode
 *
 * 
 * To run the PNF from the pnf_sim directory
 * \code
 *     ./pnfsim 127.0.01 4242 ../xml/pnf_phy_1_A.xml
 * nfapi_pnf_start
 * Starting P5 PNF connection to VNF at 127.0.0.1:4242
 * Host address info  0 Family:IPV4 Address:127.0.0.1
 * PNF Setting the SCTP_INITMSG
 * P5 socket created...
 * Socket CONNECTED
 * PNF_PARAM.request received
 * [PNF_SIM] pnf param request
 * PNF_CONFIG.request received
 * [PNF_SIM] pnf config request
 * .... and so on
 * \endcode
 * 
 * You can Ctrl-C to exit the simulators. 
 * 

 */

/*! \page nfapi_call_flow NFAPI Call Flow
 *
 * \section seq_diag Sequence diagram
 *
 * The follow sequence digram show how the nFAPI api will be used to bring up 
 * the PNF and then the PNF PHY and initiate subframe message exchange
 * The names of the CLIENT callbacks are placeholders for the functions that 
 * the CLIENT will provide.
 *
 * \msc
 *
 *   width="1750";
 *   FAPI, PNF_P7_CLIENT, PNF_P7_LIB, PNF_P5_CLIENT, PNF_P5_LIB, VNF_P5_LIB, VNF_P5_CLIENT, VNF_P7_LIB, VNF_P7_CLIENT, MAC;
 *   VNF_P5_CLIENT=>VNF_P5_LIB		[label="nfapi_vnf_config_create", URL="\ref nfapi_vnf_config_create"];
 *   VNF_P5_CLIENT=>VNF_P5_LIB		[label="nfapi_vnf_start", URL="\ref nfapi_vnf_start"];
 *   VNF_P5_LIB note VNF_P5_LIB		[label="Listening for PNF connection"];
 *   PNF_P5_CLIENT note PNF_P5_CLIENT		[label="P9 has provided the address of the VNF"];
 *	 PNF_P5_CLIENT=>PNF_P5_LIB		[label="nfapi_pnf_config_create", URL="\ref nfapi_pnf_config_create"];
 *	 PNF_P5_CLIENT=>PNF_P5_LIB		[label="nfapi_pnf_start", URL="\ref nfapi_pnf_start"];
 *   PNF_P5_LIB note PNF_P5_LIB		[label="PNF STATE : PNF IDLE"];
 *   PNF_P5_LIB->VNF_P5_LIB			[label="<connect>"];
 *   VNF_P5_LIB=>VNF_P5_CLIENT		[label="pnf_connection_indication", URL="\ref nfapi_vnf_config::pnf_connection_indication"];
 *
 *   VNF_P5_LIB<=VNF_P5_CLIENT		[label="nfapi_vnf_pnf_param_req", URL="\ref nfapi_vnf_pnf_param_req"];
 *   VNF_P5_LIB->PNF_P5_LIB			[label="PNF_PARAM.request"];
 *   PNF_P5_LIB=>PNF_P5_CLIENT		[label="pnf_param_request", URL="\ref nfapi_pnf_config::pnf_param_req"];
 *   PNF_P5_LIB<=PNF_P5_CLIENT		[label="nfapi_pnf_pnf_param_response", URL="\ref nfapi_pnf_pnf_param_resp"];
 *   VNF_P5_LIB<-PNF_P5_LIB			[label="PNF_PARAM.response"];
 *   VNF_P5_LIB=>VNF_P5_CLIENT		[label="pnf_param_response", URL="\ref nfapi_vnf_config::pnf_param_resp"];
 *
 *   VNF_P5_LIB<=VNF_P5_CLIENT		[label="nfapi_vnf_pnf_config_req", URL="\ref nfapi_vnf_pnf_config_req"];
 *   VNF_P5_LIB->PNF_P5_LIB			[label="PNF_CONFIG.request"];
 *   PNF_P5_LIB=>PNF_P5_CLIENT		[label="pnf_config_request", URL="\ref nfapi_pnf_config::pnf_config_req"];
 *   PNF_P5_LIB<=PNF_P5_CLIENT		[label="nfapi_pnf_pnf_config_response", URL="\ref nfapi_pnf_pnf_config_resp"];
 *   PNF_P5_LIB note PNF_P5_LIB		[label="PNF STATE : PNF CONFIGURED"];
 *   VNF_P5_LIB<-PNF_P5_LIB			[label="PNF_CONFIG.response"];
 *   VNF_P5_LIB=>VNF_P5_CLIENT		[label="pnf_config_response", URL="\ref nfapi_vnf_config::pnf_config_resp"];
 *
 *   VNF_P5_LIB<=VNF_P5_CLIENT		[label="nfapi_vnf_pnf_start_req", URL="\ref nfapi_vnf_pnf_start_req"];
 *   VNF_P5_LIB->PNF_P5_LIB			[label="PNF_START.request"];
 *   PNF_P5_LIB=>PNF_P5_CLIENT		[label="pnf_start_request", URL="\ref nfapi_pnf_config::pnf_start_req"];
 *   PNF_P5_CLIENT=>FAPI			[label="<create>"];
 *   FAPI note FAPI					[label="FAPI STATE : IDLE"];
 *   PNF_P5_LIB<=PNF_P5_CLIENT		[label="nfapi_pnf_pnf_start_response", URL="\ref nfapi_pnf_pnf_start_resp"];
 *   PNF_P5_LIB note PNF_P5_LIB		[label="PNF STATE : PNF RUNNING"];
 *   VNF_P5_LIB<-PNF_P5_LIB			[label="PNF_START.response"];
 *   VNF_P5_LIB=>VNF_P5_CLIENT		[label="pnf_start_response", URL="\ref nfapi_vnf_config::pnf_param_resp"];
 *
 *   --- [ label="If vnf p7 instance not already running"];
 *   VNF_P5_CLIENT=>VNF_P5_LIB	    [label="nfapi_vnf_allocate_phy", URL="\ref nfapi_vnf_allocate_phy"];
 *   VNF_P5_CLIENT=>VNF_P7_CLIENT	[label="<create>"];
 *   VNF_P7_CLIENT=>VNF_P7_LIB		[label="nfapi_vnf_p7_config_create", URL="\ref nfapi_vnf_p7_config_create"];
 *   VNF_P7_CLIENT=>VNF_P7_LIB		[label="nfapi_vnf_p7_start", URL="\ref nfapi_vnf_p7_start"];
 *   ---;
 *
 *   VNF_P5_LIB<=VNF_P5_CLIENT		[label="nfapi_vnf_param_request", URL="\ref nfapi_vnf_param_req"];
 *   VNF_P5_LIB->PNF_P5_LIB			[label="PARAM.request"];
 *   PNF_P5_LIB=>PNF_P5_CLIENT		[label="param_request", URL="\ref nfapi_pnf_config::param_req"];
 *   PNF_P5_CLIENT=>FAPI			[label="fapi_param_request"];
 *   PNF_P5_CLIENT<=FAPI			[label="fapi_param_response"];
 *   PNF_P5_CLIENT=>PNF_P7_CLIENT	[label="<create>"];
 *   PNF_P7_CLIENT=>PNF_P7_LIB		[label="nfapi_pnf_p7_create", URL="\ref nfapi_pnf_p7_config_create"];
 *   PNF_P7_CLIENT=>PNF_P7_LIB		[label="nfapi_pnf_p7_start", URL="\ref nfapi_pnf_p7_start"];
 *   PNF_P5_LIB<=PNF_P5_CLIENT		[label="nfapi_pnf_param_response", URL="\ref nfapi_pnf_param_resp"];
 *   VNF_P5_LIB<-PNF_P5_LIB			[label="PARAM.response"];
 *   VNF_P5_LIB=>VNF_P5_CLIENT		[label="param_response", URL="\ref nfapi_vnf_config::param_resp"];
 *
 *   VNF_P5_LIB<=VNF_P5_CLIENT		[label="nfapi_vnf_config_request", URL="\ref nfapi_vnf_config_req"];
 *   VNF_P5_LIB->PNF_P5_LIB			[label="CONFIG.request"];
 *   PNF_P5_LIB=>PNF_P5_CLIENT		[label="config_request", URL="\ref nfapi_pnf_config::config_req"];
 *   PNF_P5_CLIENT=>FAPI			[label="fapi_config_request"];
 *   FAPI note FAPI					[label="FAPI STATE : CONFIGURED"];
 *   PNF_P5_CLIENT<=FAPI			[label="fapi_config_response"];
 *   PNF_P5_LIB<=PNF_P5_CLIENT		[label="nfapi_pnf_config_response", URL="\ref nfapi_pnf_param_resp"];
 *   VNF_P5_LIB<-PNF_P5_LIB			[label="CONFIG.response"];
 *   VNF_P5_LIB=>VNF_P5_CLIENT		[label="config_response", URL="\ref nfapi_vnf_config::config_resp"];
 *
 *   VNF_P5_LIB<=VNF_P5_CLIENT		[label="nfapi_vnf_start_request", URL="\ref nfapi_vnf_start_req"];
 *   VNF_P5_LIB->PNF_P5_LIB			[label="START.request"];
 *   PNF_P5_LIB=>PNF_P5_CLIENT		[label="start_request", URL="\ref nfapi_pnf_config::start_req"];
 *   PNF_P5_CLIENT=>FAPI			[label="fapi_start_request"];
 *   FAPI note FAPI					[label="FAPI STATE : RUNNING"];
 *   FAPI note FAPI					[label="FAPI will start sending subframe indications"];
 *
 *   --- [ label="For each 'phy' subframe"];
 *   FAPI=>PNF_P7_CLIENT			[label="fapi_subframe_indication"];
 *   PNF_P7_CLIENT=>PNF_P7_LIB		[label="nfapi_pnf_p7_subframe_ind", URL="\ref nfapi_pnf_p7_subframe_ind"];
 *   ---;
 *   PNF_P7_CLIENT=>PNF_P5_CLIENT	[label="<start_response>"];
 *
 *   PNF_P5_LIB<=PNF_P5_CLIENT		[label="nfapi_pnf_start_response", URL="\ref nfapi_pnf_start_resp"];
 *   VNF_P5_LIB<-PNF_P5_LIB			[label="START.response"];
 *   VNF_P5_LIB=>VNF_P5_CLIENT		[label="start_response", URL="\ref nfapi_vnf_config::start_resp"];
 *   VNF_P5_CLIENT=>VNF_P7_CLIENT	[label="<p7_add_pnf>"];
 *   VNF_P7_CLIENT=>VNF_P7_LIB		[label="nfapi_vnf_p7_add_pnf", URL="\ref nfapi_vnf_p7_add_pnf"];
 *
 *   VNF_P7_LIB->PNF_P7_LIB			[label="DL_NODE_SYNC"];
 *   VNF_P7_LIB<-PNF_P7_LIB			[label="UL_NODE_SYNC"];
 *   VNF_P7_LIB<-PNF_P7_LIB			[label="TIMING_INFO"];
 *   VNF_P7_LIB note VNF_P7_LIB		[label="When sync is achieved"];
 *
 *   VNF_P7_LIB=>VNF_P7_CLIENT		[label="sync_indication", URL="\ref nfapi_vnf_p7_config::sync_indication"];
 *
 *   --- [ label="For each 'vnf phy' subframe"];
 *   VNF_P7_LIB=>VNF_P7_CLIENT		[label="subframe_indication(tti=x)", URL="\ref nfapi_vnf_p7_config::subframe_indication"];
 *   VNF_P7_CLIENT=>MAC				[label="subframe_indication"];
 *   VNF_P7_CLIENT<=MAC				[label="dl_config_req"];
 *   VNF_P7_CLIENT=>VNF_P7_LIB		[label="nfapi_vnf_p7_dl_config_req", URL="\ref nfapi_vnf_p7_dl_config_req"];
 *   VNF_P7_LIB->PNF_P7_LIB			[label="DL_CONFIG.request"];
 *   PNF_P7_LIB note PNF_P7_LIB		[label="Store in subframe buffer for tti x"];
 *   VNF_P7_CLIENT<=MAC				[label="ul_config_req"];
 *   VNF_P7_CLIENT=>VNF_P7_LIB		[label="nfapi_vnf_p7_ul_config_req", URL="\ref nfapi_vnf_p7_ul_config_req"];
 *   VNF_P7_LIB->PNF_P7_LIB			[label="UL_CONFIG.request"];
 *   PNF_P7_LIB note PNF_P7_LIB		[label="Store in subframe buffer for tti x"];
 *   VNF_P7_CLIENT<=MAC				[label="hi_dci0_req"];
 *   VNF_P7_CLIENT=>VNF_P7_LIB		[label="nfapi_vnf_p7_hi_dci0_req", URL="\ref nfapi_vnf_p7_hi_dci0_req"];
 *   VNF_P7_LIB->PNF_P7_LIB			[label="HI_DCI0.request"];
 *   PNF_P7_LIB note PNF_P7_LIB		[label="Store in subframe buffer for tti x"];
 *   VNF_P7_CLIENT<=MAC				[label="tx_req"];
 *   VNF_P7_CLIENT=>VNF_P7_LIB		[label="nfapi_vnf_p7_tx_req", URL="\ref nfapi_vnf_p7_tx_req"];
 *   VNF_P7_LIB->PNF_P7_LIB			[label="TX.request"];
 *   PNF_P7_LIB note PNF_P7_LIB		[label="Store in subframe buffer for tti x"];
 *   ---;
 *
 *
 *   --- [ label="For each 'phy' subframe"];
 *
 *   FAPI=>PNF_P7_CLIENT			[label="harq_indication"];
 *   PNF_P7_CLIENT=>PNF_P7_LIB		[label="nfapi_pnf_p7_harq_ind", URL="\ref nfapi_pnf_p7_harq_ind"];
 *   PNF_P7_LIB->VNF_P7_LIB			[label="HARQ.ind"];
 *   VNF_P7_LIB=>VNF_P7_CLIENT		[label="harq_ind", URL="\ref nfapi_vnf_p7_config::harq_indication"];
 *   VNF_P7_CLIENT=>MAC				[label="harq_ind"];
 *
 *   FAPI=>PNF_P7_CLIENT			[label="subframe_indication(tti=x)"];
 *   PNF_P7_CLIENT=>PNF_P7_LIB		[label="nfapi_pnf_p7_subframe_ind", URL="\ref nfapi_pnf_p7_subframe_ind"];
 *   PNF_P7_LIB note PNF_P7_LIB		[label="Send data from subframe buffer for tti x"];
 *   PNF_P7_LIB=>PNF_P7_CLIENT		[label="dl_config_req", URL="\ref nfapi_pnf_p7_config::dl_config_req"];
 *   PNF_P7_CLIENT=>FAPI			[label="dl_config_req"];
 *   PNF_P7_LIB=>PNF_P7_CLIENT		[label="ul_config_req", URL="\ref nfapi_pnf_p7_config::ul_config_req"];
 *   PNF_P7_CLIENT=>FAPI			[label="ul_config_req"];
 *   PNF_P7_LIB=>PNF_P7_CLIENT		[label="hi_dci0_req", URL="\ref nfapi_pnf_p7_config::hi_dci0_req"];
 *   PNF_P7_CLIENT=>FAPI			[label="hi_dci0_req"];
 *   PNF_P7_LIB=>PNF_P7_CLIENT		[label="tx_req", URL="\ref nfapi_pnf_p7_config::tx_req"];
 *   PNF_P7_CLIENT=>FAPI			[label="tx_req"];
 *   ---;
 *
 * \endmsc
 *
 * \section seq_diag_bca BCA
 *
 * -#	The VNF_P5_CLIENT is created. If it left to the VRAN partner to define how that is done. The client is responsiable to creation of the thread with the correct priorities within the wider system.
 * -#	The VNF_P5_CLIENT creates and initializes the VNF_P5_LIB (nfapi_vnf_config_create & nfapi_vnf_start) providing the address that it should listen on for incoming SCTP connections. It is expected that this would be provided by some VNF management system. The VNF_P5_CLIENT also provides information on the callbacks that the VNF_P5_LIB will use to inform the VNF_P5_CLIENT of the received messages
 * \code
 * nfapi_vnf_config_t* vnf_config = nfapi_vnf_config_create();
 * vnf_config->pnf_connection_indication = &pnf_connection_indication;
 * vnf_config->pnf_param_resp = &pnf_param_resp;
 * nfapi_vnf_start(vnf_config);
 * \endcode
 * -#	It is FFS how the address information of the VNF P5 is passed to the PNF over P9, but assuming that has been done
 * -#	The PNF_P5_CLIENT is created and then creates the PNF_P5_LIB (nfapi_pnf_init & nfapi_pnf_start) passing the address of the VNF to connect to. The PNF_P5_CLIENT also provides information on the callbacks that the PNF_P5_LIB will use to inform the PNF_P5_CLIENT of the received messages
 * \code
 * nfapi_pnf_config_t* pnf_config = nfapi_pnf_config_create();
 * pnf_config->pnf_param_req = &pnf_param_req;
 * nfapi_pnf_start(pnf_config);
 * \endcode
 * -#	The PNF_P5_LIB attempts to establish connection to the VNF_P5 endpoint
 * -#	The VNF_P5_LIB receives the STCP connection request and indicates this to the VNF_P5_CLIENT (pnf_connection_indication). The VNF_P5_CLIENT can decide to accept or reject the connection.
  * \code
 * int pnf_connection_indication(nfapi_vnf_config_t* config, int p5_idx) {
 *   // send the PNF_PARAM.request
 *   nfapi_pnf_param_request_t req;
 *   memset(&req, 0, sizeof(req));
 *   req.header.message_id = NFAPI_PNF_PARAM_REQUEST;
 *   nfapi_vnf_pnf_param_req(config, p5_idx, req);
 *   return 0;
 * }
 * \endcode

 * -#	The VNF and PNF then proceed to exchange the NFAPI PNF messages (PNF_PARAM, PNF_CONFIG and PNF_START). At each stage the LIB’s invoke callbacks on the CLIENT’s for them to handle message the return the appropriate response. 
 * -#	Finally the VNF will send the PNF_START.req which will invoke the PNF_P5_CLIENT nfapi_pnf_start_request callback. This is the point at which it is expect that the PNF_CLIENT will create the FAPI module. The PNF_P5_CLIENT will need to perform the translation between NFAPI structures and vendor specific FAPI structures for messages sent between the PNF_P5_LIB and the FAPI interface.
 * -#	The PNF_START.response will sent back to the VNF and at this point the VNF_P5_CLIENT will need to decide which VNF_P7 instance will handle the P7 connection. i.e. it needs to determine the IP address and port number of the P7 VNF instance. 
 * -#	A new VNF_P7_CLIENT & VNF_P7_LIB should be configured and started if necessary.
 * -#	The VNF_P5 will then send the PARAM.request to the PNF_P5 which will forward it onto the FAPI interface. This PARAM.request includes the VNF_P7 address. The PNF_P7_CLIENT must decide on the PNF P7 address. This information is used to create and initialize the PNF_P7_LIB. The PNF_P7 address is then returned by the PNF_P5 to the VNF_P5 in the PARAM.response.
 * -#	The VNF_P5 will then exchange with the PNF_P5 the CONFIG.request/response which will be used to configure the PNF FAPI instance.
 * -#	The VNF_P5 will then decide to start the PNF_P7 by sending the START.request.
 * -#	The PNF_P5_CLIENT need to send the start request to the FAPI instance. The response which is the start of subframe indications from the FAPI. In receipt of the first subframe indication the PNF_P5_CLIENT will send the START.response to the VNF_P5 via the PNF_P5_LIB
 * -#	The FAPI subframe indications should be forwarded to the PNF_P7_LIB by the PNF_P7_CLIENT. Until the VNF_P7 instance has sent the subframe configuration messages (dl_config, ul_config, etc) the PNF_P7_LIB will send ‘dummy’ subframe configuration messages. The contents of which are configurable by the PNF_P7_CLIENT.
 * -#	When the VNF_P5_CLIENT receives the START.response it will need to ‘communicate’ with the VNF_P7_CLIENT to inform it that the PNF_P7 instance has started. The VNF_P7_CLIENT will call the nfapi_vnf_P7_add_pnf function passing the address details of the PNF_P7 instance.
 * -#	The VNF_P7_LIB will then start the sync procedure to establish sub frame synchronization between the PNF_P7 and VNF_P7 instances. This involves sending the DL_NODE_SYNC and UL_NODE_SYNC to determine network latency and PNF processing latency to be able to request MAC generate sub frames in advance on when they are required by the FAPI interface.
 * -#	When sync is achieved the VNF_P7_LIB will send the nfapi_sync_indication to the VNF_P7_CLIENT. 
 * -#	The VNF_P7_LIB will then start issuing sub frame indications to the VNF_P7_CLIENT. The logical intent is that they indications are sent every millisecond. However due to the scheduling jitter that may be seen by the VNF these subframe indications may be less than or more than 1ms apart. How this is handled is one of the critical functions of the VNF_P7_LIB and may require specialization or requirements on the VNF environment i.e. CPU pinning.
 * -#	The VNF_P7_LIB will send the subframe_indication to the VNF_P7_CLIENT for ‘x’ subframe’s inadvance of the current TTI at the PNF. The delta ‘x’ will be determined by the sync procedure and how far in advance the FAPI needs to receive messages before RF transmission. 
 * -#	The VNF_P7_CLIENT is then responsible for communicating to the MAC layer to prepare the dl_config_request, ul_config_request, hi_dci0_request, tx_req in a timely manner and sending them to the VNF_P7_LIB
 * -#	The VNF_P7_LIB will send them to the VNF_P7_LIB. 
 * -#	The PNF_P7_LIB will store these messages in a subframe buffer or playout buffer in advance of when they are required by FAPI.
 *  -#	The PNF_P7_LIB will monitor to see if these messages arrive too late and if they do trigger a TIMING_INFO response to the VNF_P7_LIB to reassess if the sync is still valid.
 * -#	As some time in the future the FAPI will send a subframe indication for the TTI that the VNF_P7_LIB had previously requested.
 * -#	The PNF_P7_LIB will then use the messages in the subframe buffer and send them too the FAPI interface for transmission. The PNF_P7_CLIENT will need to perform the translation between NFAPI structure and vendor specific FAPI structures
 * -#	This subframe exchange will continue and allow high layer MAC and RRC function to bringup the cell and connect UE’s

 */
