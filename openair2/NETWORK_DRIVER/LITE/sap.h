#ifndef OAI_SAP_H
#define OAI_SAP_H
typedef unsigned short     OaiNwDrvRadioBearerId_t;
typedef unsigned int       OaiNwDrvSapId_t;              // Id of the QoS SAP to use
typedef unsigned short     OaiNwDrvQoSTrafficClass_t;    // QoS traffic class requested
typedef unsigned int       OaiNwDrvLocalConnectionRef_t; // local identifier
typedef unsigned short     OaiNwDrvCellID_t;             // ID of the cell for connection
typedef unsigned short     OaiNwDrvNumRGsMeas_t;         // number of RGs that could be measured
typedef unsigned int       OaiNwDrvSigLevel_t;           // Signal level measured

#define OAI_NW_DRV_SAPI_CX_MAX                 2
#define OAI_NW_DRV_MAX_MEASURE_NB              5
#define OAI_NW_DRV_PRIMITIVE_MAX_LENGTH        180  // maximum length of a NAS primitive
#define OAI_NW_DRV_SAPI_MAX                    4
#define OAI_NW_DRV_RAB_INPUT_SAPI     2
#define OAI_NW_DRV_RAB_OUTPUT_SAPI      3
#define OAI_NW_DRV_MAX_RABS     8 * 64 //NB_RAB_MAX * MAX_MOBILES_PER_RG  //27   // = MAXURAB
#define OAI_NW_DRV_LIST_RB_MAX 32
#endif
