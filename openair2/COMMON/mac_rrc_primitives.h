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

#ifndef __MAC_RRC_PRIMITIVES_H__
#    define __MAC_RRC_PRIMITIVES_H__



#ifndef OPENAIR2_IN
#include "LAYER2/RLC/rlc.h"
#endif
#include "COMMON/platform_types.h"
#include "COMMON/platform_constants.h"

#include "LTE_RadioResourceConfigCommonSIB.h"
#include "LTE_RadioResourceConfigDedicated.h"
#include "LTE_MeasGapConfig.h"
#include "LTE_TDD-Config.h"
#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
#include "LTE_MBSFN-AreaInfoList-r9.h"
#include "LTE_MBSFN-SubframeConfigList.h"
#endif
//#include "rrm_config_structs.h"
//#include "platform_types.h"
/** @defgroup _mac_rrc_primitives_ MAC Layer Primitives for Communications with RRC
 * @ingroup _openair_mac_layer_specs_
 * @{
This subclause describes the primitives for communications between the RRC and MAC sub-layers.


This subclause describes the primitives for communications between the RRC and MAC sub-layers.
The RRC sequence diagrams are shown below.

The primitives responsible for relaying of radio and traffic measurements to the radio resource manager via
RRC signaling are :
-# MAC_MEAS_REQ: enables RRC to request a measurement process in the MAC (one-shot, periodic or event-driven). This configures
a measurement process in the MAC.
-# MAC_MEAS_IND: indicates to RRC that a (periodic) measurement request is available.
-# MAC_MEAS_CONFIRM: enables RRC to acknowledge a MAC-MEAS-Ind.

These associate a set of measurements (described by the MAC_MEAS_t structure) to a particular logical channel.  Each measurement has
an associated period (which can be infinite in the case of a one-shot measurement) and a set of triggers for generating event-driven
instantaneous measurement reports. The latter is typically used to signal events that correspond to degradation or loss of links with respect to
negotiated QoS values.  These consequently trigger reconfiguration of services by the network. The averaging parameters used by the
MAC measurement process is described by the structure MAC_AVG_t. Note that it is the responsibility
of the MAC-layer to relay PHY measurements which are directly available to the scheduling entity in the MAC-layer.
RRC does not directly act on the PHY measurement sub-system.

The primitives responsible for logical channel MAC configuration by the radio resource manager via RRC signaling are

-# MAC_CONFIG_REQ: enables RRC to configure a new logical channel description at the MAC layer.  The associated QoS
descriptor (MAC_LCHAN_DESC contains the parameters defining the logical channel resource scheduling.
-# MAC_CONFIG_CONFIRM: indicates to RRC that a given logical channel has or has not been configured.

The primitives responsible for indicating data synchronization (or loss of) to RRC are
-# MAC_SYNC : indicates to RRC that a physical link for a newly established logical channel has been used (reception) with
success.  The parameter passed indicates the logical channel id.  This represents the event of successfully decoding a particular logical
channel for the first time.  In the event that several logical channels share the same physical resources (e.g. BCCH and CCCH), it
shall only be sent for the logical channel with the lowest id.
-# MAC_OUT_OF_SYNC : UE primitive indicating to RRC that a physical link for a particular Node-B has been lossed.  The parameter
passed indicates the id of the Node-B.  This represents the event of detecting an unacceptable error rate on the CHBCH of the Node-B in question.
and can be used in the case of handover between Node-B's.  Note that this message is redundant since an equivalent way of obtaining the same
result could be based on an event-driven measurement report.
*/

#        define NB_SIG_CNX_CH 1
#        define NB_CNX_CH MAX_MOBILES_PER_ENB

#        define NB_SIG_CNX_UE 2 //MAX_MANAGED_RG_PER_MOBILE
#        define NB_CNX_UE 2//MAX_MANAGED_RG_PER_MOBILE

#define NUMBER_OF_MEASUREMENT_SUBBANDS 16

#define RRC_RACH_ASS_REQ 0
#define MAC_RACH_BW_REQ 1



/*! MAC/PHY Measurement Structure*/
/*! \brief Logical Channel ID structure */
typedef struct {
  unsigned short    Index __attribute__ ((packed));       //QOS CLASS?          /*!< \brief Index of logical channel*/
} LCHAN_ID;


typedef struct {
  char Rssi;                        /*!< \brief RSSI (dBm) on physical resources corresponding to logical channel*/
  char Sinr[NUMBER_OF_MEASUREMENT_SUBBANDS];                        /*!< \brief Average SINR (dB) on physical resources corresponding to logical channel*/
  uint8_t Harq_delay;         /*!< \brief Average number of transmission rounds (times 10) on transport channel associated with
              logical channel*/
  unsigned short Bler;              /*!< \brief Average block error rate (times 1000) on transport channel associated with logical channel*/
  uint8_t Spec_eff;           /*!< \brief Actual Spectral efficiency (bits/symbol times 10) of transport channel associated with logical channel*/
  uint8_t Qdepth;
  //uint8_t Tx_rate;
  //uint8_t Rx_rate;
} __attribute__ ((__packed__)) MAC_MEAS_T;
#define MAC_MEAS_T_SIZE sizeof(MAC_MEAS_T)

/*!\brief MAC/RLC Measurement Information
 */
typedef struct {
  char Rssi;                        /*!< \brief RSSI (dBm) on physical resources corresponding to logical channel*/
  char Sinr[NUMBER_OF_MEASUREMENT_SUBBANDS];                        /*!< \brief Average SINR (dB) on physical resources corresponding to logical channel*/
  uint8_t Harq_delay;         /*!< \brief Average number of transmission rounds (times 10) on transport channel associated with
              logical channel*/
  unsigned short Bler;              /*!< \brief Average block error rate (times 1000) on transport channel associated with logical channel*/
  uint8_t Spec_eff;           /*!< \brief Actual Spectral efficiency (bits/symbol times 10) of transport channel associated with logical channel*/
  uint8_t  rlc_sdu_buffer_occ;    /*!< \brief RLC SDU buffer occupancy */
  unsigned short rlc_sdu_loss_indicator; /*!< \brief RLC SDU Loss indicator */
} __attribute__ ((__packed__)) MAC_RLC_MEAS_T;
#define MAC_RLC_MEAS_T_SIZE sizeof(MAC_RLC_MEAS_T)





/*! MAC/PHY Measurement Averaging Parameters.  These are fixed-point 16-bit representations of the interval [0,1)*/

typedef struct {
  unsigned short Rssi_forgetting_factor;                        /*!< \brief Forgetting factor for RSSI averaging*/
  unsigned short Sinr_forgetting_factor;                        /*!< \brief Forgetting factor for SINR averaging*/
  unsigned short Harq_delay_forgetting_factor;         /*!< \brief Forgetting factor for HARQ delay averaging*/
  unsigned short Bler_forgetting_factor;              /*!< \brief Forgetting factor for block error rate (times 1000) averaging*/
  unsigned short Spec_eff_forgetting_factor;           /*!< \brief Forgetting factor for Spectral efficiency averaging*/
} __attribute__ ((__packed__))  MAC_AVG_T;

/*!\brief This primitive indicates to RRC that a measurement request is available */

typedef struct {
  LCHAN_ID  Lchan_id;           /*!< \brief Logical Channel ID*/
  unsigned short Process_id;         /*!< \brief Id of MAC measurement process*/
  MAC_MEAS_T Meas;
  char Meas_status;              /*!< \brief Latest set of MAC measurements performed in the current process*/
} __attribute__ ((__packed__))  MAC_MEAS_IND;

#define MAC_MEAS_IND_SIZE sizeof(MAC_MEAS_IND)

/*!\brief This primitive indicates to MAC that a measurement request was received
 */
typedef struct {
  //LCHAN_ID  Lchan_id;           /*!< \brief Logical Channel ID*/
  //unsigned short       Process_id;         /*!< \brief Id of new MAC measurement process*/
  char     Meas_status;           /*!< \brief Status (0-positive, 1-negative)*/
} __attribute__ ((__packed__))  MAC_MEAS_CONFIRM;

/*!\brief This primitive parametrizes a MAC measurement process
 */
typedef struct {
  MAC_MEAS_T Meas_trigger;      /*!< \brief Thresholds to trigger event driven measurement reports*/
  MAC_AVG_T Mac_avg;            /*!< \brief Set of forgetting factors for the averaging of the MAC measurement process*/
  unsigned int bo_forgetting_factor; /*!< \brief Forgetting factor for RLC buffer occupancy averaging*/
  unsigned int sdu_loss_trigger; /*!< \brief Trigger for RLC SDU losses*/
  unsigned short Rep_amount;    /*!< \brief Number of Measurements for this process, 0 means infinite*/
  unsigned short Rep_interval;  /*!< \brief Reporting interval between successive measurement reports in this process*/
} __attribute__ ((__packed__))  MAC_RLC_MEAS_DESC;



/*!\brief This primitive indicates to MAC that a (periodic) measurement request is requested for a particular logical channel
 */
typedef struct {
  LCHAN_ID Lchan_id;            /*!< \brief Logical Channel ID*/
  unsigned short  UE_eNB_index;
  unsigned short Process_id;         /*!< \brief Id of new MAC measurement process*/
  //  MAC_MEAS_DESC Meas_desc;/*!< \brief Measurement descriptor associated with Logical channel*/
  MAC_MEAS_T Meas_trigger;      /*!< \brief Thresholds to trigger event driven measurement reports*/
  MAC_AVG_T Mac_avg;            /*!< \brief Set of forgetting factors for the averaging of the MAC measurement process*/
  unsigned short Rep_amount;    /*!< \brief Number of Measurements for this process, 0 means infinite*/
  unsigned short Rep_interval;  /*!< \brief Reporting interval between successive measurement reports in this process*/
  MAC_MEAS_T Mac_meas;      /*!< \brief Measure structure to be filled by the MAC  */
} __attribute__ ((__packed__))  MAC_MEAS_REQ;
#define MAC_MEAS_REQ_SIZE sizeof(MAC_MEAS_REQ)

typedef struct MEAS_REQ_TABLE_ENTRY {
  MAC_MEAS_REQ Mac_meas_req;
  unsigned int Last_report_frame;
  unsigned int Next_check_frame;
  uint8_t Status;
  uint8_t Meas_req_status;
  uint8_t Rx_activity;
  //uint8_t Meas_Direction;//???
  struct MEAS_REQ_TABLE_ENTRY *Next_entry;
  struct MEAS_REQ_TABLE_ENTRY *Prev_entry;
} __attribute__ ((__packed__)) MAC_MEAS_REQ_ENTRY;


/*!\brief This primitive parametrizes an RLC measurement process
 */
typedef struct {
  unsigned int bo_forgetting_factor; /*!< \brief Forgetting factor for buffer occupancy averaging*/
  unsigned int sdu_loss_trigger; /*!< \brief Trigger for RLC sdu losses*/
} __attribute__ ((__packed__))  RLC_MEAS_DESC;

//#define NUM_MEAS_REQ_MAX 100 //from rrm






/*! \brief MAC Logical Channel Descriptor
 */


/*!\brief This primitive indicates to RRC that a particular logical channel has been established and that successful transmission has
been received.  The parameter passed is the logical channel id.
 */
typedef struct MAC_SYNC {
  LCHAN_ID Lchan_id;       /*!< \brief Logical Channel ID*/
} MAC_SYNC;

/*!\brief This primitive indicates to RRC (UE) that a particular Node-B has been lossed and passes CHBCH logical channel
node_id as parameter
 */
typedef struct MAC_OUT_OF_SYNC {
  unsigned short  Node_id;       /*!< \brief Logical Channel ID*/
} MAC_OUT_OF_SYNC;


/*!\brief This primitive enables RRC to configure a new logical channel description at the MAC layer.
 */
typedef struct {
  LCHAN_ID   Lchan_id;              /*!< \brief Logical Channel ID*/
  //   unsigned short UE_eNB_index; //ID of CH who configued the LC (Need this to identify LC in the UE side)
  uint8_t Lchan_type; // DCCH_LCHAN, DTCH_LCHAN
  LCHAN_DESC  Lchan_desc[2];  /*!< \brief Logical Channel QoS Descriptor (MAC component) */
  uint8_t L3_info_type;
  uint8_t L3_info[16];
  unsigned short UE_eNB_index;
} __attribute__ ((__packed__))  MAC_CONFIG_REQ;
#define MAC_CONFIG_REQ_SIZE sizeof(MAC_CONFIG_REQ)


/*!\brief This primitive indicates to RRC that a given logical channel has or has not been configured.
*/
typedef struct {
  LCHAN_ID Lchan_id;        /*!< \brief Logical Channel ID*/
  uint8_t    Config_status;   /*!< \brief Configuration status (0-positive, 1-negative)*/
} __attribute__ ((__packed__))  MAC_CONFIG_CONFIRM;

/**@}*/


/*
** @addtogroup _mac_impl_ MAC Layer (MAC) Reference Implementation
 *@{
 */

//#include "LAYER2/MAC/defs.h"



typedef struct {
  char Sinr_meas[NB_CNX_CH+1][NUMBER_OF_MEASUREMENT_SUBBANDS];
  char Wideband_sinr;
  uint8_t Forg_fact;
  unsigned short Rep_interval;
  uint8_t Status;
  unsigned int Last_report_frame;
  unsigned int Next_check_frame;
  uint8_t Active;
} __attribute__ ((__packed__)) DEFAULT_UE_MEAS;
#define DEFAULT_UE_MEAS_SIZE sizeof(DEFAULT_UE_MEAS)

typedef struct {  //From RRC
  unsigned short UE_index;
  uint8_t Forg_fact;
  unsigned short Rep_interval;;
} __attribute__ ((__packed__)) DEFAULT_MEAS_REQ;
#define DEFAULT_MEAS_REQ_SIZE sizeof(DEFAULT_MEAS_REQ)

typedef struct {  //To RRC
  unsigned short UE_index __attribute__ ((packed));
  char Sinr_meas[NB_CNX_CH+1];
  uint8_t Rb_active[NB_RAB_MAX];
} __attribute__ ((__packed__)) DEFAULT_MEAS_IND;
#define DEFAULT_MEAS_IND_SIZE sizeof(DEFAULT_MEAS_IND)


typedef struct {
  char Sinr_meas[NB_CNX_CH+1][NUMBER_OF_MEASUREMENT_SUBBANDS];
  char Wideband_sinr;
  unsigned int cqi;
  uint8_t Forg_fact;
  unsigned short Rep_interval;
  unsigned int Last_report_frame;
  unsigned int Next_check_frame;
  uint8_t Status; //IDLE,NEED_rADIO_CONFIG, RADIO_CONFIG_TX, RADIO_CONFIG_ok
  uint8_t Active;
} __attribute__ ((__packed__)) DEFAULT_CH_MEAS;
#define DEFAULT_eNB_MEAS_SIZE sizeof(DEFAULT_eNB_MEAS)



#ifndef OPENAIR2_IN

typedef struct {  //RRC_INTERFACE_FUNCTIONS
  unsigned int Frame_index;
  unsigned short UE_index[NB_MODULES_MAX][NB_SIG_CNX_UE];
  uint8_t  eNB_id[NB_MODULES_MAX][NB_CNX_UE];
  //  L2_ID UE_id[NB_MODULES_MAX][NB_CNX_CH];
  uint8_t UE_id[NB_MODULES_MAX][NB_CNX_CH][5];
  void (*openair_rrc_top_init)(void);
  char (*openair_rrc_eNB_init)(uint8_t );
  char (*openair_rrc_UE_init)(uint8_t, uint8_t);
  RRC_status_t (*rrc_rx_tx)(uint8_t,uint32_t,uint8_t,uint8_t);
  uint8_t (*mac_rrc_data_ind)(uint8_t,uint32_t,uint16_t,uint8_t *,uint16_t,eNB_flag_t eNB_flag, uint8_t eNB_index);
  uint8_t (*mac_rrc_data_req)(uint8_t,uint32_t,uint16_t,uint8_t,uint8_t *,eNB_flag_t eNB_flag, uint8_t eNB_index);
  void (*mac_rrc_meas_ind)(uint8_t,MAC_MEAS_REQ_ENTRY*);
  void  (*def_meas_ind)(uint8_t, uint8_t);
  void (*rrc_data_indP)  (module_id_t , rb_id_t , sdu_size_t , char*);
  void (*fn_rrc)  (void);
  uint8_t (*get_rrc_status)(uint8_t Mod_id,uint8_t eNB_flag,uint8_t eNB_index);
  double (*rrc_get_estimated_ue_distance) (module_id_t Mod_id, uint8_t UE_id, uint8_t CC_id, uint8_t loc_type);
} RRC_XFACE;


typedef struct {
  //MAC_INTERFACE_FUNCTIONS
  unsigned int frame;
  unsigned short Node_id[NB_MODULES_MAX];
  char Is_cluster_head[NB_MODULES_MAX];
  void (*macphy_exit)(const char *);          /*  Pointer function that stops the low-level scheduler due an exit condition */
  unsigned short (*mac_config_req)(uint8_t,uint8_t,MAC_CONFIG_REQ*);
  MAC_MEAS_REQ_ENTRY* (*mac_meas_req)(uint8_t ,  MAC_MEAS_REQ*);
  void (*mac_out_of_sync_ind)(uint8_t,uint32_t,unsigned short);
  //RLC_INTERFACE_FUNCTIONS
  void (*pdcp_run)(void);
  void (*pdcp_data_req)(module_id_t, rb_id_t, sdu_size_t, char*);
  signed int (*rrc_rlc_config_req)(unsigned int, unsigned int, unsigned int, unsigned int, rlc_info_t );
  int (*rrc_mac_config_req)(uint8_t Mod_id,uint8_t eNB_flag,uint8_t UE_id,uint8_t eNB_index,
                            LTE_RadioResourceConfigCommonSIB_t *radioResourceConfigCommon,
                            struct LTE_PhysicalConfigDedicated *physicalConfigDedicated,
                            LTE_MAC_MainConfig_t *mac_MainConfig,
                            long logicalChannelIdentity,
                            LTE_LogicalChannelConfig_t *logicalChannelConfig,
                            LTE_MeasGapConfig_t *measGapConfig,
                            LTE_TDD_Config_t *tdd_Config,
                            uint8_t *SIwindowsize,
                            uint16_t *SIperiod
#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
                            ,
                            MBMS_flag_t MBMS_Flag,
                            struct LTE_MBSFN_SubframeConfigList *mbsfn_SubframeConfigList,
                            LTE_MBSFN_AreaInfoList_r9_t *mbsfn_AreaInfoList,
                            struct LTE_PMCH_InfoList_r9 *pmch_InfoList
#endif
                           );
  unsigned int (*mac_rlc_data_req)(module_id_t, unsigned int, const unsigned int,char*
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                  ,uint32_t
                                  ,uint32_t
#endif
                                   );
  void (*mac_rlc_data_ind)(module_id_t, logical_chan_id_t, char*, tb_size_t, num_tb_t, crc_t* );
  mac_rlc_status_resp_t (*mac_rlc_status_ind)     (module_id_t enb_mod_idP, module_id_t ue_mod_idP, frame_t frameP, sub_frame_t subframeP, eNB_flag_t eNB_flagP, MBMS_flag_t MBMS_flagP,
      logical_chan_id_t channel_idP, tb_size_t tb_sizeP
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
      ,uint32_t sourceL2Id
      ,uint32_t destinationL2Id
#endif
      );
  signed int (*rrc_rlc_data_req)(module_id_t, rb_id_t, mui_t, confirm_t, sdu_size_t, char *);
  void (*rrc_rlc_register_rrc) (void (*rrc_data_indP)(module_id_t , rb_id_t , sdu_size_t , char* ),
                                void  (*rrc_data_confP) (module_id_t , rb_id_t , mui_t ) ) ;
  void (*mrbch_phy_sync_failure) (uint8_t Mod_id, frame_t frame, uint8_t Free_ch_index);
  void (*dl_phy_sync_success) (uint8_t Mod_id, frame_t frame, uint8_t eNB_index);
} MAC_RLC_XFACE;


#endif



//#define IDLE 0
#define NEED_RADIO_CONFIG 3
#define RADIO_CONFIG_TX 2
#define RADIO_CONFIG_OK 1

#define MEAS_CONFIGURED 4
#define MEAS_OK 5
#define MEAS_TRIGGER 6
#define RRC_MEAS_REPORT_REQ 7
#define MEAS_REPORT 8

#define RAB_MEAS_IND 0
#define DEF_MEAS_IND 1

#define ADD_LC 0
#define REMOVE_LC 1

#endif


