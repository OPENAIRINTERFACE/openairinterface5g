/*
                                nfapi_nr_interface.h
                             -------------------
  AUTHOR  : Raymond Knopp, Guy de Souza, WEI-TAI CHEN
  COMPANY : EURECOM, NTUST
  EMAIL   : Lionel.Gauthier@eurecom.fr, desouza@eurecom.fr, kroempa@gmail.com
*/

#ifndef _NFAPI_NR_INTERFACE_H_
#define _NFAPI_NR_INTERFACE_H_

//These TLVs are used exclusively by nFAPI
#define NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV4_TAG 0x0100
#define NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV6_TAG 0x0101
#define NFAPI_NR_NFAPI_P7_VNF_PORT_TAG 0x0102
#define NFAPI_NR_NFAPI_P7_PNF_ADDRESS_IPV4_TAG 0x0103
#define NFAPI_NR_NFAPI_P7_PNF_ADDRESS_IPV6_TAG 0x0104
#define NFAPI_NR_NFAPI_P7_PNF_PORT_TAG 0x0105
#define NFAPI_NR_NFAPI_DL_TTI_TIMING_OFFSET 0x0106
#define NFAPI_NR_NFAPI_UL_TTI_TIMING_OFFSET 0x0107
#define NFAPI_NR_NFAPI_UL_DCI_TIMING_OFFSET 0x0108
#define NFAPI_NR_NFAPI_TX_DATA_TIMING_OFFSET 0x0109
#define NFAPI_NR_NFAPI_TIMING_WINDOW_TAG 0x011E
#define NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG 0x011F
#define NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG 0x0120

typedef enum {
  NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED=1,
  NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED=0
} nfapi_nr_cce_reg_mapping_type_e;

typedef enum {
  NFAPI_NR_CSET_CONFIG_MIB_SIB1 = 0,
  NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG, // implicit assumption of coreset Id other than 0
} nfapi_nr_coreset_config_type_e;

typedef enum {
  NFAPI_NR_DMRS_TYPE1,
  NFAPI_NR_DMRS_TYPE2
} nfapi_nr_dmrs_type_e;

typedef enum {
  NFAPI_NR_SRS_BEAMMANAGEMENT = 0,
  NFAPI_NR_SRS_CODEBOOK = 1,
  NFAPI_NR_SRS_NONCODEBOOK = 2,
  NFAPI_NR_SRS_ANTENNASWITCH = 3
} nfapi_nr_srs_usage_e;

#endif
