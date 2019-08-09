#ifndef __SPLIT_HEADERS_H
#define __SPLIT_HEADERS_H

#include <stdint.h>
#include <stdbool.h>
#include <openair1/PHY/defs_eNB.h>

#define CU_IP "127.0.0.1"
#define CU_PORT "7878"
#define DU_IP "127.0.0.1"
#define DU_PORT "8787"

#define MTU 65536
#define UDP_TIMEOUT 100000L // in nano second
#define MAX_BLOCKS 16
#define blockAlign 32 //bytes


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

typedef struct commonUDP_s {
  uint64_t timestamp; // id of the group (subframe for LTE)
  uint16_t nbBlocks;       // total number of blocks for this timestamp
  uint16_t blockID;        // id: 0..nbBocks-1
  uint16_t contentType;    // defines the content format
  uint16_t contentBytes;   // will be sent in a UDP packet, so must be < 2^16 bytes
} commonUDP_t;

typedef struct frequency_s {
  int frame;
  int subframe;
  int sampleSize;
  int nbAnt;
  int nbSamples;
} frequency_t;

typedef struct {
  uint16_t max_preamble[4];
  uint16_t max_preamble_energy[4];
  uint16_t max_preamble_delay[4];
  uint16_t avg_preamble_energy[4];
} fs6_ul_t;

typedef struct {
  int num_pdcch_symbols;
  int num_dci;
  DCI_ALLOC_t dci_alloc[32];
  int num_mdci;
  int amp;
  int8_t UE_ul_active[NUMBER_OF_UE_MAX];
  int8_t UE_ul_first_rb[NUMBER_OF_UE_MAX]; //
  int8_t UE_ul_last_rb[NUMBER_OF_UE_MAX]; //
  LTE_eNB_PHICH phich_vars;
} fs6_dl_t;

typedef struct {
  int UE_id;
  int8_t harq_pid;
  uint16_t rnti;
  int dataLen;
} fs6_dl_uespec_t;

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

static inline size_t alignedSize(uint8_t *ptr) {
  commonUDP_t *header=(commonUDP_t *) ptr;
  return ((header->contentBytes+sizeof(commonUDP_t)+blockAlign-1)/blockAlign)*blockAlign;
}

static inline void *commonUDPdata(uint8_t *ptr) {
  return (void *) (((commonUDP_t *)ptr)+1);
}

void *cu_fs6(void *arg);
void *du_fs6(void *arg);
void fill_rf_config(RU_t *ru, char *rf_config_file);
void rx_rf(RU_t *ru,int *frame,int *subframe);
void tx_rf(RU_t *ru);
void common_signal_procedures (PHY_VARS_eNB *eNB,int frame, int subframe);
void pmch_procedures(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc);
bool dlsch_procedures(PHY_VARS_eNB *eNB,
                      L1_rxtx_proc_t *proc,
                      int harq_pid,
                      LTE_eNB_DLSCH_t *dlsch,
                      LTE_eNB_UE_stats *ue_stats) ;
void pdsch_procedures(PHY_VARS_eNB *eNB,
                      L1_rxtx_proc_t *proc,
                      int harq_pid,
                      LTE_eNB_DLSCH_t *dlsch,
                      LTE_eNB_DLSCH_t *dlsch1);
void srs_procedures(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc);
void uci_procedures(PHY_VARS_eNB *eNB,
                    L1_rxtx_proc_t *proc);

// mistakes in main OAI
void  phy_init_RU(RU_t *);
void feptx_prec(RU_t *);
void feptx_ofdm(RU_t *);
void oai_subframe_ind(uint16_t sfn, uint16_t sf);
extern uint16_t sf_ahead;
#endif
