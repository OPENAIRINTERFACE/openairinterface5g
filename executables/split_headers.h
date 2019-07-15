#ifndef __SPLIT_HEADERS_H
#define __SPLIT_HEADERS_H

#include <stdint.h>
#include <stdbool.h>
#include <openair1/PHY/defs_eNB.h>
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

bool createUDPsock (char *sourceIP, char *sourcePort, char *destIP, char *destPort, UDPsock_t *result);
int receiveSubFrame(UDPsock_t *sock, uint64_t expectedTS, void *bufferZone,  int bufferSize);
int sendSubFrame(UDPsock_t *sock, void *bufferZone);
inline size_t alignedSize(void *ptr) {
  commonUDP_t *header=(commonUDP_t *) ptr;
  return ((header->contentBytes+sizeof(commonUDP_t)+blockAlign-1)/blockAlign)*blockAlign;
}

void *cu_fs6(void *arg);
void *du_fs6(void *arg);
void fill_rf_config(RU_t *ru, char *rf_config_file);
void rx_rf(RU_t *ru,int *frame,int *subframe);
void tx_rf(RU_t *ru);
void common_signal_procedures (PHY_VARS_eNB *eNB,int frame, int subframe);
void pmch_procedures(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc);

// mistakes in main OAI
void  phy_init_RU(RU_t *);
void feptx_prec(RU_t *);
void feptx_ofdm(RU_t *);
void oai_subframe_ind(uint16_t sfn, uint16_t sf);
#endif
