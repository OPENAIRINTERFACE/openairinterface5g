#ifndef __SPLIT_HEADERS_H
#define __SPLIT_HEADERS_H

#define MTU 65536
#define UDP_TIMEOUT 100000L // in nano second
#define MAX_BLOCKS 16
#define blockAlign 32 //bytes

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
  int nbAnt
  int nbSamples;
} frequency_t;

int createListner (port);
int receiveSubFrame(int sock, uint64_t expectedTS, void* bufferZone,  int bufferSize);
int sendSubFrame(int sock, void* bufferZone, int nbBlocks);}
#endif
