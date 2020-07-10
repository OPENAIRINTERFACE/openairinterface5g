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
#include <common/utils/load_module_shlib.h>
#include <common/config/config_userapi.h>
#include "common_lib.h"
#include <openair1/PHY/defs_eNB.h>
#include "openair1/PHY/defs_UE.h"
#define CHANNELMOD_DYNAMICLOAD
#include <openair1/SIMULATION/TOOLS/sim.h>

#define PORT 4043 //default TCP port for this simulator
#define CirSize 3072000 // 100ms is enough
#define sampleToByte(a,b) ((a)*(b)*sizeof(sample_t))
#define byteToSample(a,b) ((a)/(sizeof(sample_t)*(b)))

#define MAX_SIMULATION_CONNECTED_NODES 5
#define GENERATE_CHANNEL 10 //each frame in DL

// Fixme: datamodel, external variables in .h files, ...
#include <common/ran_context.h>

extern RAN_CONTEXT_t RC;
//

#define RFSIMU_SECTION    "rfsimulator"
#define RFSIMU_OPTIONS_PARAMNAME "options"
# define RFSIM_CONFIG_HELP_OPTIONS     " list of comma separated options to enable rf simulator functionalities. Available options: \n"\
  "        chanmod:   enable channel modelisation\n"\
  "        saviq:     enable saving written iqs to a file\n"
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            configuration parameters for the rfsimulator device                                                                              */
/*   optname                     helpstr                     paramflags           XXXptr                               defXXXval                          type         numelt  */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define RFSIMULATOR_PARAMS_DESC {\
    {"serveraddr",             "<ip address to connect to>\n",          0,         strptr:&(rfsimulator->ip),              defstrval:"127.0.0.1",           TYPE_STRING,    0 },\
    {"serverport",             "<port to connect to>\n",                0,         u16ptr:&(rfsimulator->port),            defuintval:PORT,                 TYPE_UINT16,    0 },\
    {RFSIMU_OPTIONS_PARAMNAME, RFSIM_CONFIG_HELP_OPTIONS,               0,         strlistptr:NULL,                        defstrlistval:NULL,              TYPE_STRINGLIST,0},\
    {"IQfile",                 "<file path to use when saving IQs>\n",  0,         strptr:&(saveF),                        defstrval:"/tmp/rfsimulator.iqs",TYPE_STRING,    0 },\
    {"modelname",              "<channel model name>\n",                0,         strptr:&(modelname),                    defstrval:"AWGN",                TYPE_STRING,    0 }\
  };

pthread_mutex_t Sockmutex;

typedef struct complex16 sample_t; // 2*16 bits complex number

typedef struct buffer_s {
  int conn_sock;
  openair0_timestamp lastReceivedTS;
  openair0_timestamp lastWroteTS;
  bool headerMode;
  samplesBlockHeader_t th;
  char *transferPtr;
  uint64_t remainToTransfer;
  char *circularBufEnd;
  sample_t *circularBuf;
  channel_desc_t *channel_model;
} buffer_t;

typedef struct {
  int listen_sock, epollfd;
  openair0_timestamp nextTimestamp;
  uint64_t typeStamp;
  char *ip;
  uint16_t port;
  int saveIQfile;
  buffer_t buf[FD_SETSIZE];
  int rx_num_channels;
  int tx_num_channels;
  double sample_rate;
  double tx_bw;
  int channelmod;
} rfsimulator_state_t;


void allocCirBuf(rfsimulator_state_t *bridge, int sock) {
  buffer_t *ptr=&bridge->buf[sock];
  AssertFatal ( (ptr->circularBuf=(sample_t *) malloc(sampleToByte(CirSize,1))) != NULL, "");
  ptr->circularBufEnd=((char *)ptr->circularBuf)+sampleToByte(CirSize,1);
  ptr->conn_sock=sock;
  ptr->lastReceivedTS=0;
  ptr->lastWroteTS=0;
  ptr->headerMode=true;
  ptr->transferPtr=(char *)&ptr->th;
  ptr->remainToTransfer=sizeof(samplesBlockHeader_t);
  int sendbuff=1000*1000*10;
  AssertFatal ( setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff)) == 0, "");
  struct epoll_event ev= {0};
  ev.events = EPOLLIN | EPOLLRDHUP;
  ev.data.fd = sock;
  AssertFatal(epoll_ctl(bridge->epollfd, EPOLL_CTL_ADD,  sock, &ev) != -1, "");

  if ( bridge->channelmod > 0) {
    // create channel simulation model for this mode reception
    // snr_dB is pure global, coming from configuration paramter "-s"
    // Fixme: referenceSignalPower should come from the right place
    // but the datamodel is inconsistant
    // legacy: RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower
    // (must not come from ru[]->frame_parms as it doesn't belong to ru !!!)
    // Legacy sets it as:
    // ptr->channel_model->path_loss_dB = -132.24 + snr_dB - RC.ru[0]->frame_parms->pdsch_config_common.referenceSignalPower;
    // we use directly the paramter passed on the command line ("-s")
    // the value channel_model->path_loss_dB seems only a storage place (new_channel_desc_scm() only copy the passed value)
    // Legacy changes directlty the variable channel_model->path_loss_dB place to place
    // while calling new_channel_desc_scm() with path losses = 0
    ptr->channel_model=new_channel_desc_scm(bridge->tx_num_channels,bridge->rx_num_channels,
                                            bridge->channelmod,
                                            bridge->sample_rate,
                                            bridge->tx_bw,
                                            0.0, // forgetting_factor
                                            0, // maybe used for TA
                                            0); // path_loss in dB
    random_channel(ptr->channel_model,false);
  }
}

void removeCirBuf(rfsimulator_state_t *bridge, int sock) {
  AssertFatal( epoll_ctl(bridge->epollfd, EPOLL_CTL_DEL,  sock, NULL) != -1, "");
  close(sock);
  free(bridge->buf[sock].circularBuf);
  // Fixme: no free_channel_desc_scm(bridge->buf[sock].channel_model) implemented
  // a lot of mem leaks
  free(bridge->buf[sock].channel_model);
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
                 or use rfsimulator.serveraddr configuration option\n\
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



void rfsimulator_readconfig(rfsimulator_state_t *rfsimulator) {
  char *saveF=NULL;
  char *modelname=NULL;
  paramdef_t rfsimu_params[] = RFSIMULATOR_PARAMS_DESC;
  int p = config_paramidx_fromname(rfsimu_params,sizeof(rfsimu_params)/sizeof(paramdef_t), RFSIMU_OPTIONS_PARAMNAME) ;
  int ret = config_get( rfsimu_params,sizeof(rfsimu_params)/sizeof(paramdef_t),RFSIMU_SECTION);
  AssertFatal(ret >= 0, "configuration couldn't be performed");
  rfsimulator->saveIQfile = -1;

  for(int i=0; i<rfsimu_params[p].numelt ; i++) {
    if (strcmp(rfsimu_params[p].strlistptr[i],"saviq") == 0) {
      rfsimulator->saveIQfile=open(saveF,O_APPEND| O_CREAT|O_TRUNC | O_WRONLY, 0666);

      if ( rfsimulator->saveIQfile != -1 )
        LOG_I(HW,"rfsimulator: will save written IQ samples  in %s\n", saveF);
      else
        LOG_E(HW, "can't open %s for IQ saving (%s)\n", saveF, strerror(errno));

      break;
    } else if (strcmp(rfsimu_params[p].strlistptr[i],"chanmod") == 0) {
      init_channelmod();
      rfsimulator->channelmod=modelid_fromname(modelname);
    } else {
      fprintf(stderr,"Unknown rfsimulator option: %s\n",rfsimu_params[p].strlistptr[i]);
      exit(-1);
    }
  }

  /* for compatibility keep environment variable usage */
  if ( getenv("RFSIMULATOR") != NULL ) {
    rfsimulator->ip=getenv("RFSIMULATOR");
  }

  if ( strncasecmp(rfsimulator->ip,"enb",3) == 0 ||
       strncasecmp(rfsimulator->ip,"server",3) == 0 )
    rfsimulator->typeStamp = ENB_MAGICDL_FDD;
  else
    rfsimulator->typeStamp = UE_MAGICDL_FDD;
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
    htons(t->port),
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
    htons(t->port),
sin_addr:
    { s_addr: INADDR_ANY }
  };
  addr.sin_addr.s_addr = inet_addr(t->ip);
  bool connected=false;

  while(!connected) {
    LOG_I(HW,"rfsimulator: trying to connect to %s:%d\n", t->ip, t->port);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
      LOG_I(HW,"rfsimulator: connection established\n");
      connected=true;
    }

    perror("rfsimulator");
    sleep(1);
  }

  setblocking(sock, notBlocking);
  allocCirBuf(t, sock);
  return 0;
}

int rfsimulator_write(openair0_device *device, openair0_timestamp timestamp, void **samplesVoid, int nsamps, int nbAnt, int flags) {
  rfsimulator_state_t *t = device->priv;
  LOG_D(HW,"sending %d samples at time: %ld\n", nsamps, timestamp);

  for (int i=0; i<FD_SETSIZE; i++) {
    buffer_t *b=&t->buf[i];

    if (b->conn_sock >= 0 ) {
      if ( abs((double)b->lastWroteTS-timestamp) > (double)CirSize)
        LOG_E(HW,"Tx/Rx shift too large Tx:%lu, Rx:%lu\n", b->lastWroteTS, b->lastReceivedTS);

      samplesBlockHeader_t header= {t->typeStamp, nsamps, nbAnt, timestamp};
      fullwrite(b->conn_sock,&header, sizeof(header), t);
      sample_t tmpSamples[nsamps][nbAnt];

      for(int a=0; a<nbAnt; a++) {
        sample_t *in=(sample_t *)samplesVoid[a];

        for(int s=0; s<nsamps; s++)
          tmpSamples[s][a]=in[s];
      }

      if (b->conn_sock >= 0 ) {
        fullwrite(b->conn_sock, (void *)tmpSamples, sampleToByte(nsamps,nbAnt), t);
        b->lastWroteTS=timestamp+nsamps;
      }
    }
  }

  LOG_D(HW,"sent %d samples at time: %ld->%ld, energy in first antenna: %d\n",
        nsamps, timestamp, timestamp+nsamps, signal_energy(samplesVoid[0], nsamps) );
  // Let's verify we don't have incoming data
  // This is mandatory when the opposite side don't transmit
  flushInput(t, 0);
  pthread_mutex_unlock(&Sockmutex);
  return nsamps;
}

static bool flushInput(rfsimulator_state_t *t, int timeout) {
  // Process all incoming events on sockets
  // store the data in lists
  struct epoll_event events[FD_SETSIZE]= {0};
  int nfds = epoll_wait(t->epollfd, events, FD_SETSIZE, timeout);

  if ( nfds==-1 ) {
    if ( errno==EINTR || errno==EAGAIN ) {
      return false;
    } else
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

        if ( b->lastReceivedTS != b->th.timestamp) {
          int nbAnt= b->th.nbAnt;

          for (uint64_t index=b->lastReceivedTS; index < b->th.timestamp; index++ ) {
            for (int a=0; a < nbAnt; a++) {
              b->circularBuf[(index*nbAnt+a)%CirSize].r=0;
              b->circularBuf[(index*nbAnt+a)%CirSize].i=0;
            }
          }

          if ( abs(b->th.timestamp-b->lastReceivedTS) > 50 )
            LOG_W(HW,"gap of: %ld in reception\n", b->th.timestamp-b->lastReceivedTS );
        }

        b->lastReceivedTS=b->th.timestamp;
        AssertFatal(b->lastWroteTS == 0 || ( abs((double)b->lastWroteTS-b->lastReceivedTS) < (double)CirSize),
                    "Tx/Rx shift too large Tx:%lu, Rx:%lu\n", b->lastWroteTS, b->lastReceivedTS);
        b->transferPtr=(char *)&b->circularBuf[(b->lastReceivedTS*b->th.nbAnt)%CirSize];
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
    LOG_W(HW, "rfsimulator: only 1 antenna tested\n");
  }

  pthread_mutex_lock(&Sockmutex);
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
    if ( t->nextTimestamp == 0)
      LOG_W(HW,"No connected device, generating void samples...\n");

    if (!flushInput(t, 10)) {
      for (int x=0; x < nbAnt; x++)
        memset(samplesVoid[x],0,sampleToByte(nsamps,1));

      t->nextTimestamp+=nsamps;

      if ( ((t->nextTimestamp/nsamps)%100) == 0)
        LOG_W(HW,"Generated void samples for Rx: %ld\n", t->nextTimestamp);

      *ptimestamp = t->nextTimestamp-nsamps;
      pthread_mutex_unlock(&Sockmutex);
      return nsamps;
    }
  } else {
    bool have_to_wait;

    do {
      have_to_wait=false;

      for ( int sock=0; sock<FD_SETSIZE; sock++) {
        buffer_t *b=&t->buf[sock];

        if ( b->circularBuf) {
          LOG_D(HW,"sock: %d, lastWroteTS: %lu, lastRecvTS: %lu, TS must be avail: %lu\n",
                sock, b->lastWroteTS,
                b->lastReceivedTS,
                t->nextTimestamp+nsamps);

          if (  b->lastReceivedTS > b->lastWroteTS ) {
            // The caller momdem (NB, UE, ...) must send Tx in advance, so we fill TX if Rx is in advance
            // This occurs for example when UE is in sync mode: it doesn't transmit
            // with USRP, it seems ok: if "tx stream" is off, we may consider it actually cuts the Tx power
            struct complex16 v= {0};
            void *samplesVoid[b->th.nbAnt];

            for ( int i=0; i <b->th.nbAnt; i++)
              samplesVoid[i]=(void *)&v;

            rfsimulator_write(device, b->lastReceivedTS, samplesVoid, 1, b->th.nbAnt, 0);
          }
        }

        if ( b->circularBuf )
          if ( t->nextTimestamp+nsamps > b->lastReceivedTS ) {
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

  // Add all input nodes signal in the output buffer
  for (int sock=0; sock<FD_SETSIZE; sock++) {
    buffer_t *ptr=&t->buf[sock];

    if ( ptr->circularBuf ) {
      bool reGenerateChannel=false;

      //fixme: when do we regenerate
      // it seems legacy behavior is: never in UL, each frame in DL
      if (reGenerateChannel)
        random_channel(ptr->channel_model,0);

      for (int a=0; a<nbAnt; a++) {
        if ( ptr->channel_model != NULL ) // apply a channel model
          rxAddInput( ptr->circularBuf, (struct complex16 *) samplesVoid[a],
                      a,
                      ptr->channel_model,
                      nsamps,
                      t->nextTimestamp,
                      CirSize
                    );
        else { // no channel modeling
          sample_t *out=(sample_t *)samplesVoid[a];

          for ( int i=0; i < nsamps; i++ ) {
            out[i].r+=ptr->circularBuf[((t->nextTimestamp+i)*nbAnt+a)%CirSize].r;
            out[i].i+=ptr->circularBuf[((t->nextTimestamp+i)*nbAnt+a)%CirSize].i;
          }
        } // end of no channel modeling
      } // end for a...
    }
  }

  *ptimestamp = t->nextTimestamp; // return the time of the first sample
  t->nextTimestamp+=nsamps;
  LOG_D(HW,"Rx to upper layer: %d from %ld to %ld, energy in first antenna %d\n",
        nsamps,
        *ptimestamp, t->nextTimestamp,
        signal_energy(samplesVoid[0], nsamps));
  pthread_mutex_unlock(&Sockmutex);
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
  // to change the log level, use this on command line
  // --log_config.hw_log_level debug
  rfsimulator_state_t *rfsimulator = (rfsimulator_state_t *)calloc(sizeof(rfsimulator_state_t),1);
  rfsimulator_readconfig(rfsimulator);
  pthread_mutex_init(&Sockmutex, NULL);
  LOG_I(HW,"rfsimulator: running as %s\n", rfsimulator-> typeStamp == ENB_MAGICDL_FDD ? "(eg)NB" : "UE");
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
  // initialize channel simulation
  rfsimulator->tx_num_channels=openair0_cfg->tx_num_channels;
  rfsimulator->rx_num_channels=openair0_cfg->rx_num_channels;
  rfsimulator->sample_rate=openair0_cfg->sample_rate;
  rfsimulator->tx_bw=openair0_cfg->tx_bw;
  randominit(0);
  set_taus_seed(0);
  return 0;
}
