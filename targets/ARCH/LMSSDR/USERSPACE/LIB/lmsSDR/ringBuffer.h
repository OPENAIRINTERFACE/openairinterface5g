#ifndef LMS_RING_BUFFER_H
#define LMS_RING_BUFFER_H

#include <mutex>
#include <atomic>
#include <vector>
#include <condition_variable>

template<class T>
class RingBuffer
{
public:
    struct BufferInfo
    {
        uint32_t size;
        uint32_t itemsFilled;
    };

    BufferInfo GetInfo()
    {
        unique_lock<mutex> lck(mLock);
        BufferInfo stats;
        stats.size = (uint32_t)mBuffer.size();
        stats.itemsFilled = mElementsFilled;
        return stats;
    }

	RingBuffer(uint32_t bufLength)
	{
        Reset(bufLength);
	}
	~RingBuffer(){};
	
    /** @brief inserts items to ring buffer
        @param buffer data source
        @param itemCount number of buffer items to insert
        @param timeout_ms timeout duration for operation
        @return number of items added
    */
	uint32_t push_back(const T* buffer, const uint32_t itemCount, const uint32_t timeout_ms)
	{	
        uint32_t addedItems = 0;
        while (addedItems < itemCount)
        {
            unique_lock<mutex> lck(mLock);
            while (mElementsFilled >= mBuffer.size()) //wait for free space to insert items
            {
                if (canWrite.wait_for(lck, std::chrono::milliseconds(timeout_ms)) == std::cv_status::timeout)
                    return addedItems; //dropped all items
            }

            uint32_t itemsToInsert = itemCount - addedItems;
            uint32_t itemsToEnd = (uint32_t)mBuffer.size() - mTail.load(); //might need to split memcpy into two operations
            if (itemsToInsert > itemsToEnd)
            {
                memcpy(&mBuffer[mTail], &buffer[addedItems], itemsToEnd*sizeof(T));
                memcpy(&mBuffer[0], &buffer[addedItems+itemsToEnd], (itemsToInsert - itemsToEnd)*sizeof(T));
            }
            else
                memcpy(&mBuffer[mTail], &buffer[addedItems], itemsToInsert*sizeof(T));
            mTail.store((mTail.load() + itemsToInsert) & (mBuffer.size() - 1));
            mElementsFilled += itemsToInsert;            
            lck.unlock();
            canRead.notify_one();
            addedItems += itemsToInsert;
        }
        return addedItems;
	}
	
    /** @brief Takes items out of ring buffer
        @param buffer data destination
        @param itemCount number of items to extract from ring buffer
        @param timeout_ms timeout duration for operation
        @return number of items returned
    */
	uint32_t pop_front(T* buffer, const uint32_t itemCount, const uint32_t timeout_ms)
	{
        assert(buffer != nullptr);
        uint32_t itemsTaken = 0;
        T* destBuffer = buffer;
        while (itemsTaken < itemCount)
        {
            unique_lock<mutex> lck(mLock);
            while (mElementsFilled == 0) //buffer might be empty, wait for items
            {
                if (canRead.wait_for(lck, std::chrono::milliseconds(timeout_ms)) == std::cv_status::timeout)
                    return itemsTaken;
            }

            unsigned int itemsToCopy = itemCount - itemsTaken;
            if (itemsToCopy > mElementsFilled)
                itemsToCopy = mElementsFilled;
            unsigned int itemsToEnd = (uint32_t)mBuffer.size() - mHead.load(); //migth need to split memcpy into two operations
            if (itemsToEnd < itemsToCopy)
            {
                memcpy(&destBuffer[itemsTaken], &mBuffer[mHead.load()], sizeof(T)*itemsToEnd);
                memcpy(&destBuffer[itemsTaken+itemsToEnd], &mBuffer[0], sizeof(T)*(itemsToCopy - itemsToEnd));
                mHead.store((itemsToCopy - itemsToEnd) & (mBuffer.size() - 1));
            }
            else
            {
                memcpy(&destBuffer[itemsTaken], &mBuffer[mHead.load()], sizeof(T)*itemsToCopy);
                int headVal = mHead.load();
                int valueToStore = (headVal + itemsToCopy) & (mBuffer.size() - 1);
                mHead.store(valueToStore);
                headVal = mHead.load();
            }
            mElementsFilled -= itemsToCopy;
            lck.unlock();
            canWrite.notify_one();
            itemsTaken += itemsToCopy;
        }
        return itemsTaken;
	}
	
	void Reset(uint32_t bufLength)
	{
        std::unique_lock<std::mutex> lck(mLock);        
        if (bufLength >= (uint32_t)(1 << 31))
            bufLength = (uint32_t)(1 << 31);
        for (int i = 0; i < 32; ++i)
            if ((1 << i) >= bufLength)
            {
                bufLength = (1 << i);
                break;
            }
        mBuffer.resize(bufLength);
		mHead.store(0);
		mTail.store(0);
		mElementsFilled = 0;
	}
	
protected:
	std::vector<T> mBuffer;
    std::atomic<uint32_t> mHead;
    std::atomic<uint32_t> mTail;
    std::mutex mLock;
	uint32_t mElementsFilled;
    std::condition_variable canWrite;
    std::condition_variable canRead;
};

#endif
