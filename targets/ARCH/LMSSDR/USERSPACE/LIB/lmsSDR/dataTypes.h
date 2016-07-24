#ifndef LMS_DATA_TYPES_H
#define LMS_DATA_TYPES_H

typedef struct
{
    uint8_t reserved[8];
    uint64_t counter;
    int16_t samples[2040];
} PacketLTE;

typedef struct
{
    int16_t i;
    int16_t q;
} complex16_t;

class SamplesPacket
{
  public:
    uint64_t timestamp; //timestamp of the packet
    uint16_t first; //index of first unused sample in samples[]
    uint16_t last; //end index of samples
    static const uint16_t samplesCount = 1024; //maximum number of samples in packet
    complex16_t samples[samplesCount]; //must be power of two    
};

complex16_t operator &=(complex16_t & other1, const complex16_t & other) // copy assignment
{    
    other1.i = other.i;
    other1.q = other.q;
    return other1;
}

#endif
