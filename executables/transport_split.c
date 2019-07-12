#include <executables/split_headers.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>

int createListner (int port) {
  int sock;
  AssertFatal((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) >= 0, "");
  struct sockaddr_in addr = {
sin_family:
    AF_INET,
sin_port:
    htons(port),
sin_addr:
    { s_addr: INADDR_ANY }
  };
  int enable=1;
  AssertFatal(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))==0,"");
  AssertFatal(bind(sock, (const struct sockaddr *) &addr, sizeof(addr))==0,"");
  struct timeval tv= {0,UDP_TIMEOUT};
  AssertFatal(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) ==0,"");
  // Make a send/recv buffer larger than a a couple of subframe
  // so the kernel will store for us in and out paquets
  int buff=1000*1000*10;
  AssertFatal ( setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &buff, sizeof(buff)) == 0, "");
  AssertFatal ( setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buff, sizeof(buff)) == 0, "");
}

// sock: udp socket
// expectedTS: the expected timestamp, 0 if unknown
// bufferZone: a reception area of bufferSize
int receiveSubFrame(int sock, uint64_t expectedTS, void *bufferZone,  int bufferSize) {
  int rcved=0;
  commonUDP_t *tmp=NULL;

  do {
    //read all subframe data from the control unit
    int ret=recv(sock, bufferZone, bufferSize, 0);

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
  } while ( !rcved || rcved < tmp->nbBlocks);

  return rcved;
}

int sendSubFrame(int sock, void *bufferZone, int nbBlocks) {
  do {
    int sz=alignedSize(bufferZone);
    int ret=send(sock, bufferZone, sz, 0);

    if ( ret != sz )
      LOG_W(HW,"Wrote socket doesn't return size %d (val: %d, errno:%d)\n",
            sz, ret, errno);

    bufferZone+=sz;
    nbBlocks--;
  } while (nbBlocks);

  return 0;
}
