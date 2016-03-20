/**
@author Lime Microsystems
@brief  Stream board communications for Matlab
*/

#include "LMS_SDR.h"

#include "lmsComms.h"
#include "LMS_StreamBoard.h"
#include "ringBuffer.h"
#include "IConnection.h"
#include "fifo.h"
#include "dataTypes.h"
#include <unistd.h>
#include <thread>

#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;

typedef enum
{
    LMS_SUCCESS = 0,
    LMS_ERROR
} LMS_STATUS;

LMS_SamplesFIFO rxBuffer(1);
static thread rxThread;
atomic<bool> rxStop(1);
atomic<bool> rxRunning(0);
atomic<unsigned long> rxDroppedSamples(0);
atomic<long> RxDataRate(0);
atomic<uint32_t> rxSamplingRate(0);

LMS_SamplesFIFO txBuffer(1);
static thread txThread;
atomic<bool> txStop(1);
atomic<bool> txRunning(0);
atomic<long> txDroppedSamples(0);
atomic<long> TxDataRate(0);
atomic<uint32_t> txSamplingRate(0);

LMScomms comPort(IConnection::COM_PORT);
LMScomms usbPort(IConnection::USB_PORT);
LMS_StreamBoard streamer(&usbPort);

const int samplesInPacket = 1024;
unsigned int opMode = 0;

DLL_EXPORT void LMS_Stats(uint32_t *RxBufSize, uint32_t *RxBufFilled, uint32_t *RxSamplingRate, uint32_t *TxBufSize, uint32_t *TxBufFilled, uint32_t *TxSamplingRate)
{   
    LMS_SamplesFIFO::BufferInfo rxStats = rxBuffer.GetInfo();
    LMS_SamplesFIFO::BufferInfo txStats = txBuffer.GetInfo();
    if (RxBufSize)
        *RxBufSize = rxStats.size*samplesInPacket;
    if (RxBufFilled)
        *RxBufFilled = rxStats.itemsFilled*samplesInPacket;
    if (TxBufSize)
        *TxBufSize = txStats.size*samplesInPacket;
    if (TxBufFilled)
        *TxBufFilled = txStats.itemsFilled*samplesInPacket;
    if (TxSamplingRate)
        *TxSamplingRate = txSamplingRate.load();
    if (RxSamplingRate)
        *RxSamplingRate = rxSamplingRate.load();
}

DLL_EXPORT int LMS_Init(const int OperationMode, uint32_t trxBuffersLength)
{   
    opMode = OperationMode;
    unsigned int packetsNeeded = trxBuffersLength / samplesInPacket;
    if (trxBuffersLength % samplesInPacket != 0)
        ++packetsNeeded;
    if (packetsNeeded >= (uint32_t)(1 << 31))
        packetsNeeded = (uint32_t)(1 << 31);
    for (int i = 0; i < 32; ++i)
        if ((1 << i) >= packetsNeeded)
        {
            packetsNeeded = (1 << i);
            break;
        }
    rxBuffer.Reset(packetsNeeded);
    txBuffer.Reset(packetsNeeded);
    return LMS_SUCCESS;
}

DLL_EXPORT int LMS_Destroy()
{
    LMS_RxStop();
    //buffers size is reduced to lower memory consumption when not used
    rxBuffer.Reset(1);
    txBuffer.Reset(1);
    return LMS_SUCCESS;
}

DLL_EXPORT LMScomms* LMS_GetUSBPort()
{
	return &usbPort;
}

DLL_EXPORT LMScomms* LMS_GetCOMPort()
{
	return &comPort;
}

const int maxDevListLen = 32;
const int maxDevNameLen = 256; //each device name not longer than 256
static char DeviceNames[maxDevListLen][maxDevNameLen];

DLL_EXPORT int LMS_UpdateDeviceList(LMScomms* port)
{   
    memset(DeviceNames, 0, maxDevListLen*maxDevNameLen);
    port->Close();
    int devCount = port->RefreshDeviceList();
    vector<string> names = port->GetDeviceList();
    int charsWritten = 0;
    for (unsigned int i = 0; i < names.size() && i < maxDevListLen; ++i)
    {   
        charsWritten += sprintf(DeviceNames[i], "[%i] %.*s", i, maxDevNameLen, names[i].c_str());
    }
    return devCount;
}

DLL_EXPORT const char* LMS_GetDeviceName(LMScomms* port, unsigned int deviceIndex)
{   
    if (deviceIndex < maxDevListLen)
        return DeviceNames[deviceIndex];
    else
        return "";
}

DLL_EXPORT int LMS_DeviceOpen(LMScomms* port, const uint32_t deviceIndex)
{
    port->Close();
    int status = port->Open(deviceIndex);
    if (status == 1)
    {
        return LMS_SUCCESS;
    }
    else
    {
        return LMS_ERROR;
    }    
}

DLL_EXPORT void LMS_DeviceClose(LMScomms* port)
{
    port->Close();
}

DLL_EXPORT uint32_t LMS_ControlWrite(LMScomms* port, const uint8_t *buffer, const uint16_t bufLen)
{
    return port->Write(buffer, bufLen, 0);
}

DLL_EXPORT uint32_t LMS_ControlRead(LMScomms* port, uint8_t* buffer, const uint16_t bufLen)
{
    return port->Read(buffer, bufLen, 0);
}



const int RX_BUFF_SZ (1020*1024/4);
uint32_t rx_buffer[RX_BUFF_SZ];
uint64_t start_timestamp = 0;;
uint32_t wr_pos = 0;
uint32_t rd_pos = 0;

void ReceivePackets()
{
    rxRunning.store(true);
    uint32_t samplesCollected = 0;

    const int bufferSize = 4096;// 4096;
    const int buffersCount = 32; // must be power of 2
    const int buffersCountMask = buffersCount - 1;
    int handles[buffersCount]= {0}; 
    char buffers[buffersCount*bufferSize]={0};
    
    struct sched_param sp;
    sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (pthread_setschedparam(pthread_self(),SCHED_FIFO,&sp)!=0)
    {
     printf("Shed prams failed\n");  
    }

    //switch off Rx
    uint16_t regVal = streamer.SPI_read(0x0005);
    streamer.SPI_write(0x0005, regVal & ~0x6);
    //USB FIFO reset
    LMScomms::GenericPacket ctrPkt;
    ctrPkt.cmd = CMD_USB_FIFO_RST;
    ctrPkt.outBuffer.push_back(0x01);
    usbPort.TransferPacket(ctrPkt);
    ctrPkt.outBuffer[0] = 0x00;
    usbPort.TransferPacket(ctrPkt);

    streamer.SPI_write(0x0005, regVal | 0x6);  
    
    streamer.SPI_write(0x0001, 0x0001);
    streamer.SPI_write(0x0007, 0x0000);
    
    for (int i = 0; i<buffersCount; ++i)
        handles[i] = usbPort.BeginDataReading(&buffers[i*bufferSize], bufferSize);

    int bi = 0;

    while (rxStop.load() != true)
    {
        if (usbPort.WaitForReading(handles[bi], 1000) == false)
            break;

        long bytesToRead = bufferSize;
        long bytesReceived = usbPort.FinishDataReading(&buffers[bi*bufferSize], bytesToRead, handles[bi]);
        if (bytesReceived > 0)
        {	
            PacketLTE* pkt = (PacketLTE*)&buffers[bi*bufferSize];
            for(uint16_t sampleIndex = 0; sampleIndex < sizeof(pkt->samples)/sizeof(int16_t); sampleIndex++)
            {
                    pkt->samples[sampleIndex] <<= 4;
                    pkt->samples[sampleIndex] >>= 4;		
            }

            rxBuffer.push_samples((complex16_t*)pkt->samples, 1020, pkt->counter, 100);                  
        }
        // Re-submit this request to keep the queue full
        memset(&buffers[bi*bufferSize], 0, bufferSize);
        handles[bi] = usbPort.BeginDataReading(&buffers[bi*bufferSize], bufferSize);
        bi = (bi + 1) & buffersCountMask;
        pthread_yield();
    }
    
    usbPort.AbortReading();
    for (int j = 0; j<buffersCount; j++)
    {
        long bytesToRead = bufferSize;
        usbPort.WaitForReading(handles[j], 1000);
        usbPort.FinishDataReading(&buffers[j*bufferSize], bytesToRead, handles[j]);
    }
    rxRunning.store(false);
}


DLL_EXPORT uint32_t LMS_TRxWrite(const int16_t *data, const uint32_t samplesCount, const uint32_t antenna_id, uint64_t timestamp)
{
    static uint32_t tx_buffer[1020];
    static int index = 0;
  
    const int bufferSize = 1024 * 4;
    const int buffersCount = 32; // must be power of 2
    const int buffersCountMask = buffersCount - 1;

    static int handles[buffersCount] = {0};
    static char buffers[buffersCount*bufferSize]={0};
    static bool bufferUsed[buffersCount] = {0};    
    static int bi = 0; //buffer index
    PacketLTE* pkt;

    uint64_t ts = timestamp - index;

    pkt = (PacketLTE*)&buffers[bi*bufferSize];

    for (int i=0;i<samplesCount;i++)
    {
      ((uint32_t*)pkt->samples)[index++]=(((uint32_t*)data)[i]& 0xFFF0FFF) | 0x1000;

      if (index == 1020)
      {    
       pkt->counter = ts;

       if (bufferUsed[bi])
       {
              if (usbPort.WaitForSending(handles[bi], 1000) == false)
                  return -1;
              // Must always call FinishDataXfer to release memory of contexts[i]
              long tempToSend = sizeof(PacketLTE);
              usbPort.FinishDataSending(&buffers[bi*bufferSize], tempToSend, handles[bi]);

              bufferUsed[bi] = false;
         }
        handles[bi] = usbPort.BeginDataSending(&buffers[bi*bufferSize], sizeof(PacketLTE));
        bufferUsed[bi] = true;
        bi = (bi + 1) & buffersCountMask;  
        pkt = (PacketLTE*)&buffers[bi*bufferSize];
        ts += 1020;
        index = 0;       
      }
    } 
}

vector<PacketLTE> PacketsBuffer(2048, PacketLTE());
DLL_EXPORT uint32_t LMS_TRxRead(int16_t *buffer, const uint32_t samplesCount, const uint32_t antenna_id, uint64_t *timestamp, const uint32_t timeout_ms)
{
	if (usbPort.IsOpen() == false && opMode != -1)
        return 0;
    pthread_yield();
    uint32_t samplesPopped = rxBuffer.pop_samples((complex16_t*)buffer, samplesCount, timestamp, timeout_ms);
    return samplesPopped;
}


DLL_EXPORT int LMS_RxStart()
{
    if (rxRunning.load())
        return 1;
    if (usbPort.IsOpen() == false && opMode != -1)
        return 1;
    rxStop.store(false);
    unsigned int bufSz = rxBuffer.GetInfo().size;
    rxBuffer.Reset(bufSz);
    rxThread = thread(ReceivePackets);
    return 0;
}

DLL_EXPORT int LMS_RxStop()
{
    if (rxRunning.load())
    {        
        if (rxThread.joinable())
        {
            rxStop.store(true);
            rxThread.join();
        }
        rxStop.store(true);
    }
    return 0;
}
