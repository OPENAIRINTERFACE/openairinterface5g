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
#include <common/utils/telnetsrv/telnetsrv.h>
#include <common/config/config_userapi.h>
#include "common_lib.h"
#include <openair1/PHY/defs_eNB.h>
#include "openair1/PHY/defs_UE.h"
#define CHANNELMOD_DYNAMICLOAD
#include <openair1/SIMULATION/TOOLS/sim.h>
#include "rfsimulator.h"

#define PORT 4043 //default TCP port for this simulator
//
// CirSize defines the number of samples inquired for a read cycle
// It is bounded by a slot read capability (which depends on bandwidth and numerology)
// up to multiple slots read to allow I/Q buffering of the I/Q TCP stream
//
// As a rule of thumb:
// -it can't be less than the number of samples for a slot
// -it can range up to multiple slots
//
// The default value is chosen for 10ms buffering which makes 23040*20 = 460800 samples
// The previous value is kept below in comment it was computed for 100ms 1x 20MHz
// #define CirSize 6144000 // 100ms SiSo 20MHz LTE
#define CirSize 460800 // 10ms  SiSo 40Mhz 3/4 sampling NR78 FR1
#define sampleToByte(a,b) ((a)*(b)*sizeof(sample_t))
#define byteToSample(a,b) ((a)/(sizeof(sample_t)*(b)))

#define GENERATE_CHANNEL 10 // each frame (or slot?) in DL

// This needs to be re-architected in the future
//
// File Descriptors management in rfsimulator is not optimized
// Relying on FD_SETSIZE (actually 1024) is not appropriated
// Also the use of fd value as returned by Linux as an index for buf[] structure is not appropriated
// especially for client (UE) side since only 1 fd per connection to a gNB is needed. On the server
// side the value should be tuned to the maximum number of connections with UE's which corresponds
// to the maximum number of UEs hosted by a gNB which is unlikely to be in the order of thousands
// since all I/Q's would flow through the same TCP transport.
// Until a convenient management is implemented, the MAX_FD_RFSIMU is used everywhere (instead of
// FD_SETSIE) and reduced to 125. This should allow for around 20 simultaeous UEs.
//
// #define MAX_FD_RFSIMU FD_SETSIZE
#define MAX_FD_RFSIMU 250
#define SYSCTL_MEM_VALUE 134217728 // Kernel network buffer size
#define SEND_BUFF_SIZE SYSCTL_MEM_VALUE // Socket buffer size

// Simulator role
typedef enum { SIMU_ROLE_SERVER = 1, SIMU_ROLE_CLIENT } simuRole;

//

#define RFSIMU_SECTION    "rfsimulator"
#define RFSIMU_OPTIONS_PARAMNAME "options"


#define RFSIM_CONFIG_HELP_OPTIONS     " list of comma separated options to enable rf simulator functionalities. Available options: \n"\
  "        chanmod:   enable channel modelisation\n"\
  "        saviq:     enable saving written iqs to a file\n"
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            configuration parameters for the rfsimulator device                                                                              */
/*   optname                     helpstr                     paramflags           XXXptr                               defXXXval                          type         numelt  */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define simOpt PARAMFLAG_NOFREE|PARAMFLAG_CMDLINE_NOPREFIXENABLED
#define RFSIMULATOR_PARAMS_DESC {					\
    {"serveraddr",             "<ip address to connect to>\n",        simOpt,  .strptr=&rfsimulator->ip,               .defstrval="127.0.0.1",           TYPE_STRING,    0 },\
    {"serverport",             "<port to connect to>\n",              simOpt,  .u16ptr=&(rfsimulator->port),           .defuintval=PORT,                 TYPE_UINT16,    0 },\
    {RFSIMU_OPTIONS_PARAMNAME, RFSIM_CONFIG_HELP_OPTIONS,             0,       .strlistptr=NULL,                       .defstrlistval=NULL,              TYPE_STRINGLIST,0 },\
    {"IQfile",                 "<file path to use when saving IQs>\n",simOpt,  .strptr=&saveF,                         .defstrval="/tmp/rfsimulator.iqs",TYPE_STRING,    0 },\
    {"modelname",              "<channel model name>\n",              simOpt,  .strptr=&modelname,                     .defstrval="AWGN",                TYPE_STRING,    0 },\
    {"ploss",                  "<channel path loss in dB>\n",         simOpt,  .dblptr=&(rfsimulator->chan_pathloss),  .defdblval=0,                     TYPE_DOUBLE,    0 },\
    {"forgetfact",             "<channel forget factor ((0 to 1)>\n", simOpt,  .dblptr=&(rfsimulator->chan_forgetfact),.defdblval=0,                     TYPE_DOUBLE,    0 },\
    {"offset",                 "<channel offset in samps>\n",         simOpt,  .iptr=&(rfsimulator->chan_offset),      .defintval=0,                     TYPE_INT,       0 },\
    {"wait_timeout",           "<wait timeout if no UE connected>\n", simOpt,  .iptr=&(rfsimulator->wait_timeout),     .defintval=1,                     TYPE_INT,       0 },\
  };

static void getset_currentchannels_type(char *buf, int debug, webdatadef_t *tdata, telnet_printfunc_t prnt);
extern int get_currentchannels_type(char *buf, int debug, webdatadef_t *tdata, telnet_printfunc_t prnt); // in random_channel.c
static int rfsimu_setchanmod_cmd(char *buff, int debug, telnet_printfunc_t prnt, void *arg);
static int rfsimu_setdistance_cmd(char *buff, int debug, telnet_printfunc_t prnt, void *arg);
static int rfsimu_getdistance_cmd(char *buff, int debug, telnet_printfunc_t prnt, void *arg);
static int rfsimu_vtime_cmd(char *buff, int debug, telnet_printfunc_t prnt, void *arg);
static telnetshell_cmddef_t rfsimu_cmdarray[] = {
    {"show models", "", (cmdfunc_t)rfsimu_setchanmod_cmd, {(webfunc_t)getset_currentchannels_type}, TELNETSRV_CMDFLAG_WEBSRVONLY | TELNETSRV_CMDFLAG_GETWEBTBLDATA, NULL},
    {"setmodel", "<model name> <model type>", (cmdfunc_t)rfsimu_setchanmod_cmd, {NULL}, TELNETSRV_CMDFLAG_PUSHINTPOOLQ | TELNETSRV_CMDFLAG_TELNETONLY, NULL},
    {"setdistance", "<model name> <distance>", (cmdfunc_t)rfsimu_setdistance_cmd, {NULL}, TELNETSRV_CMDFLAG_PUSHINTPOOLQ | TELNETSRV_CMDFLAG_NEEDPARAM },
    {"getdistance", "<model name>", (cmdfunc_t)rfsimu_getdistance_cmd, {NULL}, TELNETSRV_CMDFLAG_PUSHINTPOOLQ},
    {"vtime", "", (cmdfunc_t)rfsimu_vtime_cmd, {NULL}, TELNETSRV_CMDFLAG_PUSHINTPOOLQ | TELNETSRV_CMDFLAG_AUTOUPDATE},
    {"", "", NULL},
};
static telnetshell_cmddef_t *setmodel_cmddef = &(rfsimu_cmdarray[1]);

static telnetshell_vardef_t rfsimu_vardef[] = {{"", 0, 0, NULL}};
pthread_mutex_t Sockmutex;
unsigned int nb_ue = 0;

typedef c16_t sample_t; // 2*16 bits complex number

typedef struct buffer_s {
  int conn_sock;
  openair0_timestamp lastReceivedTS;
  bool headerMode;
  bool trashingPacket;
  samplesBlockHeader_t th;
  char *transferPtr;
  uint64_t remainToTransfer;
  char *circularBufEnd;
  sample_t *circularBuf;
  channel_desc_t *channel_model;
} buffer_t;

typedef struct {
  int listen_sock, epollfd;
  openair0_timestamp nextRxTstamp;
  openair0_timestamp lastWroteTS;
  simuRole role;
  char *ip;
  uint16_t port;
  int saveIQfile;
  buffer_t buf[MAX_FD_RFSIMU];
  int rx_num_channels;
  int tx_num_channels;
  double sample_rate;
  double tx_bw;
  int channelmod;
  double chan_pathloss;
  double chan_forgetfact;
  int    chan_offset;
  float  noise_power_dB;
  void *telnetcmd_qid;
  poll_telnetcmdq_func_t poll_telnetcmdq;
  int wait_timeout;
} rfsimulator_state_t;

static int allocCirBuf(rfsimulator_state_t *bridge, int sock)
{
  buffer_t *ptr=&bridge->buf[sock];
  ptr->circularBuf = malloc(sampleToByte(CirSize, 1));
  if (ptr->circularBuf == NULL) {
    LOG_E(HW, "malloc(%lu) failed\n", sampleToByte(CirSize, 1));
    return -1;
  }
  ptr->circularBufEnd=((char *)ptr->circularBuf)+sampleToByte(CirSize,1);
  ptr->conn_sock=sock;
  ptr->lastReceivedTS=0;
  ptr->headerMode=true;
  ptr->trashingPacket=false;
  ptr->transferPtr=(char *)&ptr->th;
  ptr->remainToTransfer=sizeof(samplesBlockHeader_t);
  int sendbuff = SEND_BUFF_SIZE;
  if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff)) != 0) {
    LOG_E(HW, "setsockopt(SO_SNDBUF) failed\n");
    return -1;
  }
  struct epoll_event ev= {0};
  ev.events = EPOLLIN | EPOLLRDHUP;
  ev.data.fd = sock;
  if (epoll_ctl(bridge->epollfd, EPOLL_CTL_ADD, sock, &ev) != 0) {
    LOG_E(HW, "epoll_ctl(EPOLL_CTL_ADD) failed\n");
    return -1;
  }

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
    static bool init_done=false;

    if (!init_done) {
      uint64_t rand;
      FILE *h=fopen("/dev/random","r");

      if ( 1 != fread(&rand,sizeof(rand),1,h) )
        LOG_W(HW, "Can't read /dev/random\n");

      fclose(h);
      randominit(rand);
      tableNor(rand);
      init_done=true;
    }
    char *modelname = (bridge->role == SIMU_ROLE_SERVER) ? "rfsimu_channel_ue0" : "rfsimu_channel_enB0";
    ptr->channel_model = find_channel_desc_fromname(modelname); // path_loss in dB
    if (!ptr->channel_model) {
      LOG_E(HW, "Channel model %s not found, check config file\n", modelname);
      return -1;
    }

    set_channeldesc_owner(ptr->channel_model, RFSIMU_MODULEID);
    random_channel(ptr->channel_model,false);
    LOG_I(HW, "Random channel %s in rfsimulator activated\n", modelname);
  }
  return 0;
}

static void removeCirBuf(rfsimulator_state_t *bridge, int sock) {
  if (epoll_ctl(bridge->epollfd, EPOLL_CTL_DEL, sock, NULL) != 0) {
    LOG_E(HW, "epoll_ctl(EPOLL_CTL_DEL) failed\n");
  }
  close(sock);
  free(bridge->buf[sock].circularBuf);
  // Fixme: no free_channel_desc_scm(bridge->buf[sock].channel_model) implemented
  // a lot of mem leaks
  //free(bridge->buf[sock].channel_model);
  memset(&bridge->buf[sock], 0, sizeof(buffer_t));
  bridge->buf[sock].conn_sock=-1;
  nb_ue--;
}

static void socketError(rfsimulator_state_t *bridge, int sock) {
  if (bridge->buf[sock].conn_sock!=-1) {
    LOG_W(HW, "Lost socket\n");
    removeCirBuf(bridge, sock);

    if (bridge->role == SIMU_ROLE_CLIENT)
      exit(1);
  }
}

enum  blocking_t {
  notBlocking,
  blocking
};

static int setblocking(int sock, enum blocking_t active)
{
  int opts = fcntl(sock, F_GETFL);
  if (opts < 0) {
    LOG_E(HW, "fcntl(F_GETFL) failed, errno(%d)\n", errno);
    return -1;
  }

  if (active==blocking)
    opts = opts & ~O_NONBLOCK;
  else
    opts = opts | O_NONBLOCK;

  opts = fcntl(sock, F_SETFL, opts);
  if (opts < 0) {
    LOG_E(HW, "fcntl(F_SETFL) failed, errno(%d)\n", errno);
    return -1;
  }
  return 0;
}

static bool flushInput(rfsimulator_state_t *t, int timeout, int nsamps);

static void fullwrite(int fd, void *_buf, ssize_t count, rfsimulator_state_t *t) {
  if (t->saveIQfile != -1) {
    if (write(t->saveIQfile, _buf, count) != count )
      LOG_E(HW, "write() in save iq file failed (%d)\n", errno);
  }

  char *buf = _buf;
  ssize_t l;

  while (count) {
    l = write(fd, buf, count);

    if (l <= 0) {
      if (errno==EINTR)
        continue;

      if (errno == EAGAIN) {
        LOG_D(HW, "write() failed, errno(%d)\n", errno);
        usleep(250);
        continue;
      } else
        return;
    }

    count -= l;
    buf += l;
  }
}

static void rfsimulator_readconfig(rfsimulator_state_t *rfsimulator) {
  char *saveF=NULL;
  char *modelname=NULL;
  paramdef_t rfsimu_params[] = RFSIMULATOR_PARAMS_DESC;
  int p = config_paramidx_fromname(rfsimu_params, sizeofArray(rfsimu_params), RFSIMU_OPTIONS_PARAMNAME);
  int ret = config_get(config_get_if(), rfsimu_params, sizeofArray(rfsimu_params), RFSIMU_SECTION);
  AssertFatal(ret >= 0, "configuration couldn't be performed\n");

  rfsimulator->saveIQfile = -1;

  for(int i=0; i<rfsimu_params[p].numelt ; i++) {
    if (strcmp(rfsimu_params[p].strlistptr[i],"saviq") == 0) {
      rfsimulator->saveIQfile=open(saveF,O_APPEND| O_CREAT|O_TRUNC | O_WRONLY, 0666);

      if ( rfsimulator->saveIQfile != -1 )
        LOG_I(HW, "Will save written IQ samples in %s\n", saveF);
      else {
        LOG_E(HW, "open(%s) failed for IQ saving, errno(%d)\n", saveF, errno);
        exit(-1);
      }

      break;
    } else if (strcmp(rfsimu_params[p].strlistptr[i],"chanmod") == 0) {
      init_channelmod();
      load_channellist(rfsimulator->tx_num_channels, rfsimulator->rx_num_channels, rfsimulator->sample_rate, rfsimulator->tx_bw);
      rfsimulator->channelmod=true;
    } else {
      fprintf(stderr, "unknown rfsimulator option: %s\n", rfsimu_params[p].strlistptr[i]);
      exit(-1);
    }
  }

  /* for compatibility keep environment variable usage */
  if ( getenv("RFSIMULATOR") != NULL ) {
    rfsimulator->ip=getenv("RFSIMULATOR");
    LOG_W(HW, "The RFSIMULATOR environment variable is deprecated and support will be removed in the future. Instead, add parameter --rfsimulator.serveraddr %s to set the server address. Note: the default is \"server\"; for the gNB/eNB, you don't have to set any configuration.\n", rfsimulator->ip);
    LOG_I(HW, "Remove RFSIMULATOR environment variable to get rid of this message and the sleep.\n");
    sleep(10);
  }

  if ( strncasecmp(rfsimulator->ip,"enb",3) == 0 ||
       strncasecmp(rfsimulator->ip,"server",3) == 0 )
    rfsimulator->role = SIMU_ROLE_SERVER;
  else
    rfsimulator->role = SIMU_ROLE_CLIENT;
}

static int rfsimu_setchanmod_cmd(char *buff, int debug, telnet_printfunc_t prnt, void *arg) {
  char *modelname=NULL;
  char *modeltype=NULL;
  rfsimulator_state_t *t = (rfsimulator_state_t *)arg;
  if (t->channelmod == false) {
    prnt("%s: ERROR channel modelisation disabled...\n", __func__);
    return 0;
  }
  if (buff == NULL) {
    prnt("%s: ERROR wrong rfsimu setchannelmod command...\n", __func__);
    return 0;
  }
  if (debug)
    prnt("%s: rfsimu_setchanmod_cmd buffer \"%s\"\n", __func__, buff);
  int s = sscanf(buff,"%m[^ ] %ms\n",&modelname, &modeltype);

  if (s == 2) {
    int channelmod=modelid_fromstrtype(modeltype);

    if (channelmod<0)
      prnt("%s: ERROR: model type %s unknown\n", __func__, modeltype);
    else {
      rfsimulator_state_t *t = (rfsimulator_state_t *)arg;
      int found=0;
      for (int i = 0; i < MAX_FD_RFSIMU; i++) {
        buffer_t *b=&t->buf[i];
        if ( b->channel_model==NULL)
          continue;
        if (b->channel_model->model_name==NULL)
          continue;
        if (b->conn_sock >= 0 && (strcmp(b->channel_model->model_name,modelname)==0)) {
          channel_desc_t *newmodel = new_channel_desc_scm(t->tx_num_channels,
                                                          t->rx_num_channels,
                                                          channelmod,
                                                          t->sample_rate,
                                                          0,
                                                          t->tx_bw,
                                                          30e-9, // TDL delay-spread parameter
                                                          0.0,
                                                          CORR_LEVEL_LOW,
                                                          t->chan_forgetfact, // forgetting_factor
                                                          t->chan_offset, // maybe used for TA
                                                          t->chan_pathloss,
                                                          t->noise_power_dB); // path_loss in dB
          set_channeldesc_owner(newmodel, RFSIMU_MODULEID);
          set_channeldesc_name(newmodel,modelname);
          random_channel(newmodel,false);
          channel_desc_t *oldmodel=b->channel_model;
          b->channel_model=newmodel;
          free_channel_desc_scm(oldmodel);
          prnt("%s: New model type %s applied to channel %s connected to sock %d\n", __func__, modeltype, modelname, i);
          found=1;
          break;
        }
      } /* for */
      if (found==0)
        prnt("%s: Channel %s not found or not currently used\n", __func__, modelname);
    }
  } else {
    prnt("%s: ERROR: 2 parameters required: model name and model type (%i found)\n", __func__, s);
  }

  free(modelname);
  free(modeltype);
  return CMDSTATUS_FOUND;
}

static void getset_currentchannels_type(char *buf, int debug, webdatadef_t *tdata, telnet_printfunc_t prnt)
{
  if (strncmp(buf, "set", 3) == 0) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "setmodel %s %s", tdata->lines[0].val[1], tdata->lines[0].val[3]);
    push_telnetcmd_func_t push_telnetcmd = (push_telnetcmd_func_t)get_shlibmodule_fptr("telnetsrv", TELNET_PUSHCMD_FNAME);
    push_telnetcmd(setmodel_cmddef, cmd, prnt);
  } else {
    get_currentchannels_type("modify type", debug, tdata, prnt);
  }
}
/*getset_currentchannels_type */

//static void print_cirBuf(struct complex16 *circularBuf,
//                         uint64_t firstSample,
//                         uint32_t cirSize,
//                         int neg,
//                         int pos,
//                         int nbTx)
//{
//  for (int i = -neg; i < pos ; ++i) {
//    for (int txAnt = 0; txAnt < nbTx; txAnt++) {
//      const int idx = ((firstSample + i) * nbTx + txAnt + cirSize) % cirSize;
//      if (i == 0)
//        printf("->");
//      printf("%08x%08x\n", circularBuf[idx].r, circularBuf[idx].i);
//    }
//  }
//  printf("\n");
//}

static void rfsimu_offset_change_cirBuf(struct complex16 *circularBuf,
                                        uint64_t firstSample,
                                        uint32_t cirSize,
                                        int old_offset,
                                        int new_offset,
                                        int nbTx)
{
  //int start = max(new_offset, old_offset) + 10;
  //int end = 10;
  //printf("new_offset %d old_offset %d start %d end %d\n", new_offset, old_offset, start, end);
  //printf("ringbuffer before:\n");
  //print_cirBuf(circularBuf, firstSample, cirSize, start, end, nbTx);

  int doffset = new_offset - old_offset;
  if (doffset > 0) {
    /* Moving away, creating a gap. We need to insert "zero" samples between
     * the previous (end of the) slot and the new slot (at the ringbuffer
     * index) to prevent that the receiving side detects things that are not
     * in the channel (e.g., samples that have already been delivered). */
    for (int i = new_offset; i > 0; --i) {
      for (int txAnt = 0; txAnt < nbTx; txAnt++) {
        const int newidx = ((firstSample - i) * nbTx + txAnt + cirSize) % cirSize;
        if (i > doffset) {
          // shift samples not read yet
          const int oldidx = (newidx + doffset) % cirSize;
          circularBuf[newidx] = circularBuf[oldidx];
        } else {
          // create zero samples between slots
          const struct complex16 nullsample = {0, 0};
          circularBuf[newidx] = nullsample;
        }
      }
    }
  } else {
    /* Moving closer, creating overlap between samples. For simplicity, we
     * simply drop `doffset` samples at the end of the previous slot
     * (this is, in a sense, arbitrary). In a real channel, there would be
     * some overlap between samples, e.g., for `doffset == 1` we could add
     * two samples. I think that we cannot do that for multiple samples,
     * though, and so we just drop some */
    // drop the last -doffset samples of the previous slot
    for (int i = old_offset; i > -doffset; --i) {
      for (int txAnt = 0; txAnt < nbTx; txAnt++) {
        const int oldidx = ((firstSample - i) * nbTx + txAnt + cirSize) % cirSize;
        const int newidx = (oldidx - doffset) % cirSize;
        circularBuf[newidx] = circularBuf[oldidx];
      }
    }
  }

  //printf("ringbuffer after:\n");
  //print_cirBuf(circularBuf, firstSample, cirSize, start, end, nbTx);
}

static int rfsimu_setdistance_cmd(char *buff, int debug, telnet_printfunc_t prnt, void *arg)
{
  if (debug)
    prnt("%s() buffer \"%s\"\n", __func__, buff);

  char *modelname;
  int distance;
  int s = sscanf(buff,"%m[^ ] %d\n", &modelname, &distance);
  if (s != 2) {
    prnt("%s: require exact two parameters\n", __func__);
    return CMDSTATUS_VARNOTFOUND;
  }

  rfsimulator_state_t *t = (rfsimulator_state_t *)arg;
  const double sample_rate = t->sample_rate;
  const double c = 299792458; /* 3e8 */

  const int new_offset = (double) distance * sample_rate / c;
  const double new_distance = (double) new_offset * c / sample_rate;

  prnt("\n%s: new_offset %d new (exact) distance %.3f m\n", __func__, new_offset, new_distance);

  /* Set distance in rfsim and channel model, update channel and ringbuffer */
  for (int i = 0; i < MAX_FD_RFSIMU; i++) {
    buffer_t *b=&t->buf[i];
    if (b->conn_sock <= 0 || b->channel_model == NULL || b->channel_model->model_name == NULL || strcmp(b->channel_model->model_name, modelname) != 0) {
      if (b->channel_model != NULL && b->channel_model->model_name != NULL)
        prnt("  %s: model %s unmodified\n", __func__, b->channel_model->model_name);
      continue;
    }

    channel_desc_t *cd = b->channel_model;
    const int old_offset = cd->channel_offset;
    cd->channel_offset = new_offset;

    const int nbTx = cd->nb_tx;
    prnt("  %s: Modifying model %s...\n", __func__, modelname);
    rfsimu_offset_change_cirBuf(b->circularBuf, t->nextRxTstamp, CirSize, old_offset, new_offset, nbTx);
  }

  free(modelname);

  return CMDSTATUS_FOUND;
}

static int rfsimu_getdistance_cmd(char *buff, int debug, telnet_printfunc_t prnt, void *arg)
{
  if (debug)
    prnt("%s() buffer \"%s\"\n", __func__, (buff != NULL) ? buff : "NULL");

  rfsimulator_state_t *t = (rfsimulator_state_t *)arg;
  const double sample_rate = t->sample_rate;
  const double c = 299792458; /* 3e8 */

  for (int i = 0; i < MAX_FD_RFSIMU; i++) {
    buffer_t *b=&t->buf[i];
    if (b->conn_sock <= 0 || b->channel_model == NULL || b->channel_model->model_name == NULL)
      continue;

    channel_desc_t *cd = b->channel_model;
    const int offset = cd->channel_offset;
    const double distance = (double) offset * c / sample_rate;
    prnt("%s: \%s offset %d distance %.3f m\n", __func__, cd->model_name, offset, distance);
  }

  return CMDSTATUS_FOUND;
}

static int rfsimu_vtime_cmd(char *buff, int debug, telnet_printfunc_t prnt, void *arg)
{
  rfsimulator_state_t *t = (rfsimulator_state_t *)arg;
  const openair0_timestamp ts = t->nextRxTstamp;
  const double sample_rate = t->sample_rate;
  prnt("%s: vtime measurement: TS %llu sample_rate %.3f\n", __func__, ts, sample_rate);
  return CMDSTATUS_FOUND;
}

static void customNetForPerf()
{
  int res = 0;
  char sysctlmem[256];
  memset(sysctlmem, 0, 256);
  sprintf(sysctlmem, "/sbin/sysctl -n -e -q -w net.core.rmem_default=%d", SYSCTL_MEM_VALUE);
  LOG_W(HW, "running command \"%s\" to increase RFsim performance\n", sysctlmem);
  res = system(sysctlmem);
  if (res != 0) {
    LOG_W(HW, "Cannot set net.core.rmem_default to %d\n", SYSCTL_MEM_VALUE);
  }
  memset(sysctlmem, 0, 256);
  sprintf(sysctlmem, "/sbin/sysctl -n -e -q -w net.core.rmem_max=%d", SYSCTL_MEM_VALUE);
  LOG_W(HW, "running command \"%s\" to increase RFsim performance\n", sysctlmem);
  res = system(sysctlmem);
  if (res != 0) {
    LOG_W(HW, "Cannot set net.core.rmem_max to %d\n", SYSCTL_MEM_VALUE);
  }
  memset(sysctlmem, 0, 256);
  sprintf(sysctlmem, "/sbin/sysctl -n -e -q -w net.core.wmem_default=%d", SYSCTL_MEM_VALUE);
  LOG_W(HW, "running command \"%s\" to increase RFsim performance\n", sysctlmem);
  res = system(sysctlmem);
  if (res != 0) {
    LOG_W(HW, "Cannot set net.core.wmem_default to %d\n", SYSCTL_MEM_VALUE);
  }
  memset(sysctlmem, 0, 256);
  sprintf(sysctlmem, "/sbin/sysctl -n -e -q -w net.core.wmem_max=%d", SYSCTL_MEM_VALUE);
  LOG_W(HW, "running command \"%s\" to increase RFsim performance\n", sysctlmem);
  res = system(sysctlmem);
  if (res != 0) {
    LOG_W(HW, "Cannot set net.core.wmem_max to %d\n", SYSCTL_MEM_VALUE);
  }
}

static int startServer(openair0_device *device) {
  rfsimulator_state_t *t = (rfsimulator_state_t *) device->priv;
  t->role = SIMU_ROLE_SERVER;
  t->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (t->listen_sock < 0) {
    LOG_E(HW, "socket(SOCK_STREAM) failed, errno(%d)\n", errno);
    return -1;
  }
  int enable = 1;
  if (setsockopt(t->listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0) {
    LOG_E(HW, "setsockopt(SO_REUSEADDR) failed, errno(%d)\n", errno);
    return -1;
  }
  struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(t->port), .sin_addr = {.s_addr = INADDR_ANY}};
  customNetForPerf();
  int rc = bind(t->listen_sock, (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0) {
    LOG_E(HW, "bind() failed, errno(%d)\n", errno);
    return -1;
  }
  if (listen(t->listen_sock, 5) != 0) {
    LOG_E(HW, "listen() failed, errno(%d)\n", errno);
    return -1;
  }
  struct epoll_event ev= {0};
  ev.events = EPOLLIN;
  ev.data.fd = t->listen_sock;
  if (epoll_ctl(t->epollfd, EPOLL_CTL_ADD, t->listen_sock, &ev) != 0) {
    LOG_E(HW, "epoll_ctl(EPOLL_CTL_ADD) failed, errno(%d)\n", errno);
    return -1;
  }
  return 0;
}

static int startClient(openair0_device *device) {
  rfsimulator_state_t *t = device->priv;
  t->role = SIMU_ROLE_CLIENT;
  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    LOG_E(HW, "socket(SOCK_STREAM) failed, errno(%d)\n", errno);
    return -1;
  }
  struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(t->port), .sin_addr = {.s_addr = INADDR_ANY}};
  addr.sin_addr.s_addr = inet_addr(t->ip);
  bool connected=false;

  customNetForPerf();
  while(!connected) {
    LOG_I(HW, "Trying to connect to %s:%d\n", t->ip, t->port);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
      LOG_I(HW, "Connection to %s:%d established\n", t->ip, t->port);
      connected=true;
    }

    LOG_I(HW, "connect() to %s:%d failed, errno(%d)\n", t->ip, t->port, errno);
    sleep(1);
  }

  if (setblocking(sock, notBlocking) == -1) {
    return -1;
  }
  return allocCirBuf(t, sock);
}

static int rfsimulator_write_internal(rfsimulator_state_t *t, openair0_timestamp timestamp, void **samplesVoid, int nsamps, int nbAnt, int flags, bool alreadyLocked) {
  if (!alreadyLocked)
    pthread_mutex_lock(&Sockmutex);

  LOG_D(HW, "Sending %d samples at time: %ld, nbAnt %d\n", nsamps, timestamp, nbAnt);

  for (int i = 0; i < MAX_FD_RFSIMU; i++) {
    buffer_t *b=&t->buf[i];

    if (b->conn_sock >= 0 ) {
      samplesBlockHeader_t header = {nsamps, nbAnt, timestamp};
      fullwrite(b->conn_sock,&header, sizeof(header), t);
      sample_t tmpSamples[nsamps][nbAnt];

      if (nbAnt == 1) {
        if (b->conn_sock >= 0) {
          fullwrite(b->conn_sock, samplesVoid[0], sampleToByte(nsamps, nbAnt), t);
        }
      } else {
        for (int a = 0; a < nbAnt; a++) {
          sample_t *in = (sample_t *)samplesVoid[a];

          for (int s = 0; s < nsamps; s++)
            tmpSamples[s][a] = in[s];
        }

        if (b->conn_sock >= 0) {
          fullwrite(b->conn_sock, (void *)tmpSamples, sampleToByte(nsamps, nbAnt), t);
        }
      }
    }
  }

  if ( t->lastWroteTS != 0 && fabs((double)t->lastWroteTS-timestamp) > (double)CirSize)
    LOG_W(HW, "Discontinuous TX gap too large Tx:%lu, %lu\n", t->lastWroteTS, timestamp);

  if (t->lastWroteTS > timestamp+nsamps)
    LOG_W(HW, "Not supported to send Tx out of order %lu, %lu\n", t->lastWroteTS, timestamp);

  t->lastWroteTS=timestamp+nsamps;

  if (!alreadyLocked)
    pthread_mutex_unlock(&Sockmutex);

  LOG_D(HW,
        "Sent %d samples at time: %ld->%ld, energy in first antenna: %d\n",
        nsamps,
        timestamp,
        timestamp + nsamps,
        signal_energy(samplesVoid[0], nsamps));
  return nsamps;
}

static int rfsimulator_write(openair0_device *device, openair0_timestamp timestamp, void **samplesVoid, int nsamps, int nbAnt, int flags) {
  return rfsimulator_write_internal(device->priv, timestamp, samplesVoid, nsamps, nbAnt, flags, false); // false = with lock
  // return rfsimulator_write_internal(device->priv, timestamp, samplesVoid, nsamps, nbAnt, flags, true);
}

static bool flushInput(rfsimulator_state_t *t, int timeout, int nsamps_for_initial) {
  // Process all incoming events on sockets
  // store the data in lists
  struct epoll_event events[MAX_FD_RFSIMU] = {{0}};
  int nfds = epoll_wait(t->epollfd, events, MAX_FD_RFSIMU, timeout);

  if ( nfds==-1 ) {
    if ( errno==EINTR || errno==EAGAIN ) {
      return false;
    } else {
      LOG_W(HW, "epoll_wait() failed, errno(%d)\n", errno);
      return false;
    }
  }

  for (int nbEv = 0; nbEv < nfds; ++nbEv) {
    int fd=events[nbEv].data.fd;

    if (events[nbEv].events & EPOLLIN && fd == t->listen_sock) {
      int conn_sock;
      conn_sock = accept(t->listen_sock, NULL, NULL);
      if (conn_sock == -1) {
        LOG_E(HW, "accept() failed, errno(%d)\n", errno);
        return false;
      }
      if (setblocking(conn_sock, notBlocking)) {
        return false;
      }
      if (allocCirBuf(t, conn_sock) == -1) {
        return false;
      }
      LOG_I(HW, "A client connects, sending the current time\n");
      c16_t v= {0};
      nb_ue++;
      void *samplesVoid[t->tx_num_channels];

      for ( int i=0; i < t->tx_num_channels; i++)
        samplesVoid[i]=(void *)&v;

      rfsimulator_write_internal(t, t->lastWroteTS > 1 ? t->lastWroteTS - 1 : 0, samplesVoid, 1, t->tx_num_channels, 1, false);
    } else {
      if ( events[nbEv].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP) ) {
        socketError(t,fd);
        continue;
      }

      buffer_t *b=&t->buf[fd];

      if ( b->circularBuf == NULL ) {
        LOG_E(HW, "Received data on not connected socket %d\n", events[nbEv].data.fd);
        continue;
      }

      ssize_t blockSz;

      if ( b->headerMode)
        blockSz=b->remainToTransfer;
      else
        blockSz= b->transferPtr + b->remainToTransfer <= b->circularBufEnd ?
                 b->remainToTransfer :
                 b->circularBufEnd - b->transferPtr ;

      ssize_t sz=recv(fd, b->transferPtr, blockSz, MSG_DONTWAIT);

      if ( sz < 0 ) {
        if ( errno != EAGAIN ) {
          LOG_E(HW, "recv() failed, errno(%d)\n", errno);
          //abort();
        }
      } else if ( sz == 0 )
        continue;

      LOG_D(HW, "Socket rcv %zd bytes\n", sz);
      b->remainToTransfer -= sz;
      b->transferPtr+=sz;

      if (b->transferPtr==b->circularBufEnd )
        b->transferPtr=(char *)b->circularBuf;

      // check the header and start block transfer
      if ( b->headerMode==true && b->remainToTransfer==0) {
        b->headerMode = false;

        if (t->nextRxTstamp == 0) { // First block in UE, resync with the gNB current TS
          t->nextRxTstamp=b->th.timestamp> nsamps_for_initial ?
                           b->th.timestamp -  nsamps_for_initial :
                           0;
          b->lastReceivedTS=b->th.timestamp> nsamps_for_initial ?
                            b->th.timestamp :
                            nsamps_for_initial;
          LOG_D(HW, "UE got first timestamp: starting at %lu\n", t->nextRxTstamp);
          b->trashingPacket=true;
        } else if (b->lastReceivedTS < b->th.timestamp) {
          int nbAnt= b->th.nbAnt;

          if ( b->th.timestamp-b->lastReceivedTS < CirSize ) {
            for (uint64_t index=b->lastReceivedTS; index < b->th.timestamp; index++ ) {
              for (int a=0; a < nbAnt; a++) {
                b->circularBuf[(index*nbAnt+a)%CirSize].r = 0;
                b->circularBuf[(index*nbAnt+a)%CirSize].i = 0;
              }
            }
          } else {
            memset(b->circularBuf, 0, sampleToByte(CirSize,1));
          }

          b->lastReceivedTS=b->th.timestamp;
        } else if (b->lastReceivedTS > b->th.timestamp && b->th.size == 1) {
          LOG_W(HW, "Received Rx/Tx synchro out of order\n");
          b->trashingPacket=true;
        } else if (b->lastReceivedTS == b->th.timestamp) {
          // normal case
        } else {
          LOG_W(HW, "Received data in past: current is %lu, new reception: %lu!\n", b->lastReceivedTS, b->th.timestamp);
          b->trashingPacket=true;
        }

        pthread_mutex_lock(&Sockmutex);

        if (t->lastWroteTS != 0 && (fabs((double)t->lastWroteTS-b->lastReceivedTS) > (double)CirSize))
          LOG_W(HW, "UEsock(%d) Tx/Rx shift too large Tx:%lu, Rx:%lu\n", fd, t->lastWroteTS, b->lastReceivedTS);

        pthread_mutex_unlock(&Sockmutex);
        b->transferPtr=(char *)&b->circularBuf[(b->lastReceivedTS*b->th.nbAnt)%CirSize];
        b->remainToTransfer=sampleToByte(b->th.size, b->th.nbAnt);
      }

      if ( b->headerMode==false ) {
        if ( ! b->trashingPacket ) {
          b->lastReceivedTS=b->th.timestamp+b->th.size-byteToSample(b->remainToTransfer,b->th.nbAnt);
          LOG_D(HW, "UEsock: %d Set b->lastReceivedTS %ld\n", fd, b->lastReceivedTS);
        }

        if ( b->remainToTransfer==0) {
          LOG_D(HW, "UEsock: %d Completed block reception: %ld\n", fd, b->lastReceivedTS);
          b->headerMode=true;
          b->transferPtr=(char *)&b->th;
          b->remainToTransfer = sizeof(samplesBlockHeader_t);
          b->trashingPacket=false;
        }
      }
    }
  }

  return nfds>0;
}

static int rfsimulator_read(openair0_device *device, openair0_timestamp *ptimestamp, void **samplesVoid, int nsamps, int nbAnt)
{
  rfsimulator_state_t *t = device->priv;
  LOG_D(HW, "Enter rfsimulator_read, expect %d samples, will release at TS: %ld, nbAnt %d\n", nsamps, t->nextRxTstamp+nsamps, nbAnt);

  // deliver data from received data
  // check if a UE is connected
  int first_sock;

  for (first_sock = 0; first_sock < MAX_FD_RFSIMU; first_sock++)
    if (t->buf[first_sock].circularBuf != NULL )
      break;

  if (first_sock == MAX_FD_RFSIMU) {
    // no connected device (we are eNB, no UE is connected)
    if ( t->nextRxTstamp == 0)
      LOG_W(HW, "No connected device, generating void samples...\n");

    if (!flushInput(t, t->wait_timeout,  nsamps)) {
      for (int x=0; x < nbAnt; x++)
        memset(samplesVoid[x],0,sampleToByte(nsamps,1));

      t->nextRxTstamp+=nsamps;

      if ( ((t->nextRxTstamp/nsamps)%100) == 0)
        LOG_D(HW, "No UE, Generating void samples for Rx: %ld\n", t->nextRxTstamp);

      *ptimestamp = t->nextRxTstamp-nsamps;
      return nsamps;
    }
  } else {
    bool have_to_wait;

    do {
      have_to_wait=false;

      buffer_t *b = NULL;
      for (int sock = 0; sock < MAX_FD_RFSIMU; sock++) {
        b = &t->buf[sock];

        if ( b->circularBuf )
          if ( t->nextRxTstamp+nsamps > b->lastReceivedTS ) {
            have_to_wait=true;
            break;
          }
      }

      if (have_to_wait) {
        LOG_D(HW,
              "Waiting on socket, current last ts: %ld, expected at least : %ld\n",
              b->lastReceivedTS,
              t->nextRxTstamp + nsamps);
        flushInput(t, 3, nsamps);
      }
    } while (have_to_wait);
  }

  // Clear the output buffer
  for (int a=0; a<nbAnt; a++)
    memset(samplesVoid[a],0,sampleToByte(nsamps,1));

  // Add all input nodes signal in the output buffer
  for (int sock = 0; sock < MAX_FD_RFSIMU; sock++) {
    buffer_t *ptr=&t->buf[sock];

    if ( ptr->circularBuf ) {
      bool reGenerateChannel=false;

      //fixme: when do we regenerate
      // it seems legacy behavior is: never in UL, each frame in DL
      if (reGenerateChannel)
        random_channel(ptr->channel_model,0);

      if (t->poll_telnetcmdq)
        t->poll_telnetcmdq(t->telnetcmd_qid,t);

      for (int a=0; a<nbAnt; a++) {//loop over number of Rx antennas
        if ( ptr->channel_model != NULL ) { // apply a channel model
          rxAddInput(ptr->circularBuf, (c16_t *) samplesVoid[a],
                     a,
                     ptr->channel_model,
                     nsamps,
                     t->nextRxTstamp,
                     CirSize);
        }
        else { // no channel modeling
          int nbAnt_tx = ptr->th.nbAnt; // number of Tx antennas
          if ((nbAnt_tx == 1) && ((nb_ue == 1) || (t->role == SIMU_ROLE_CLIENT))) { // optimized for 1 Tx and 1 UE
            sample_t *out = (sample_t *)samplesVoid[a];
            int firstIndex = t->nextRxTstamp % CirSize;
            sample_t *firstSample = (sample_t *)&(ptr->circularBuf[firstIndex]);
            if (firstIndex + nsamps > CirSize) {
              int tailSz = CirSize - firstIndex;
              memcpy(out, firstSample, sampleToByte(tailSz, nbAnt_tx));
              memcpy(out + tailSz, &ptr->circularBuf[0], sampleToByte(nsamps - tailSz, nbAnt_tx));
            } else {
              memcpy(out, firstSample, nsamps * 4);
            }
          } else {
            // SIMD (with simde) optimization might be added here later
            double H_awgn_mimo[4][4] = {{1.0, 0.2, 0.1, 0.05}, // rx 0
                                        {0.2, 1.0, 0.2, 0.1}, // rx 1
                                        {0.1, 0.2, 1.0, 0.2}, // rx 2
                                        {0.05, 0.1, 0.2, 1.0}}; // rx 3

            sample_t *out = (sample_t *)samplesVoid[a];

            LOG_D(HW, "nbAnt_tx %d\n", nbAnt_tx);
            for (int i = 0; i < nsamps; i++) { // loop over nsamps
              for (int a_tx = 0; a_tx < nbAnt_tx; a_tx++) { // sum up signals from nbAnt_tx antennas
                out[i].r += (short)(ptr->circularBuf[((t->nextRxTstamp + i) * nbAnt_tx + a_tx) % CirSize].r * H_awgn_mimo[a][a_tx]);
                out[i].i += (short)(ptr->circularBuf[((t->nextRxTstamp + i) * nbAnt_tx + a_tx) % CirSize].i * H_awgn_mimo[a][a_tx]);
              } // end for a_tx
            } // end for i (number of samps)
          } // end of 1 tx antenna optimization
        } // end of no channel modeling
      } // end for a (number of rx antennas)
    }
  }

  *ptimestamp = t->nextRxTstamp; // return the time of the first sample
  t->nextRxTstamp+=nsamps;
  LOG_D(HW,
        "Rx to upper layer: %d from %ld to %ld, energy in first antenna %d\n",
        nsamps,
        *ptimestamp,
        t->nextRxTstamp,
        signal_energy(samplesVoid[0], nsamps));
  return nsamps;
}

static int rfsimulator_get_stats(openair0_device *device) {
  return 0;
}
static int rfsimulator_reset_stats(openair0_device *device) {
  return 0;
}
static void rfsimulator_end(openair0_device *device) {
  rfsimulator_state_t* s = device->priv;
  for (int i = 0; i < MAX_FD_RFSIMU; i++) {
    buffer_t *b = &s->buf[i];
    if (b->conn_sock >= 0 )
      close(b->conn_sock);
  }
  close(s->epollfd);
}
static int rfsimulator_stop(openair0_device *device) {
  return 0;
}
static int rfsimulator_set_freq(openair0_device *device, openair0_config_t *openair0_cfg) {
  return 0;
}
static int rfsimulator_set_gains(openair0_device *device, openair0_config_t *openair0_cfg) {
  return 0;
}
static int rfsimulator_write_init(openair0_device *device) {
  return 0;
}
__attribute__((__visibility__("default")))
int device_init(openair0_device *device, openair0_config_t *openair0_cfg) {
  // to change the log level, use this on command line
  // --log_config.hw_log_level debug
  rfsimulator_state_t *rfsimulator = calloc(sizeof(rfsimulator_state_t), 1);
  // initialize channel simulation
  rfsimulator->tx_num_channels=openair0_cfg->tx_num_channels;
  rfsimulator->rx_num_channels=openair0_cfg->rx_num_channels;
  rfsimulator->sample_rate=openair0_cfg->sample_rate;
  rfsimulator->tx_bw=openair0_cfg->tx_bw;  
  rfsimulator_readconfig(rfsimulator);
  LOG_W(HW, "sample_rate %f\n", rfsimulator->sample_rate);
  pthread_mutex_init(&Sockmutex, NULL);
  LOG_I(HW,
        "Running as %s\n",
        rfsimulator->role == SIMU_ROLE_SERVER ? "server waiting opposite rfsimulators to connect"
                                              : "client: will connect to a rfsimulator server side");
  device->trx_start_func = rfsimulator->role == SIMU_ROLE_SERVER ? startServer : startClient;
  device->trx_get_stats_func   = rfsimulator_get_stats;
  device->trx_reset_stats_func = rfsimulator_reset_stats;
  device->trx_end_func         = rfsimulator_end;
  device->trx_stop_func        = rfsimulator_stop;
  device->trx_set_freq_func    = rfsimulator_set_freq;
  device->trx_set_gains_func   = rfsimulator_set_gains;
  device->trx_write_func       = rfsimulator_write;
  device->trx_read_func      = rfsimulator_read;
  /* let's pretend to be a b2x0 */
  device->type = RFSIMULATOR;
  openair0_cfg[0].rx_gain[0] = 0;
  device->openair0_cfg=&openair0_cfg[0];
  device->priv = rfsimulator;
  device->trx_write_init = rfsimulator_write_init;

  for (int i = 0; i < MAX_FD_RFSIMU; i++)
    rfsimulator->buf[i].conn_sock=-1;

  AssertFatal((rfsimulator->epollfd = epoll_create1(0)) != -1, "epoll_create1() failed, errno(%d)", errno);
  // we need to call randominit() for telnet server (use gaussdouble=>uniformrand)
  randominit(0);
  set_taus_seed(0);
  /* look for telnet server, if it is loaded, add the channel modeling commands to it */
  add_telnetcmd_func_t addcmd = (add_telnetcmd_func_t)get_shlibmodule_fptr("telnetsrv", TELNET_ADDCMD_FNAME);

  if (addcmd != NULL) {
    rfsimulator->poll_telnetcmdq =  (poll_telnetcmdq_func_t)get_shlibmodule_fptr("telnetsrv", TELNET_POLLCMDQ_FNAME);
    addcmd("rfsimu",rfsimu_vardef,rfsimu_cmdarray);

    for(int i=0; rfsimu_cmdarray[i].cmdfunc != NULL; i++) {
      if (  rfsimu_cmdarray[i].qptr != NULL) {
        rfsimulator->telnetcmd_qid = rfsimu_cmdarray[i].qptr;
        break;
      }
    }
  }

  /* write on a socket fails if the other end is closed and we get SIGPIPE */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    perror("SIGPIPE");
    exit(1);
  }

  return 0;
}
