/*
  Author: Laurent THOMAS, Open Cells for Nokia
  copyleft: OpenAirInterface Software Alliance and it's licence
*/

/* 
 * Open issues and limitations
 * The read and write should be called in the same thread, that is not new USRP UHD design
 * When the opposite side switch from passive reading to active R+Write, the synchro is not fully deterministic
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>

#include <common/utils/assertions.h>
#include <common/utils/LOG/log.h>
#include "common_lib.h"
#include <openair1/PHY/defs_eNB.h>
#include "openair1/PHY/defs_UE.h"

#define PORT 4043 //TCP port for this simulator
#define CirSize 3072000 // 100ms is enough
#define sample_t uint32_t // 2*16 bits complex number
#define sampleToByte(a,b) ((a)*(b)*sizeof(sample_t))
#define byteToSample(a,b) ((a)/(sizeof(sample_t)*(b)))

#define sample_t uint32_t //2*16 bits complex number

typedef struct buffer_s {
  int conn_sock;
  bool alreadyRead;
  uint64_t lastReceivedTS;
  bool headerMode;
  samplesBlockHeader_t th;
  char *transferPtr;
  uint64_t remainToTransfer;
  char *circularBufEnd;
  sample_t *circularBuf;
} buffer_t;

typedef struct {
  int listen_sock, epollfd;
  uint64_t nextTimestamp;
  uint64_t typeStamp;
  char *ip;
  int saveIQfile;
  buffer_t buf[FD_SETSIZE];
} rfsimulator_state_t;

void allocCirBuf(rfsimulator_state_t *bridge, int sock) {
  buffer_t *ptr=&bridge->buf[sock];
  AssertFatal ( (ptr->circularBuf=(sample_t *) malloc(sampleToByte(CirSize,1))) != NULL, "");
  ptr->circularBufEnd=((char *)ptr->circularBuf)+sampleToByte(CirSize,1);
  ptr->conn_sock=sock;
  ptr->alreadyRead=false;
  ptr->lastReceivedTS=0;
  ptr->headerMode=true;
  ptr->transferPtr=(char *)&ptr->th;
  ptr->remainToTransfer=sizeof(samplesBlockHeader_t);
  int sendbuff=1000*1000*10;
  AssertFatal ( setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff)) == 0, "");
  struct epoll_event ev= {0};
  ev.events = EPOLLIN | EPOLLRDHUP;
  ev.data.fd = sock;
  AssertFatal(epoll_ctl(bridge->epollfd, EPOLL_CTL_ADD,  sock, &ev) != -1, "");
}

void removeCirBuf(rfsimulator_state_t *bridge, int sock) {
  AssertFatal( epoll_ctl(bridge->epollfd, EPOLL_CTL_DEL,  sock, NULL) != -1, "");
  close(sock);
  free(bridge->buf[sock].circularBuf);
  memset(&bridge->buf[sock], 0, sizeof(buffer_t));
  bridge->buf[sock].conn_sock=-1;
}

void socketError(rfsimulator_state_t *bridge, int sock) {
  if (bridge->buf[sock].conn_sock!=-1) {
    LOG_W(HW,"Lost socket \n");
    removeCirBuf(bridge, sock);

    if (bridge->typeStamp==UE_MAGICDL_FDD)
      exit(1);
  }
}

#define helpTxt "\
\x1b[31m\
rfsimulator: error: you have to run one UE and one eNB\n\
For this, export RFSIMULATOR=enb (eNB case) or \n\
                 RFSIMULATOR=<an ip address> (UE case)\n\
\x1b[m"

enum  blocking_t {
  notBlocking,
  blocking
};

void setblocking(int sock, enum blocking_t active) {
  int opts;
  AssertFatal( (opts = fcntl(sock, F_GETFL)) >= 0,"");

  if (active==blocking)
    opts = opts & ~O_NONBLOCK;
  else
    opts = opts | O_NONBLOCK;

  AssertFatal(fcntl(sock, F_SETFL, opts) >= 0, "");
}

static bool flushInput(rfsimulator_state_t *t, int timeout);

void fullwrite(int fd, void *_buf, ssize_t count, rfsimulator_state_t *t) {
  if (t->saveIQfile != -1) {
    if (write(t->saveIQfile, _buf, count) != count )
      LOG_E(HW,"write in save iq file failed (%s)\n",strerror(errno));
  }

  AssertFatal(fd>=0 && _buf && count >0 && t,
	      "Bug: %d/%p/%zd/%p", fd, _buf, count, t);

  char *buf = _buf;
  ssize_t l;
  setblocking(fd, notBlocking);

  while (count) {
    l = write(fd, buf, count);

    if (l <= 0) {
      if (errno==EINTR)
        continue;

      if(errno==EAGAIN) {
	// The opposite side is saturated
	// we read incoming sockets meawhile waiting
        flushInput(t, 5);
        continue;
      } else
        return;
    }

    count -= l;
    buf += l;
  }
}

int server_start(openair0_device *device) {
  rfsimulator_state_t *t = (rfsimulator_state_t *) device->priv;
  t->typeStamp=ENB_MAGICDL_FDD;
  AssertFatal((t->listen_sock = socket(AF_INET, SOCK_STREAM, 0)) >= 0, "");
  int enable = 1;
  AssertFatal(setsockopt(t->listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == 0, "");
  struct sockaddr_in addr = {
sin_family:
    AF_INET,
sin_port:
    htons(PORT),
sin_addr:
    { s_addr: INADDR_ANY }
  };
  bind(t->listen_sock, (struct sockaddr *)&addr, sizeof(addr));
  AssertFatal(listen(t->listen_sock, 5) == 0, "");
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = t->listen_sock;
  AssertFatal(epoll_ctl(t->epollfd, EPOLL_CTL_ADD,  t->listen_sock, &ev) != -1, "");
  return 0;
}

int start_ue(openair0_device *device) {
  rfsimulator_state_t *t = device->priv;
  t->typeStamp=UE_MAGICDL_FDD;
  int sock;
  AssertFatal((sock = socket(AF_INET, SOCK_STREAM, 0)) >= 0, "");
  struct sockaddr_in addr = {
sin_family:
    AF_INET,
sin_port:
    htons(PORT),
sin_addr:
    { s_addr: INADDR_ANY }
  };
  addr.sin_addr.s_addr = inet_addr(t->ip);
  bool connected=false;

  while(!connected) {
    LOG_I(HW,"rfsimulator: trying to connect to %s:%d\n", t->ip, PORT);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
      LOG_I(HW,"rfsimulator: connection established\n");
      connected=true;
    }

    perror("rfsimulator");
    sleep(1);
  }

  setblocking(sock, notBlocking);
  allocCirBuf(t, sock);
  t->buf[sock].alreadyRead=true; // UE will start blocking on read
  return 0;
}

uint64_t lastW=-1;
int rfsimulator_write(openair0_device *device, openair0_timestamp timestamp, void **samplesVoid, int nsamps, int nbAnt, int flags) {
  rfsimulator_state_t *t = device->priv;
  LOG_D(HW,"sending %d samples at time: %ld\n", nsamps, timestamp);

  for (int i=0; i<FD_SETSIZE; i++) {
    buffer_t *ptr=&t->buf[i];

    if (ptr->conn_sock >= 0 ) {
      samplesBlockHeader_t header= {t->typeStamp, nsamps, nbAnt, timestamp};
      fullwrite(ptr->conn_sock,&header, sizeof(header), t);
      sample_t tmpSamples[nsamps][nbAnt];

      for(int a=0; a<nbAnt; a++) {
        sample_t *in=(sample_t *)samplesVoid[a];

        for(int s=0; s<nsamps; s++)
          tmpSamples[s][a]=in[s];
      }

      if (ptr->conn_sock >= 0 )
        fullwrite(ptr->conn_sock, (void *)tmpSamples, sampleToByte(nsamps,nbAnt), t);
    }
  }
  lastW=timestamp;
  LOG_D(HW,"sent %d samples at time: %ld->%ld, energy in first antenna: %d\n",
        nsamps, timestamp, timestamp+nsamps, signal_energy(samplesVoid[0], nsamps) );
  // Let's verify we don't have incoming data
  // This is mandatory when the opposite side don't transmit
  flushInput(t, 0);
  return nsamps;
}

static bool flushInput(rfsimulator_state_t *t, int timeout) {
  // Process all incoming events on sockets
  // store the data in lists
  struct epoll_event events[FD_SETSIZE]= {0};
  int nfds = epoll_wait(t->epollfd, events, FD_SETSIZE, timeout);

  if ( nfds==-1 ) {
    if ( errno==EINTR || errno==EAGAIN )
      return false;
    else
      AssertFatal(false,"error in epoll_wait\n");
  }

  for (int nbEv = 0; nbEv < nfds; ++nbEv) {
    int fd=events[nbEv].data.fd;

    if (events[nbEv].events & EPOLLIN && fd == t->listen_sock) {
      int conn_sock;
      AssertFatal( (conn_sock = accept(t->listen_sock,NULL,NULL)) != -1, "");
      setblocking(conn_sock, notBlocking);
      allocCirBuf(t, conn_sock);
      LOG_I(HW,"A ue connected\n");
    } else {
      if ( events[nbEv].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP) ) {
        socketError(t,fd);
        continue;
      }

      buffer_t *b=&t->buf[fd];

      if ( b->circularBuf == NULL ) {
        LOG_E(HW, "received data on not connected socket %d\n", events[nbEv].data.fd);
        continue;
      }

      ssize_t blockSz;

      if ( b->headerMode)
        blockSz=b->remainToTransfer;
      else
        blockSz= b->transferPtr+b->remainToTransfer < b->circularBufEnd ?
                 b->remainToTransfer :
                 b->circularBufEnd - 1 - b->transferPtr ;

      ssize_t sz=recv(fd, b->transferPtr, blockSz, MSG_DONTWAIT);

      if ( sz < 0 ) {
        if ( errno != EAGAIN ) {
          LOG_E(HW,"socket failed %s\n", strerror(errno));
          abort();
        }
      } else if ( sz == 0 )
        continue;

      LOG_D(HW, "Socket rcv %zd bytes\n", sz);
      AssertFatal((b->remainToTransfer-=sz) >= 0, "");
      b->transferPtr+=sz;

      if (b->transferPtr==b->circularBufEnd - 1)
        b->transferPtr=(char *)b->circularBuf;

      // check the header and start block transfer
      if ( b->headerMode==true && b->remainToTransfer==0) {
        AssertFatal( (t->typeStamp == UE_MAGICDL_FDD  && b->th.magic==ENB_MAGICDL_FDD) ||
                     (t->typeStamp == ENB_MAGICDL_FDD && b->th.magic==UE_MAGICDL_FDD), "Socket Error in protocol");
        b->headerMode=false;
        b->alreadyRead=true;

        if ( b->lastReceivedTS != b->th.timestamp) {
          int nbAnt= b->th.nbAnt;

          for (uint64_t index=b->lastReceivedTS; index < b->th.timestamp; index++ )
            for (int a=0; a < nbAnt; a++)
              b->circularBuf[(index*nbAnt+a)%CirSize]=0;

          LOG_W(HW,"gap of: %ld in reception\n", b->th.timestamp-b->lastReceivedTS );
        }

        b->lastReceivedTS=b->th.timestamp;
	AssertFatal(lastW == -1 || ( abs((double)lastW-b->lastReceivedTS) < (double)CirSize),
	  "Tx/Rx shift too large Tx:%lu, Rx:%lu\n", lastW, b->lastReceivedTS);
        b->transferPtr=(char *)&b->circularBuf[b->lastReceivedTS%CirSize];
        b->remainToTransfer=sampleToByte(b->th.size, b->th.nbAnt);
      }

      if ( b->headerMode==false ) {
	LOG_D(HW,"Set b->lastReceivedTS %ld\n", b->lastReceivedTS);
        b->lastReceivedTS=b->th.timestamp+b->th.size-byteToSample(b->remainToTransfer,b->th.nbAnt);
	// First block in UE, resync with the eNB current TS
	if ( t->nextTimestamp == 0 )
	  t->nextTimestamp=b->lastReceivedTS-b->th.size;
	
        if ( b->remainToTransfer==0) {
          LOG_D(HW,"Completed block reception: %ld\n", b->lastReceivedTS);
          b->headerMode=true;
          b->transferPtr=(char *)&b->th;
          b->remainToTransfer=sizeof(samplesBlockHeader_t);
          b->th.magic=-1;
        }
      }
    }
  }

  return nfds>0;
}

int rfsimulator_read(openair0_device *device, openair0_timestamp *ptimestamp, void **samplesVoid, int nsamps, int nbAnt) {
  if (nbAnt != 1) {
    LOG_E(HW, "rfsimulator: only 1 antenna tested\n");
    exit(1);
  }

  rfsimulator_state_t *t = device->priv;
  LOG_D(HW, "Enter rfsimulator_read, expect %d samples, will release at TS: %ld\n", nsamps, t->nextTimestamp+nsamps);
  // deliver data from received data
  // check if a UE is connected
  int first_sock;

  for (first_sock=0; first_sock<FD_SETSIZE; first_sock++)
    if (t->buf[first_sock].circularBuf != NULL )
      break;

  if ( first_sock ==  FD_SETSIZE ) {
    // no connected device (we are eNB, no UE is connected)
    if (!flushInput(t, 10)) {
      for (int x=0; x < nbAnt; x++)
        memset(samplesVoid[x],0,sampleToByte(nsamps,1));

      t->nextTimestamp+=nsamps;
      LOG_W(HW,"Generated void samples for Rx: %ld\n", t->nextTimestamp);
      *ptimestamp = t->nextTimestamp-nsamps;
      return nsamps;
    }
  } else {
    bool have_to_wait;

    do {
      have_to_wait=false;

      for ( int sock=0; sock<FD_SETSIZE; sock++) {
        if ( t->buf[sock].circularBuf && t->buf[sock].alreadyRead )
	  if ( t->buf[sock].lastReceivedTS == 0 ||
	       (t->nextTimestamp+nsamps) > t->buf[sock].lastReceivedTS ) {
	    have_to_wait=true;
	    break;
	  }
      }

      if (have_to_wait)
        /*printf("Waiting on socket, current last ts: %ld, expected at least : %ld\n",
          ptr->lastReceivedTS,
          t->nextTimestamp+nsamps);
        */
        flushInput(t, 3);
    } while (have_to_wait);
  }

  // Clear the output buffer
  for (int a=0; a<nbAnt; a++)
    memset(samplesVoid[a],0,sampleToByte(nsamps,1));

  // Add all input signal in the output buffer
  for (int sock=0; sock<FD_SETSIZE; sock++) {
    buffer_t *ptr=&t->buf[sock];

    if ( ptr->circularBuf && ptr->alreadyRead ) {
      for (int a=0; a<nbAnt; a++) {
        sample_t *out=(sample_t *)samplesVoid[a];

        for ( int i=0; i < nsamps; i++ )
          out[i]+=ptr->circularBuf[((t->nextTimestamp+i)*nbAnt+a)%CirSize]<<1;
      }
    }
  }

  *ptimestamp = t->nextTimestamp; // return the time of the first sample
  t->nextTimestamp+=nsamps;
  LOG_D(HW,"Rx to upper layer: %d from %ld to %ld, energy in first antenna %d\n",
        nsamps,
        *ptimestamp, t->nextTimestamp,
        signal_energy(samplesVoid[0], nsamps));
  return nsamps;
}


int rfsimulator_request(openair0_device *device, void *msg, ssize_t msg_len) {
  abort();
  return 0;
}
int rfsimulator_reply(openair0_device *device, void *msg, ssize_t msg_len) {
  abort();
  return 0;
}
int rfsimulator_get_stats(openair0_device *device) {
  return 0;
}
int rfsimulator_reset_stats(openair0_device *device) {
  return 0;
}
void rfsimulator_end(openair0_device *device) {}
int rfsimulator_stop(openair0_device *device) {
  return 0;
}
int rfsimulator_set_freq(openair0_device *device, openair0_config_t *openair0_cfg,int exmimo_dump_config) {
  return 0;
}
int rfsimulator_set_gains(openair0_device *device, openair0_config_t *openair0_cfg) {
  return 0;
}


__attribute__((__visibility__("default")))
int device_init(openair0_device *device, openair0_config_t *openair0_cfg) {
  //set_log(HW,OAILOG_DEBUG);
  //set_log(PHY,OAILOG_DEBUG);
  rfsimulator_state_t *rfsimulator = (rfsimulator_state_t *)calloc(sizeof(rfsimulator_state_t),1);

  if ((rfsimulator->ip=getenv("RFSIMULATOR")) == NULL ) {
    LOG_E(HW,helpTxt);
    exit(1);
  }

  if ( strncasecmp(rfsimulator->ip,"enb",3) == 0 ||
       strncasecmp(rfsimulator->ip,"server",3) == 0 )
    rfsimulator->typeStamp = ENB_MAGICDL_FDD;
  else
    rfsimulator->typeStamp = UE_MAGICDL_FDD;
  
  LOG_I(HW,"rfsimulator: running as %s\n", rfsimulator-> typeStamp == ENB_MAGICDL_FDD ? "(eg)NB" : "UE");
  char *saveF;

  if ((saveF=getenv("saveIQfile")) != NULL) {
    rfsimulator->saveIQfile=open(saveF,O_APPEND| O_CREAT|O_TRUNC | O_WRONLY, 0666);

    if ( rfsimulator->saveIQfile != -1 )
      LOG_I(HW,"rfsimulator: will save written IQ samples  in %s\n", saveF);
    else
      LOG_E(HW, "can't open %s for IQ saving (%s)\n", saveF, strerror(errno));
  } else
    rfsimulator->saveIQfile = -1;

  device->trx_start_func       = rfsimulator->typeStamp == ENB_MAGICDL_FDD ?
                                 server_start :
                                 start_ue;
  device->trx_get_stats_func   = rfsimulator_get_stats;
  device->trx_reset_stats_func = rfsimulator_reset_stats;
  device->trx_end_func         = rfsimulator_end;
  device->trx_stop_func        = rfsimulator_stop;
  device->trx_set_freq_func    = rfsimulator_set_freq;
  device->trx_set_gains_func   = rfsimulator_set_gains;
  device->trx_write_func       = rfsimulator_write;
  device->trx_read_func      = rfsimulator_read;
  /* let's pretend to be a b2x0 */
  device->type = USRP_B200_DEV;
  device->openair0_cfg=&openair0_cfg[0];
  device->priv = rfsimulator;

  for (int i=0; i<FD_SETSIZE; i++)
    rfsimulator->buf[i].conn_sock=-1;

  AssertFatal((rfsimulator->epollfd = epoll_create1(0)) != -1,"");
  return 0;
}
