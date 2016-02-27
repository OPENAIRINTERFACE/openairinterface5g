#ifndef LMS_FIFO_BUFFER_H
#define LMS_FIFO_BUFFER_H

#include <mutex>
#include <atomic>
#include <vector>
#include <thread>
#include <condition_variable>
#include "dataTypes.h"

class LMS_SamplesFIFO
{
public:
    struct BufferInfo
    {
        uint32_t size;
        uint32_t itemsFilled;
    };

    BufferInfo GetInfo()
    {
        std::unique_lock<std::mutex> lck2(readLock);
        std::unique_lock<std::mutex> lck(writeLock);
        BufferInfo stats;
        stats.size = (uint32_t)mBuffer.size();
        stats.itemsFilled = mElementsFilled.load();
        return stats;
    }

	LMS_SamplesFIFO(uint32_t bufLength)
	{
        Reset(bufLength);
	}
	
	~LMS_SamplesFIFO(){};
	
    /** @brief inserts items to ring buffer
        @param buffer data source
        @param itemCount number of buffer items to insert
        @param timeout_ms timeout duration for operation
        @param overwrite enable to overwrite oldest items inside the buffer
        @return number of items added
    */
	uint32_t push_packet(SamplesPacket *buffer, const uint32_t itemCount, const uint32_t timeout_ms, const bool overwrite = true)
	{	
        uint32_t addedItems = 0;
        std::unique_lock<std::mutex> lck(writeLock);
        while (addedItems < itemCount)
        {            
            while (mElementsFilled.load() >= mBuffer.size()) //wait for free space to insert items
            {
                if (canWrite.wait_for(lck, std::chrono::milliseconds(timeout_ms)) == std::cv_status::timeout)
                    return addedItems; //dropped all items
            }

            uint32_t itemsToInsert = itemCount - addedItems;
            uint32_t itemsToEnd = (uint32_t)mBuffer.size() - mTail.load(); //might need to split memcpy into two operations
            if (itemsToInsert > itemsToEnd)
            {
                memcpy(&mBuffer[mTail], &buffer[addedItems], itemsToEnd*sizeof(SamplesPacket));
                memcpy(&mBuffer[0], &buffer[addedItems+itemsToEnd], (itemsToInsert - itemsToEnd)*sizeof(SamplesPacket));
            }
            else
                memcpy(&mBuffer[mTail], &buffer[addedItems], itemsToInsert*sizeof(SamplesPacket));
            mTail.store((mTail.load() + itemsToInsert) & (mBuffer.size() - 1));
            mElementsFilled.fetch_add(itemsToInsert);
            canRead.notify_one();
            addedItems += itemsToInsert;
        }
        return addedItems;
	}

    /** @brief inserts items to ring buffer
    @param buffer data source
    @param itemCount number of buffer items to insert
    @param timeout_ms timeout duration for operation
    @param overwrite enable to overwrite oldest items inside the buffer
    @return number of items added
    */
    uint32_t push_samples(const complex16_t *buffer, const uint32_t samplesCount, uint64_t timestamp, const uint32_t timeout_ms, const bool overwrite = true)
    {
        assert(buffer != nullptr);
        const int samplesInPacket = SamplesPacket::samplesCount;
        uint32_t samplesTaken = 0;
        std::unique_lock<std::mutex> lck(writeLock);
        while (samplesTaken < samplesCount)
        {   
            while (mElementsFilled.load() >= mBuffer.size()) //buffer might be full, wait for free slots
            {
                if (canWrite.wait_for(lck, std::chrono::milliseconds(timeout_ms)) == std::cv_status::timeout)
                    return samplesTaken;
            }

            int tailIndex = mTail.load(); //which element to fill
            while (mElementsFilled.load() < mBuffer.size() && samplesTaken < samplesCount) // not to release lock too often
            {
                mBuffer[tailIndex].timestamp = timestamp + samplesTaken;
                mBuffer[tailIndex].first = 0;
                mBuffer[tailIndex].last = 0;
                while (mBuffer[tailIndex].last < samplesInPacket && samplesTaken < samplesCount)
                {
                    mBuffer[tailIndex].samples[mBuffer[tailIndex].last++] = buffer[samplesTaken++];
                }
                mTail.store((tailIndex + 1) & (mBuffer.size() - 1));//advance to next one
                tailIndex = mTail.load();
                mElementsFilled.fetch_add(1);
                canRead.notify_one();
            }
        }
        return samplesTaken;
    }
	
    /** @brief Takes items out of ring buffer
        @param buffer data destination
        @param samplesCount number of samples to pop
		@param timestamp returns timestamp of the first sample in buffer
        @param timeout_ms timeout duration for operation
        @return number of samples returned
    */
    uint32_t pop_samples(complex16_t* buffer, const uint32_t samplesCount, uint64_t *timestamp, const uint32_t timeout_ms)
	{
        assert(buffer != nullptr);
        const int samplesInPacket = SamplesPacket::samplesCount;
        uint32_t samplesFilled = 0;		
		*timestamp = 0;
        std::unique_lock<std::mutex> lck(readLock);
        while (samplesFilled < samplesCount)
        {   
            while (mElementsFilled.load() == 0) //buffer might be empty, wait for packets
            {
                if (canRead.wait_for(lck, std::chrono::milliseconds(timeout_ms)) == std::cv_status::timeout)
                    return samplesFilled;
            }
			if(samplesFilled == 0)
                *timestamp = mBuffer[mHead.load()].timestamp + mBuffer[mHead.load()].first;
			
			while(mElementsFilled.load() > 0 && samplesFilled < samplesCount)
			{	
				int headIndex = mHead.load();
                while (mBuffer[headIndex].first < mBuffer[headIndex].last && samplesFilled < samplesCount)
				{
					buffer[samplesFilled++] = mBuffer[headIndex].samples[mBuffer[headIndex].first++];
				}
                if (mBuffer[headIndex].first == mBuffer[headIndex].last) //packet depleated
				{
                    mBuffer[headIndex].first = 0;
                    mBuffer[headIndex].last = 0;
                    mBuffer[headIndex].timestamp = 0;
					mHead.store( (headIndex + 1) & (mBuffer.size() - 1) );//advance to next one
                    headIndex = mHead.load();
					mElementsFilled.fetch_sub(1);
                    canWrite.notify_one();
				}
			}
        }
        return samplesFilled;
	}
	
	void Reset(uint32_t bufLength)
	{
        std::unique_lock<std::mutex> lck(writeLock);
        std::unique_lock<std::mutex> lck2(readLock);
		mBuffer.resize(bufLength);
		mHead.store(0);
		mTail.store(0);
        mElementsFilled.store(0);
	}
	
protected:
	std::vector<SamplesPacket> mBuffer;
    std::atomic<uint32_t> mHead;
    std::atomic<uint32_t> mTail;
    std::mutex writeLock;
    std::mutex readLock;
	std::atomic<uint32_t> mElementsFilled;
    std::condition_variable canWrite;
    std::condition_variable canRead;
};

#endif
