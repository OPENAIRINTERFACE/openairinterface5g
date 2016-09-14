/***************************************************************************
                          graal_sap.h  -  description
                             -------------------
    copyright            : (C) 2002 by Eurecom
    email                : michelle.wetterwald@eurecom.fr
                           yan.moret@eurecom.fr
 ***************************************************************************

 ***************************************************************************/

#ifndef _NAS_SAP_H
#define _NAS_SAP_H


// RT-FIFO identifiers ** must be identical to Access Stratum as_sap.h and rrc_sap.h

#define RRC_DEVICE_GC          RRC_SAPI_GCSAP
#define RRC_DEVICE_NT          RRC_SAPI_NTSAP
#define RRC_DEVICE_DC_INPUT0   RRC_SAPI_DCSAP_IN
#define RRC_DEVICE_DC_OUTPUT0  RRC_SAPI_DCSAP_OUT


//FIFO indexes in control blocks

#define NAS_DC_INPUT_SAPI   0
#define NAS_DC_OUTPUT_SAPI  1
#define NAS_SAPI_CX_MAX     2

#define NAS_GC_SAPI         0
#define NAS_NT_SAPI         1

#define NAS_RAB_INPUT_SAPI      2
#define NAS_RAB_OUTPUT_SAPI     3


#define NAS_SAPI_MAX       4




#endif



