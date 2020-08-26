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
* Author and copyright: Laurent Thomas, open-cells.com
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


#ifndef __SPLIT_HEADERS_H
#define __SPLIT_HEADERS_H

#include <stdint.h>
#include <stdbool.h>
#include <openair1/PHY/defs_eNB.h>
#include <common/utils/telnetsrv/telnetsrv_proccmd.h>

#define CU_PORT "7878"
#define DU_PORT "8787"
#define SPLIT73_CU 1
#define SPLIT73_DU 2
extern int split73;

#define MTU 65536
#define UDP_TIMEOUT 900000L // in micro  second (struct timeval, NOT struct timespec)
// linux may timeout for a much longer time (up to 10ms)
#define blockAlign 32 //bytes to align memory for SIMD copy (256 bits vectors)

// FS6 transport configuration and handler
typedef struct {
  char *sourceIP;
  char *sourcePort;
  char *destIP;
  char *destPort;
  struct addrinfo *destAddr;
  int sockHandler;
} UDPsock_t;

#define CTsentCUv0 0xA500
#define CTsentDUv0 0x5A00

// Main FS6 transport layer header
// All packets starts with this header
typedef struct commonUDP_s {
  uint64_t timestamp; // id of the group (subframe for LTE)
  uint16_t nbBlocks;       // total number of blocks for this timestamp
  uint16_t blockID;        // id: 0..nbBocks-1
  uint16_t contentType;    // defines the content format
  uint16_t contentBytes;   // will be sent in a UDP packet, so must be < 2^16 bytes
  uint64_t senderClock;
} commonUDP_t;

// FS6 UL common header (DU to CU)
// gives the RACH detection data and is always sent to inform the CU that a subframe arrived
typedef struct {
  uint16_t max_preamble[4];
  uint16_t max_preamble_energy[4];
  uint16_t max_preamble_delay[4];
  uint16_t avg_preamble_energy[4];
} fs6_ul_t;

// FS6 DL common header (CU to DU)
// gives the DCI configuration from each subframe
typedef struct {
  uint8_t pbch_pdu[4];
  int num_pdcch_symbols;
  int num_dci;
  DCI_ALLOC_t dci_alloc[8];
  int num_mdci;
  int amp;
  LTE_eNB_PHICH phich_vars;
  uint64_t DuClock;
  uint64_t CuSpentMicroSec;
} fs6_dl_t;

// a value to type all sub packets,
// to detect errors, and to be able to extend to other versions
// the first byte of each sub structure should match one of these values
enum pckType {
  fs6UlConfig=25,
  fs6DlConfig=26,
  fs6ULConfigCCH=27,
  fs6ULsch=28,
  fs6ULcch=29,
  fs6ULindicationHarq=40,
  fs6ULindicationSr=41,
};

// CU to DU definition of a future UL subframe decode
// defines a UE future data plane
typedef struct {
  enum pckType type:8;
  uint16_t UE_id;
  int8_t harq_pid;
  UE_type_t ue_type;

  uint8_t dci_alloc;
  uint8_t rar_alloc;
  SCH_status_t status;
  uint8_t Msg3_flag;
  uint8_t subframe;
  uint32_t frame;
  uint8_t handled;
  uint8_t phich_active;
  uint8_t phich_ACK;
  uint16_t previous_first_rb;
  uint32_t B;
  uint32_t G;
  UCI_format_t uci_format;
  uint8_t Or2;
  uint8_t o_RI[2];
  uint8_t o_ACK[4];
  uint8_t O_ACK;
  uint8_t o_RCC;
  int16_t q_ACK[MAX_ACK_PAYLOAD];
  int16_t q_RI[MAX_RI_PAYLOAD];
  uint32_t RTC[MAX_NUM_ULSCH_SEGMENTS];
  uint8_t ndi;
  uint8_t round;
  uint8_t rvidx;
  uint8_t Nl;
  uint8_t n_DMRS;
  uint8_t previous_n_DMRS;
  uint8_t n_DMRS2;
  int32_t delta_TF;
  uint32_t repetition_number ;
  uint32_t total_number_of_repetitions;

  uint16_t harq_mask;
  uint16_t nb_rb;
  uint8_t Qm;
  uint16_t first_rb;
  uint8_t O_RI;
  uint8_t Or1;
  uint16_t Msc_initial;
  uint8_t Nsymb_initial;
  uint8_t V_UL_DAI;
  uint8_t srs_active;
  uint32_t TBS;
  uint8_t Nsymb_pusch;
  uint8_t Mlimit;
  uint8_t max_turbo_iterations;
  uint8_t bundling;
  uint16_t beta_offset_cqi_times8;
  uint16_t beta_offset_ri_times8;
  uint16_t beta_offset_harqack_times8;
  uint8_t Msg3_active;
  uint16_t rnti;
  uint8_t cyclicShift;
  uint8_t cooperation_flag;
  uint8_t num_active_cba_groups;
  uint16_t cba_rnti[4];//NUM_MAX_CBA_GROUP];
} fs6_dl_ulsched_t;

// CU to DU defintion of a DL packet for a given UE
// The data itself is padded at the end of this structure
typedef struct {
  enum pckType type:8;
  int UE_id;
  int8_t harq_pid;
  uint16_t rnti;
  int16_t sqrt_rho_a;
  int16_t sqrt_rho_b;
  CEmode_t CEmode:8;
  uint16_t nb_rb;
  uint8_t Qm;
  int8_t Nl;
  uint8_t pdsch_start;
  uint8_t sib1_br_flag;
  uint16_t i0;
  uint32_t rb_alloc[4];
  int dataLen;
} fs6_dl_uespec_t;

// CU to DU definition of CCH channel
typedef struct {
  int16_t UE_id;
  LTE_eNB_UCI cch_vars;
} fs6_dl_uespec_ulcch_element_t;

// header to group all UE CCH channels definitions in one UDP packet
typedef struct {
  enum pckType type:8;
  int16_t nb_active_ue;
} fs6_dl_uespec_ulcch_t;

// code internal, not transmitted as this
typedef struct {
  int ta;
}  ul_propagation_t;

// One UE UL data, data plane, UE data appended after the header
typedef struct {
  enum pckType type:8;
  short UE_id;
  uint8_t harq_id;
  uint8_t segment;
  int segLen;
  int r_offset;
  int G;
  int ulsch_power[2];
  uint8_t o_ACK[4];
  uint8_t O_ACK;
  int ta;
  uint8_t o[MAX_CQI_BYTES];
  uint8_t cqi_crc_status;
} fs6_ul_uespec_t;

// UL UCI (control plane), per UE
typedef struct {
  enum pckType type:8;
  int UEid;
  int frame;
  int subframe;
  LTE_eNB_UCI uci;
  uint8_t harq_ack[4];
  uint8_t tdd_mapping_mode;
  uint16_t tdd_multiplexing_mask;
  unsigned short n0_subband_power_dB;
  uint16_t rnti;
  int32_t stat;
} fs6_ul_uespec_uci_element_t;

// all segments UCI grouped in one UDP packet
typedef struct {
  enum pckType type:8;
  int16_t nb_active_ue;
}  fs6_ul_uespec_uci_t;


bool createUDPsock (char *sourceIP, char *sourcePort, char *destIP, char *destPort, UDPsock_t *result);
int receiveSubFrame(UDPsock_t *sock, void *bufferZone,  int bufferSize, uint16_t contentType);
int sendSubFrame(UDPsock_t *sock, void *bufferZone, ssize_t secondHeaderSize, uint16_t contentType);

#define initBufferZone(xBuf) \
  uint8_t xBuf[FS6_BUF_SIZE];   \
  ((commonUDP_t *)xBuf)->nbBlocks=0;

#define hUDP(xBuf) ((commonUDP_t *)xBuf)
#define hDL(xBuf)  ((fs6_dl_t*)(((commonUDP_t *)xBuf)+1))
#define hUL(xBuf)  ((fs6_ul_t*)(((commonUDP_t *)xBuf)+1))
#define hDLUE(xBuf) ((fs6_dl_uespec_t*) (((fs6_dl_t*)(((commonUDP_t *)xBuf)+1))+1))
#define hTxULUE(xBuf) ((fs6_dl_ulsched_t*) (((fs6_dl_t*)(((commonUDP_t *)xBuf)+1))+1))
#define hTxULcch(xBuf) ((fs6_dl_uespec_ulcch_t*) (((fs6_dl_t*)(((commonUDP_t *)xBuf)+1))+1))
#define hULUE(xBuf) ((fs6_ul_uespec_t*) (((fs6_ul_t*)(((commonUDP_t *)xBuf)+1))+1))
#define hULUEuci(xBuf) ((fs6_ul_uespec_uci_t*) (((fs6_ul_t*)(((commonUDP_t *)xBuf)+1))+1))

static inline size_t alignedSize(uint8_t *ptr) {
  commonUDP_t *header=(commonUDP_t *) ptr;
  return ((header->contentBytes+sizeof(commonUDP_t)+blockAlign-1)/blockAlign)*blockAlign;
}

static inline void *commonUDPdata(uint8_t *ptr) {
  return (void *) (((commonUDP_t *)ptr)+1);
}

void setAllfromTS(uint64_t TS, L1_rxtx_proc_t *proc);
void sendFs6Ulharq(enum pckType type, int UEid, PHY_VARS_eNB *eNB,LTE_eNB_UCI *uci, int frame, int subframe, uint8_t *harq_ack, uint8_t tdd_mapping_mode, uint16_t tdd_multiplexing_mask,
                   uint16_t rnti,  int32_t stat);
void sendFs6Ul(PHY_VARS_eNB *eNB, int UE_id, int harq_pid, int segmentID, int16_t *data, int dataLen, int r_offset);
void *cu_fs6(void *arg);
void *du_fs6(void *arg);
void fill_rf_config(RU_t *ru, char *rf_config_file);
int init_rf(RU_t *ru);
void rx_rf(RU_t *ru, L1_rxtx_proc_t *proc);
void tx_rf(RU_t *ru, L1_rxtx_proc_t *proc);
void common_signal_procedures (PHY_VARS_eNB *eNB,int frame, int subframe);
void pmch_procedures(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc, int fembms_flag);
bool dlsch_procedures(PHY_VARS_eNB *eNB,
                      L1_rxtx_proc_t *proc,
                      int harq_pid,
                      LTE_eNB_DLSCH_t *dlsch,
                      LTE_eNB_UE_stats *ue_stats) ;
void postDecode(L1_rxtx_proc_t *proc, notifiedFIFO_elt_t *req);
void pdsch_procedures(PHY_VARS_eNB *eNB,
                      L1_rxtx_proc_t *proc,
                      int harq_pid,
                      LTE_eNB_DLSCH_t *dlsch,
                      LTE_eNB_DLSCH_t *dlsch1);
void srs_procedures(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc);
void uci_procedures(PHY_VARS_eNB *eNB,
                    L1_rxtx_proc_t *proc);
void ocp_rx_prach(PHY_VARS_eNB *eNB,
		  L1_rxtx_proc_t *proc,
		  RU_t *ru,
		  uint16_t *max_preamble,
		  uint16_t *max_preamble_energy,
		  uint16_t *max_preamble_delay,
		  uint16_t *avg_preamble_energy,
		  uint16_t Nf,
		  uint8_t tdd_mapindex,
		  uint8_t br_flag);
void rx_prach0(PHY_VARS_eNB *eNB,
               RU_t *ru,
	       int frame_prach,
	       int subframe,
               uint16_t *max_preamble,
               uint16_t *max_preamble_energy,
               uint16_t *max_preamble_delay,
               uint16_t *avg_preamble_energy,
               uint16_t Nf,
               uint8_t tdd_mapindex,
               uint8_t br_flag,
               uint8_t ce_level
	       );
void ocp_tx_rf(RU_t *ru, L1_rxtx_proc_t *proc);

// mistakes in main OAI
void  phy_init_RU(RU_t *);
void fep_full(RU_t *ru, int subframe);
void feptx_prec(RU_t *ru,int frame,int subframe);
void feptx_ofdm(RU_t *ru, int frame, int subframe);
void oai_subframe_ind(uint16_t sfn, uint16_t sf);
void softmodem_printresources(int sig, telnet_printfunc_t pf);
extern uint16_t sf_ahead;
#endif
