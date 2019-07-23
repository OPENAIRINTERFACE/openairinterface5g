#include <executables/split_headers.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netdb.h>

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
  AssertFatal(setsockopt(result->sockHandler, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) ==0,"");
  // Make a send/recv buffer larger than a a couple of subframe
  // so the kernel will store for us in and out paquets
  int buff=1000*1000*10;
  AssertFatal ( setsockopt(result->sockHandler, SOL_SOCKET, SO_SNDBUF, &buff, sizeof(buff)) == 0, "");
  AssertFatal ( setsockopt(result->sockHandler, SOL_SOCKET, SO_RCVBUF, &buff, sizeof(buff)) == 0, "");
  return true;
}

// sock: udp socket
// expectedTS: the expected timestamp, 0 if unknown
// bufferZone: a reception area of bufferSize
int receiveSubFrame(UDPsock_t *sock, uint64_t expectedTS, void *bufferZone,  int bufferSize) {
  int rcved=0;
  commonUDP_t *tmp=NULL;

  do {
    //read all subframe data from the control unit
    int ret=recv(sock->sockHandler, bufferZone, bufferSize, 0);

    if ( ret==-1) {
      if ( errno == EWOULDBLOCK || errno== EINTR ) {
        return  rcved; // Timeout, subframe incomplete
      } else {
        LOG_E(HW,"Critical issue in socket: %s\n", strerror(errno));
        return -1;
      }
    } else {
      tmp=(commonUDP_t *)bufferZone;

      if ( expectedTS && tmp->timestamp != expectedTS) {
        LOG_W(HW,"Received a paquet in mixed subframes, dropping it\n");
      } else {
        rcved++;
        bufferZone+=ret;
      }
    }
  } while ( !rcved || rcved < tmp->nbBlocks );

  return rcved;
}

int sendSubFrame(UDPsock_t *sock, void *bufferZone, ssize_t secondHeaderSize) {
  commonUDP_t *UDPheader=(commonUDP_t *)bufferZone ;
  int nbBlocks=UDPheader->nbBlocks;
  int blockId=0;

  do {
    if (blockId > 0 ) {
      commonUDP_t *currentHeader=(commonUDP_t *)bufferZone;
      *currentHeader=*UDPheader;
      currentHeader->blockID=blockId;
      memcpy(commonUDPdata((void *)currentHeader), commonUDPdata(bufferZone), secondHeaderSize);
      blockId++;
    }

    int sz=alignedSize(bufferZone);
    // Let's use the first address returned by getaddrinfo()
    int ret=sendto(sock->sockHandler, bufferZone, sz, 0,
                   sock->destAddr->ai_addr, sock->destAddr->ai_addrlen);

    if ( ret != sz )
      LOG_W(HW,"Wrote socket doesn't return size %d (val: %d, errno:%d, %s)\n",
            sz, ret, errno, strerror(errno));

    bufferZone+=sz;
    nbBlocks--;
  } while (nbBlocks);

  return 0;
}
