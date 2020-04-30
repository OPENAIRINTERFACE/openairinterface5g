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



#include <executables/split_headers.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netdb.h>
#include <targets/RT/USER/lte-softmodem.h>

bool createUDPsock (char *sourceIP, char *sourcePort, char *destIP, char *destPort, UDPsock_t *result) {
  struct addrinfo hints= {0}, *servinfo, *p;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  int status;

  if ((status = getaddrinfo(sourceIP, sourcePort, &hints, &servinfo)) != 0) {
    LOG_E(GTPU,"getaddrinfo error: %s\n", gai_strerror(status));
    return false;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((result->sockHandler = socket(p->ai_family, p->ai_socktype,
                                      p->ai_protocol)) == -1) {
      LOG_W(GTPU,"socket: %s\n", strerror(errno));
      continue;
    }

    if (bind(result->sockHandler, p->ai_addr, p->ai_addrlen) == -1) {
      close(result->sockHandler);
      LOG_W(GTPU,"bind: %s\n", strerror(errno));
      continue;
    }

    break; // if we get here, we must have connected successfully
  }

  if (p == NULL) {
    // looped off the end of the list with no successful bind
    LOG_E(GTPU,"failed to bind socket: %s %s \n",sourceIP,sourcePort);
    return false;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if ((status = getaddrinfo(destIP, destPort, &hints, &servinfo)) != 0) {
    LOG_E(GTPU,"getaddrinfo error: %s\n", gai_strerror(status));
    return false;
  }

  if (servinfo) {
    result->destAddr=servinfo;
  } else {
    LOG_E(PHY,"No valid UDP addr: %s:%s\n",destIP, destPort);
    return false;
  }

  int enable=1;
  AssertFatal(setsockopt(result->sockHandler, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))==0,"");
  struct timeval tv= {0,UDP_TIMEOUT};

  if (IS_SOFTMODEM_RFSIM)
    tv.tv_sec=2; //debug: wait 2 seconds for human understanding

  AssertFatal(setsockopt(result->sockHandler, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) ==0,"");
  // Make a send/recv buffer larger than a a couple of subframe
  // so the kernel will store for us in and out paquets
  int buff=1000*1000*10;
  AssertFatal ( setsockopt(result->sockHandler, SOL_SOCKET, SO_SNDBUF, &buff, sizeof(buff)) == 0, "");
  AssertFatal ( setsockopt(result->sockHandler, SOL_SOCKET, SO_RCVBUF, &buff, sizeof(buff)) == 0, "");
  return true;
}

// sock: udp socket
// bufferZone: a reception area of bufferSize
int receiveSubFrame(UDPsock_t *sock, void *bufferZone,  int bufferSize, uint16_t contentType) {
  int rcved=0;
  commonUDP_t *bufOrigin=(commonUDP_t *)bufferZone;
  static uint8_t crossData[65536];
  static int crossDataSize=0;

  if (crossDataSize) {
    LOG_D(HW,"copy a block received in previous subframe\n");
    memcpy(bufferZone, crossData, crossDataSize);
    rcved=1;
    bufferZone+=crossDataSize;
    crossDataSize=0;
  }

  do {
    //read all subframe data from the control unit
    int ret=recv(sock->sockHandler, bufferZone, bufferSize, 0);

    if ( ret==-1) {
      if ( errno == EWOULDBLOCK || errno== EINTR ) {
        LOG_I(HW,"Received: Timeout, subframe incomplete\n");
        return  rcved;
      } else {
        LOG_E(HW,"Critical issue in socket: %s\n", strerror(errno));
        return -1;
      }
    } else {
      if (hUDP(bufferZone)->contentType != contentType)
        abort();

      if (rcved && bufOrigin->timestamp != hUDP(bufferZone)->timestamp ) {
        if ( hUDP(bufferZone)->timestamp > bufOrigin->timestamp ) {
          LOG_W(HW,"Received data for TS: %lu before end of TS : %lu completion\n",
                hUDP(bufferZone)->timestamp,
                bufOrigin->timestamp);
          memcpy(crossData, bufferZone, ret );
          crossDataSize=ret;
          return rcved;
        } else {
          LOG_W(HW,"Dropping late packet\n");
          continue;
        }
      }

      rcved++;
      bufferZone+=ret;
    }

    LOG_D(HW,"Received: blocks: %d/%d, size %d, TS: %lu\n",
          rcved, bufOrigin->nbBlocks, ret, bufOrigin->timestamp);
  } while ( rcved == 0 || rcved < bufOrigin->nbBlocks );

  return rcved;
}

int sendSubFrame(UDPsock_t *sock, void *bufferZone, ssize_t secondHeaderSize, uint16_t contentType) {
  commonUDP_t *UDPheader=(commonUDP_t *)bufferZone ;
  UDPheader->contentType=contentType;
  UDPheader->senderClock=rdtsc();
  int nbBlocks=UDPheader->nbBlocks;
  int blockId=0;

  if (nbBlocks <= 0 ) {
    LOG_E(PHY,"FS6: can't send blocks: %d\n", nbBlocks);
    return 0;
  }

  do {
    if (blockId > 0 ) {
      commonUDP_t *currentHeader=(commonUDP_t *)bufferZone;
      currentHeader->timestamp=UDPheader->timestamp;
      currentHeader->nbBlocks=UDPheader->nbBlocks;
      currentHeader->blockID=blockId;
      currentHeader->contentType=UDPheader->contentType;
      memcpy(commonUDPdata((void *)currentHeader), commonUDPdata(bufferZone), secondHeaderSize);
    }

    blockId++;
    int sz=alignedSize(bufferZone);
    // Let's use the first address returned by getaddrinfo()
    int ret=sendto(sock->sockHandler, bufferZone, sz, 0,
                   sock->destAddr->ai_addr, sock->destAddr->ai_addrlen);

    if ( ret != sz )
      LOG_W(HW,"Wrote socket doesn't return size %d (val: %d, errno:%d, %s)\n",
            sz, ret, errno, strerror(errno));

    LOG_D(HW,"Sent: TS: %lu, blocks %d/%d, block size : %d \n",
          UDPheader->timestamp, UDPheader->nbBlocks-nbBlocks, UDPheader->nbBlocks, sz);
    bufferZone+=sz;
    nbBlocks--;
  } while (nbBlocks);

  return 0;
}
