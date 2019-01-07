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

/*********************************************************************
                          rrc_nas_primitives.h  -  description
                             -------------------
    begin                : Jan 11, 2002
    copyright            : (C) 2001 by Eurecom
    email                : navid.nikaein@eurecom.fr
                           lionel.gauthier@eurecom.fr
                           knopp@eurecom.fr
 *********************************************************************
  Define RRC external interface primitives
 ********************************************************************/
#ifndef __RRC_NASPRIM_H__
#    define __RRC_NASPRIM_H__

//----------------------------------------------------------
// Primitives
//----------------------------------------------------------
#    define INFO_BROADCAST_REQ  1
#    define INFO_BROADCAST_IND  2
#    define CONN_ESTABLISH_REQ  3
#    define CONN_ESTABLISH_IND  4
#    define CONN_ESTABLISH_RESP 5
#    define CONN_ESTABLISH_CNF  6
#    define CONN_RELEASE_REQ    7
#    define CONN_RELEASE_IND    8
#    define DATA_TRANSFER_REQ   9
#    define DATA_TRANSFER_IND   10
#    define RB_ESTABLISH_REQ    11
#    define RB_ESTABLISH_IND    12
#    define RB_ESTABLISH_CNF    14
#    define RB_RELEASE_REQ      15
#    define RB_RELEASE_IND      16
#    define MEASUREMENT_IND     17
#    define CONN_LOSS_IND       18
#    define PAGING_REQ          19
#    define NOTIFICATION_IND    20

//----------------------------------------------------------
// Constants
//----------------------------------------------------------
// Define max length authorized (to be updated later)
#    define NAS_MAX_LENGTH 180  // maximum length of a NAS primitive
#    define NAS_TL_SIZE  4      // size of the Type+Length fields of the primitive
#    define NAS_DATA_MAX_LENGTH NAS_MAX_LENGTH - NAS_TL_SIZE    //remaining bytes for the message
#    define MAX_RABS     8 * 64 //NB_RAB_MAX * MAX_MOBILES_PER_RG  //27   // = MAXURAB
#    define MAX_MEASURE_NB  5

//Connection Establishment status
// UE
#    define TERMINATED         0
#    define ABORTED          1
#    define ALREADY_CONNECTED  2
// RG + RB Establishment Confirm
#    define ACCEPTED   0
#    define FAILURE  1
//----------------------------------------------------------

// Parameter types
//----------------------------------------------------------
typedef unsigned int     nasPeriod_t;    // Broadcast repetition period
typedef unsigned short     nasBroadcastCategory_t; // identifies the type of data to broadcast (IP signalling/ List of neighbors)
typedef unsigned char      nasIMEI_t[14];  // 14 digits of IMEI
typedef unsigned short     nasDataLength_t;        // Length of the data in the primitive
typedef unsigned short     nasCellID_t;    // ID of the cell for connection
typedef unsigned int     nasLocalConnectionRef_t;        // local identifier
typedef unsigned short     nasConnectionStatus_t;  // connection establishment status
typedef unsigned short     nasReleaseCause_t;      // connection release cause
typedef unsigned short     nasSignalingPriority_t; // priority to use srb3 or srb4
typedef unsigned short     nasRadioBearerId_t;
typedef unsigned short    nasQoSTrafficClass_t;   //QoS traffic class requested
typedef unsigned short     nasIPdscp_t;    // DSCP code transported to service NAS
typedef struct nasRBDef {
  nasRadioBearerId_t rbId;
  nasQoSTrafficClass_t QoSclass;
  nasIPdscp_t dscp;
} nasrbParms_t;
typedef unsigned int    nasSapId_t;     // Id of the QoS SAP to use
typedef unsigned short     nasRBEstablishStatus_t; // radio bearer establishment status
typedef unsigned short     nasNumRBsInList_t;      // number of RBs defined in the list
typedef nasrbParms_t nasRBList_t[MAX_RABS];     // List of Rbs for handover
typedef unsigned short     nasNumRGsMeas_t;        // number of RGs that could be measured
typedef unsigned int     nasPagingUEId_t;        // Cell_Id of the mobile, = Local Conn Ref
typedef unsigned int     nasSigLevel_t;  // Signal level measured
typedef struct nasMeasures {
  nasCellID_t cell_id;
  nasSigLevel_t level;
} nasMeasures_t;

//----------------------------------------------------------
// Primitive definitions
//----------------------------------------------------------
// -- SAP-GC
struct NASInfoBroadcastReq {
  nasPeriod_t period;          // 0 = one-shot, otherwise in  x 10 ms
  nasBroadcastCategory_t category;
  nasDataLength_t nasDataLength;
};
struct NASInfoBroadcastInd {
  nasDataLength_t nasDataLength;
};
struct nas_ue_gc_element {
  unsigned short type;
  unsigned short length;
  union {
    struct NASInfoBroadcastInd broadcast_ind;
  } nasUEGCPrimitive;
};
struct nas_rg_gc_element {
  unsigned short type;
  unsigned short length;
  union {
    struct NASInfoBroadcastReq broadcast_req;
  } nasRGGCPrimitive;
};

// -- SAP-DC
struct NASConnEstablishReq {
  nasLocalConnectionRef_t localConnectionRef;  //provided by NAS
  nasCellID_t     cellId;
};
struct NASConnEstablishInd {
  nasLocalConnectionRef_t localConnectionRef;
  nasIMEI_t InterfaceIMEI;
};
struct NASConnEstablishConf {
  nasLocalConnectionRef_t localConnectionRef;
  nasConnectionStatus_t status;        // can be : Accepted, Failure
  nasNumRBsInList_t num_RBs;    // actual number of RBs in the list
  nasRBList_t     RB_List;
};
struct NASConnEstablishResp {
  nasLocalConnectionRef_t localConnectionRef;
  nasIMEI_t InterfaceIMEI;
  nasConnectionStatus_t status;        // can be : Terminated, Aborted , Already_Connected
};
struct NASConnReleaseReq {
  nasLocalConnectionRef_t localConnectionRef;
  nasReleaseCause_t releaseCause;
};
struct NASConnReleaseInd {
  nasLocalConnectionRef_t localConnectionRef;
  nasReleaseCause_t releaseCause;
};
struct NASConnLossInd {
  nasLocalConnectionRef_t localConnectionRef;
};
struct NASDataReq {
  nasLocalConnectionRef_t localConnectionRef;
  nasSignalingPriority_t priority;
  nasDataLength_t nasDataLength;
};
struct NASDataInd {
  nasLocalConnectionRef_t localConnectionRef;
  nasSignalingPriority_t priority;
  nasDataLength_t nasDataLength;
};
struct NASrbEstablishReq {
  nasLocalConnectionRef_t localConnectionRef;
  nasRadioBearerId_t rbId;
  nasQoSTrafficClass_t QoSclass;
  nasIPdscp_t dscp;
};
struct NASrbEstablishInd {
  nasLocalConnectionRef_t localConnectionRef;
  nasRadioBearerId_t rbId;
  nasQoSTrafficClass_t QoSclass;
  nasIPdscp_t dscp;
  nasSapId_t sapId;
};
struct NASrbEstablishConf {
  nasLocalConnectionRef_t localConnectionRef;
  nasRadioBearerId_t rbId;
  nasSapId_t sapId;
  nasRBEstablishStatus_t status;       // can be : Accepted, Failure
};
struct NASrbReleaseReq {
  nasLocalConnectionRef_t localConnectionRef;
  nasRadioBearerId_t rbId;
};
struct NASrbReleaseInd {
  nasLocalConnectionRef_t localConnectionRef;
  nasRadioBearerId_t rbId;
};
struct NASMeasureReq {
  nasLocalConnectionRef_t localConnectionRef;
};
struct NASMeasureInd {
  nasLocalConnectionRef_t localConnectionRef;
  nasNumRGsMeas_t nb_rg;
  nasMeasures_t measures[MAX_MEASURE_NB];
};

/*****
 * UE Primitives
 *****/
struct nas_ue_dc_element {
  unsigned short type;
  unsigned short length;
  union {
    struct NASConnEstablishReq conn_establish_req;
    struct NASConnEstablishResp conn_establish_resp;

    //    struct NASConnReleaseInd conn_release_ind;
    struct NASConnReleaseReq conn_release_req;
    struct NASConnLossInd conn_loss_ind;
    struct NASDataReq data_transfer_req;
    struct NASDataInd data_transfer_ind;
    struct NASrbEstablishInd rb_establish_ind;
    struct NASrbReleaseInd rb_release_ind;
    struct NASMeasureInd measurement_ind;
    struct NASMeasureReq measurement_req;
  } nasUEDCPrimitive;
};

/*****
 * RG Primitives
 *****/
struct nas_rg_dc_element {
  unsigned short type;
  unsigned short length;
  union {
    struct NASConnEstablishInd conn_establish_ind;
    struct NASConnEstablishConf conn_establish_conf;
    struct NASConnReleaseInd conn_release_ind;

    //    struct NASConnReleaseReq conn_release_req;
    struct NASConnLossInd conn_loss_ind;
    struct NASDataReq data_transfer_req;
    struct NASDataInd data_transfer_ind;
    struct NASrbEstablishReq rb_establish_req;
    struct NASrbEstablishConf rb_establish_conf;
    struct NASrbReleaseReq rb_release_req;
  } nasRGDCPrimitive;
};

// -- SAP-NT
struct NASPagingReq {
  nasPagingUEId_t UeId;
  nasDataLength_t nasDataLength;
};
struct NASNotificationInd {
  nasDataLength_t nasDataLength;
};
struct nas_ue_nt_element {
  unsigned short type;
  unsigned short length;
  union {
    struct NASNotificationInd notification_ind;
  } nasUENTPrimitive;
};
struct nas_rg_nt_element {
  unsigned short type;
  unsigned short length;
  union {
    struct NASPagingReq paging_req;
  } nasRGNTPrimitive;
};

#endif
