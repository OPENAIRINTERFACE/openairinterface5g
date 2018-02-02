/*
 * Copyright 2017 Cisco Systems, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _POOL_H_
#define _POOL_H_

#include <mutex>
#include <map>
#include <queue>
#include <cassert>


class pool_config
{
    public:
        
        size_t size;
        uint16_t initial_count;
};

class memory_pool
{
    std::mutex mutex;
	std::map<size_t, std::queue<uint8_t*>> free_store;
	std::map<size_t, uint16_t> allocated_store;
	
    void init(std::vector<pool_config> config)
    {
        printf("memory_pool::init\n");
        		
        for(pool_config c : config)
        {
            // todo : use the initial count to allocate initial pool memory
            free_store.insert(std::pair<size_t, 
                                        std::queue<uint8_t*>>
                                            (c.size, std::queue<uint8_t*>()));
			allocated_store.insert(std::pair<size_t, uint16_t>(c.size, 0));
        }
    }
    
    memory_pool()
    {
        std::vector<pool_config> config 
        {
            {8, 0}, {32, 0}, {64, 0}, {128, 0}, {256, 0}, 
            {1024, 0}, {4096, 0}, {8912, 0}
            
        };
        
        init(config);
    }
    
    
    uint16_t determine_pool_size(size_t size)
    {
        for(auto pool : free_store)
        {
            if(pool.first >= size)
                return pool.first;
        }
        
        return 0;
    }
    
    uint8_t* alloc(size_t size)
    {
        if(size == 0)
            return 0;
            
        uint8_t* ptr = 0;
        size_t psize = determine_pool_size(size + 4);
        
        if(psize == 0)
        {
            ptr =  (uint8_t*)malloc(size + 4);
			memset(ptr, 0, size + 4);	
        }
        else
        {
            mutex.lock();
            
            auto it = free_store.find(psize);
            
            if(it == free_store.end())
            {
                // Unknown pool size
   				ptr =  (uint8_t*)malloc(psize + 4);
				memcpy(ptr, &psize, 4);
            }
            else
            {
                if(it->second.size() > 0)
                {
                    // Take from the pool
                    ptr = (uint8_t*)(it->second.front());
    				it->second.pop();
                }
                else
                {
                    // No memory left in pool
                    ptr =  (uint8_t*)malloc(psize + 4);
				    memcpy(ptr, &psize, 4);
                }
            }
            
            mutex.unlock();
        }

        return (ptr + 4);
    }
    
    void dealloc(uint8_t* ptr)
    {
      	if(ptr == 0)
			return;
			
		uint8_t* _ptr = (uint8_t*)ptr;

        size_t psize;
		memcpy(&psize, _ptr - 4, 4);
		
		if(psize == 0)
		{
		    free(_ptr - 4);
		}
		else
		{
		    mutex.lock();
		    
		    auto it = free_store.find(psize);
			if(it == free_store.end())
			{
			    free(_ptr - 4);
			}
			else
			{
			    assert((_ptr - 4) != 0);    
			    it->second.push(_ptr - 4);
			}
		    
		    mutex.unlock();
		}
  
    }
    
    public:
    
    memory_pool(memory_pool const&) = delete;
    void operator=(memory_pool const&) = delete;
    
    /*! Get the memory_pool instance */
    static memory_pool& instance()
    {
        static memory_pool instance;
        return instance;
    }    

    /*! Allocate a buffer from the memory_pool is at least size */
    static uint8_t* allocate(size_t size)
    {
        return instance().alloc(size);
    }
    
    /*! Return the buffer to the memory_pool */
    static void deallocate(uint8_t* ptr)
    {
        return instance().dealloc(ptr);
    }
        
    
};

#endif // _POOL_H_
