#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>

#include <openair2/COMMON/platform_types.h>
#include <openair3/UTILS/conversions.h>
#include "common/utils/LOG/log.h"
#include <common/utils/ocp_itti/intertask_interface.h>
#include <openair2/COMMON/gtpv1_u_messages_types.h>
#include <openair3/ocp-gtpu/gtp_itf.h>
//#include <openair1/PHY/phy_extern.h>
#include <map>
using namespace std;

#pragma pack(1)

typedef struct Gtpv1uMsgHeader {
  uint8_t PN:1;
  uint8_t S:1;
  uint8_t E:1;
  uint8_t spare:1;
  uint8_t PT:1;
  uint8_t version:3;
  uint8_t msgType;
  uint16_t msgLength;
  uint32_t teid;
} __attribute__((packed)) Gtpv1uMsgHeaderT;

#pragma pack()

  // TS 29.060, table 7.1 defines the possible message types
  // here are all the possible messages (3GPP R16)
#define GTP_ECHO_REQ                                         (1)
#define GTP_ECHO_RSP                                         (2)
#define GTP_ERROR_INDICATION                                 (26)
#define GTP_SUPPORTED_EXTENSION_HEADER_INDICATION            (31)
#define GTP_END_MARKER                                       (254)
#define GTP_GPDU                                             (255)


typedef struct ocp_gtpv1u_bearer_s {
  /* TEID used in dl and ul */
  teid_t          teid_incoming;                ///< eNB TEID
  teid_t          teid_outgoing;                ///< Remote TEID
  in_addr_t       outgoing_ip_addr;
  struct in6_addr outgoing_ip6_addr;
  tcp_udp_port_t  outgoing_port;
  uint16_t        seqNum;
  uint8_t         npduNum;
} ocp_gtpv1u_bearer_t;

typedef struct {
  map<int, ocp_gtpv1u_bearer_t> bearers;
} teidData_t;

typedef struct {
  rnti_t rnti;
} rntiData_t;

class gtpEndPoint {
 public:
  openAddr_t addr;
  map<int,teidData_t> ue2te_mapping;
  map<int,rntiData_t> te2ue_mapping;
};

class gtpEndPoints {
 public:
  pthread_mutex_t gtp_lock=PTHREAD_MUTEX_INITIALIZER;
  // the instance id will be the Linux socket handler, as this is uniq
  map<int, gtpEndPoint> instances;
};

gtpEndPoints globGtp;

#ifdef __cplusplus
extern "C" {
#endif

static  uint32_t gtpv1uNewTeid(void) {
#ifdef GTPV1U_LINEAR_TEID_ALLOCATION
  g_gtpv1u_teid = g_gtpv1u_teid + 1;
  return g_gtpv1u_teid;
#else
  return random() + random() % (RAND_MAX - 1) + 1;
#endif
}


#define GTPV1U_HEADER_SIZE                                  (8)
static  int gtpv1uCreateAndSendMsg(int h, uint32_t peerIp, uint16_t peerPort, uint32_t teid, uint8_t *Msg,int msgLen,
                                   bool seqNumFlag, bool  npduNumFlag, bool extHdrFlag, int seqNum, int npduNum, int extHdrType) {
  AssertFatal(extHdrFlag==false,"Not developped");
  int headerAdditional=0;

  if ( seqNumFlag || npduNumFlag || extHdrFlag)
    headerAdditional=4;

  uint8_t *buffer;
  int fullSize=GTPV1U_HEADER_SIZE+headerAdditional+msgLen;
  AssertFatal((buffer=(uint8_t *) malloc(fullSize)) != NULL, "");
  Gtpv1uMsgHeaderT      *msgHdr = (Gtpv1uMsgHeaderT *)buffer ;
  msgHdr->PN=npduNumFlag;
  msgHdr->S=seqNumFlag;
  msgHdr->E=extHdrFlag;
  msgHdr->spare=0;
  msgHdr->PT=1;
  msgHdr->version=1;
  msgHdr->msgType=GTP_GPDU;
  msgHdr->msgLength=htons(msgLen);
  msgHdr->teid=htonl(teid);

  if(seqNumFlag || extHdrFlag || npduNumFlag) {
    *((uint16_t *) (buffer+8)) = seqNumFlag ? htons(seqNum) : 0x0000;
    *((uint8_t *) (buffer+10)) = npduNumFlag ? htons(npduNum) : 0x00;
    *((uint8_t *) (buffer+11)) = extHdrFlag ? htons(extHdrType) : 0x00;
  }

  memcpy(buffer+GTPV1U_HEADER_SIZE+headerAdditional, Msg, msgLen);
  static struct sockaddr_in to= {0};
  to.sin_family      = AF_INET;
  to.sin_port        = htons(peerPort);
  to.sin_addr.s_addr = peerIp ;
  LOG_D(GTPU,"sending packet size: %d to %s\n",fullSize, inet_ntoa(to.sin_addr) );

  if (sendto(h, (void *)buffer, (size_t)fullSize, 0,(struct sockaddr *)&to, sizeof(to) ) != fullSize ) {
    LOG_E(GTPU,
          "[SD %d] Failed to send data to " IPV4_ADDR " on port %d, buffer size %u\n",
          h, IPV4_ADDR_FORMAT(peerIp), peerPort, fullSize);
    free(buffer);
    return GTPNOK;
  }

  free(buffer);
  return  !GTPNOK;
}

static void gtpv1uSend(instance_t instance, gtpv1u_enb_tunnel_data_req_t *req, bool seqNumFlag, bool npduNumFlag) {
  uint8_t *buffer=req->buffer+req->offset;
  size_t length=req->length;
  uint64_t rnti=req->rnti;
  int  rab_id=req->rab_id;
  pthread_mutex_lock(&globGtp.gtp_lock);
  auto inst=&globGtp.instances[instance];
  auto ptrRnti=inst->ue2te_mapping.find(rnti);

  if (  ptrRnti==inst->ue2te_mapping.end() ) {
    LOG_E(GTPU, "Gtpv1uProcessUlpReq failed: while getting ue rnti %lx in hashtable ue_mapping\n", rnti);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return;
  }

  auto ptr=ptrRnti->second.bearers;

  if ( ptr.find(rab_id) == ptr.end() ) {
    LOG_E(GTPU,"sending a packet to a non existant RNTI:RAB: %lx/%x\n", rnti, rab_id);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return;
  } else
    LOG_D(GTPU,"sending a packet to RNTI:RAB:teid %lx/%x/%x, len %lu, oldseq %d, oldnum %d\n",
          rnti, rab_id,ptr[rab_id].teid_outgoing,length,  ptr[rab_id].seqNum,ptr[rab_id].npduNum );

  if(seqNumFlag)
    ptr[rab_id].seqNum++;

  if(npduNumFlag)
    ptr[rab_id].npduNum++;

  pthread_mutex_unlock(&globGtp.gtp_lock);
  gtpv1uCreateAndSendMsg(instance,
                         ptr[rab_id].outgoing_ip_addr,
                         ptr[rab_id].outgoing_port,
                         ptr[rab_id].teid_outgoing,
                         buffer, length, false, false, false, ptr[rab_id].seqNum, ptr[rab_id].npduNum, 0) ;
}

static  int udpServerSocket(openAddr_s addr) {
  LOG_I(GTPU, "Initializing UDP for local address %s with port %s\n", addr.originHost, addr.originService);
  int status;
  struct addrinfo hints= {0}, *servinfo, *p;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(addr.originHost, addr.originService, &hints, &servinfo)) != 0) {
    LOG_E(GTPU,"getaddrinfo error: %s\n", gai_strerror(status));
    return -1;
  }

  int sockfd=-1;

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1) {
      LOG_W(GTPU,"socket: %s\n", strerror(errno));
      continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      LOG_W(GTPU,"bind: %s\n", strerror(errno));
      continue;
    }

    break; // if we get here, we must have connected successfully
  }

  if (p == NULL) {
    // looped off the end of the list with no successful bind
    LOG_E(GTPU,"failed to bind socket: %s %s \n", addr.originHost, addr.originService);
    return -1;
  }

  freeaddrinfo(servinfo); // all done with this structure
  int sendbuff = 1000*1000*10;
  AssertFatal(0==setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff)),"");
  LOG_D(GTPU,"Created listener for paquets to: %s:%s, send buffer size: %d\n", addr.originHost, addr.originService,sendbuff);
  return sockfd;
}

instance_t ocp_gtpv1Init(openAddr_t context) {
  pthread_mutex_lock(&globGtp.gtp_lock);
  int id=udpServerSocket(context);

  if (id>=0) {
    globGtp.instances[id].addr=context;
    itti_subscribe_event_fd(OCP_GTPV1_U, id);
  } else
    LOG_E(GTPU,"can't create GTP-U instance\n");

  pthread_mutex_unlock(&globGtp.gtp_lock);

  return id;
}

int ocp_gtpv1u_create_s1u_tunnel(instance_t instance,
                                 const gtpv1u_enb_create_tunnel_req_t  *create_tunnel_req,
                                 gtpv1u_enb_create_tunnel_resp_t *create_tunnel_resp) {
  LOG_D(GTPU, "Start create tunnels for RNTI %x, num_tunnels %d, sgw_S1u_teid %d\n",
        create_tunnel_req->rnti,
        create_tunnel_req->num_tunnels,
        create_tunnel_req->sgw_S1u_teid[0]);
  pthread_mutex_lock(&globGtp.gtp_lock);
  auto inst=&globGtp.instances[instance];
  auto it=inst->ue2te_mapping.find(create_tunnel_req->rnti);

  if ( it != inst->ue2te_mapping.end() ) {
    LOG_W(GTPU,"Create a config for a already existing GTP tunnel (rnti %x)\n", create_tunnel_req->rnti);
    inst->ue2te_mapping.erase(it);
  }

  rntiData_t rntiData= {create_tunnel_req->rnti};

  for (int i = 0; i < create_tunnel_req->num_tunnels; i++) {
    ocp_gtpv1u_bearer_t gtpv1u_ue_data;
    uint32_t s1u_teid=gtpv1uNewTeid();

    while ( inst->te2ue_mapping.find(s1u_teid) != inst->te2ue_mapping.end() ) {
      LOG_W(GTPU, "generated a random Teid that exists, re-generating (%u)\n",s1u_teid);
      s1u_teid=gtpv1uNewTeid();
    };

    inst->te2ue_mapping[s1u_teid]=rntiData;

    int addrs_length_in_bytes = create_tunnel_req->sgw_addr[i].length / 8;

    switch (addrs_length_in_bytes) {
      case 4:
        memcpy(&gtpv1u_ue_data.outgoing_ip_addr,create_tunnel_req->sgw_addr[i].buffer,4);
        break;

      case 16:
        memcpy(&gtpv1u_ue_data.outgoing_ip6_addr.s6_addr,
               create_tunnel_req->sgw_addr[i].buffer,
               16);
        break;

      case 20:
        memcpy(&gtpv1u_ue_data.outgoing_ip_addr,create_tunnel_req->sgw_addr[i].buffer,4);
        memcpy(&gtpv1u_ue_data.outgoing_ip6_addr.s6_addr,
               create_tunnel_req->sgw_addr[i].buffer+4,
               16);

      default:
        AssertFatal(false, "SGW Address size impossible");
    }

    gtpv1u_ue_data.teid_incoming = s1u_teid;
    gtpv1u_ue_data.outgoing_port=2152;
    gtpv1u_ue_data.teid_outgoing= create_tunnel_req->sgw_S1u_teid[i];
    inst->ue2te_mapping[create_tunnel_req->rnti].bearers[create_tunnel_req->eps_bearer_id[i]]=gtpv1u_ue_data;
    LOG_D(GTPU, "Created tunnel %x for RNTI %x, SGW: " IPV4_ADDR ":%d\n",
          s1u_teid, create_tunnel_req->rnti,
          IPV4_ADDR_FORMAT(inst->ue2te_mapping[create_tunnel_req->rnti].bearers[create_tunnel_req->eps_bearer_id[i]].outgoing_ip_addr),
          inst->ue2te_mapping[create_tunnel_req->rnti].bearers[create_tunnel_req->eps_bearer_id[i]].outgoing_port );
    create_tunnel_resp->status=0;
    create_tunnel_resp->rnti=create_tunnel_req->rnti;
    create_tunnel_resp->num_tunnels=create_tunnel_req->num_tunnels;
    create_tunnel_resp->enb_S1u_teid[i]=gtpv1u_ue_data.teid_incoming;
    create_tunnel_resp->eps_bearer_id[i] = create_tunnel_req->eps_bearer_id[i];
    //TBD: fix me this is quite bad, for IPv4 only and
    // Overcomplex stuff to fill InitialContextSetupResponse in S1AP
    int status;
    struct addrinfo hints= {0}, *servinfo, *p;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    openAddr_t *addr=&globGtp.instances[instance].addr;

    if ((status = getaddrinfo(addr->originHost, addr->originService, &hints, &servinfo)) != 0) {
      LOG_E(GTPU,"getaddrinfo error: %s\n", gai_strerror(status));
      pthread_mutex_unlock(&globGtp.gtp_lock);
      return -1;
    }

    // TBD: we take the first IPv4 addr, but it should be coded in the right way
    for(p = servinfo; p != NULL; p = p->ai_next)
      if (p->ai_family == AF_INET) {
        struct sockaddr_in *ipv4P=(struct sockaddr_in *)p->ai_addr;
        memcpy(create_tunnel_resp->enb_addr.buffer,
               &ipv4P->sin_addr.s_addr, sizeof(ipv4P->sin_addr.s_addr));
        create_tunnel_resp->enb_addr.length = sizeof(ipv4P->sin_addr.s_addr);
        break;
      }

    if ( p == NULL )
      LOG_E(GTPU,"can't translate IP address string: %s\n",addr->originHost);

    freeaddrinfo(servinfo); // all done with this structure
  }

  pthread_mutex_unlock(&globGtp.gtp_lock);
  return !GTPNOK;
}

int ocp_gtpv1u_update_s1u_tunnel(
  const instance_t                              instance,
  const gtpv1u_enb_create_tunnel_req_t *const  create_tunnel_req,
  const rnti_t                                  prior_rnti
) {
  LOG_D(GTPU, "Start update tunnels for old RNTI %x, new RNTI %x, num_tunnels %d, sgw_S1u_teid %d, eps_bearer_id %d\n",
        prior_rnti,
        create_tunnel_req->rnti,
        create_tunnel_req->num_tunnels,
        create_tunnel_req->sgw_S1u_teid[0],
        create_tunnel_req->eps_bearer_id[0]);
  pthread_mutex_lock(&globGtp.gtp_lock);
  auto inst=&globGtp.instances[instance];

  if ( inst->ue2te_mapping.find(create_tunnel_req->rnti) != inst->ue2te_mapping.end() ) {
    LOG_E(GTPU,"Update a existing GTP tunnel to a already existing tunnel (new rnti %x, old rnti %x)\n", create_tunnel_req->rnti, prior_rnti);
  }

  auto it=inst->ue2te_mapping.find(prior_rnti);

  if ( it != inst->ue2te_mapping.end() ) {
    LOG_W(GTPU,"Update a not existing tunnel, start create the new one (new rnti %x, old rnti %x)\n", create_tunnel_req->rnti, prior_rnti);
    gtpv1u_enb_create_tunnel_resp_t tmp;
    (void)ocp_gtpv1u_create_s1u_tunnel(instance, create_tunnel_req, &tmp);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return 0;
  }

  inst->ue2te_mapping[create_tunnel_req->rnti]=it->second;
  inst->ue2te_mapping.erase(it);
  pthread_mutex_unlock(&globGtp.gtp_lock);
  return 0;
}
  
int ocp_gtpv1u_create_x2u_tunnel(
  const instance_t instanceP,
  const gtpv1u_enb_create_x2u_tunnel_req_t *  const create_tunnel_req_pP,
  gtpv1u_enb_create_x2u_tunnel_resp_t * const create_tunnel_resp_pP){
  AssertFatal( false, "to be developped\n");
}
  
int ocp_gtpv1u_delete_s1u_tunnel( const instance_t instance,
                            const gtpv1u_enb_delete_tunnel_req_t *const req_pP) {
  LOG_D(GTPU, "Start delete tunnels for RNTI %x, num_erab %d, eps_bearer_id %d \n",
        req_pP->rnti,
        req_pP->num_erab,
        req_pP->eps_bearer_id[0]);
  pthread_mutex_lock(&globGtp.gtp_lock);
  auto inst=&globGtp.instances[instance];
  auto it=inst->ue2te_mapping.find(req_pP->rnti);

  if ( it == inst->ue2te_mapping.end() ) {
    LOG_W(GTPU,"Delete a non existing GTP tunnel\n");
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return -1;
  }

  for (auto j=it->second.bearers.begin();
       j!=it->second.bearers.end();
       ++j)
    inst->te2ue_mapping.erase(j->second.teid_incoming);

  inst->ue2te_mapping.erase(it);
  pthread_mutex_unlock(&globGtp.gtp_lock);
  return !GTPNOK;
}

static int Gtpv1uHandleEchoReq(int h,
                               uint8_t *msgBuf,
                               uint32_t msgBufLen,
                               uint16_t peerPort,
                               uint32_t peerIp) {
  LOG_E(GTPU,"to be dev\n");
  int rc = GTPNOK;
  return rc;
}

static int Gtpv1uHandleError(int h,
                             uint8_t *msgBuf,
                             uint32_t msgBufLen,
                             uint16_t peerPort,
                             uint32_t peerIp) {
  LOG_E(GTPU,"to be dev\n");
  int rc = GTPNOK;
  return rc;
}
  
static int Gtpv1uHandleSupportedExt(int h,
                             uint8_t *msgBuf,
                             uint32_t msgBufLen,
                             uint16_t peerPort,
                             uint32_t peerIp) {
  LOG_E(GTPU,"to be dev\n");
  int rc = GTPNOK;
  return rc;
}
  
  static int Gtpv1uHandleEndMarker(int h,
                             uint8_t *msgBuf,
                             uint32_t msgBufLen,
                             uint16_t peerPort,
                             uint32_t peerIp) {
  LOG_E(GTPU,"to be dev\n");
  int rc = GTPNOK;
  return rc;
}
static int Gtpv1uHandleGpdu(int h,
                            uint8_t *msgBuf,
                            uint32_t msgBufLen,
                            uint16_t peerPort,
                            uint32_t peerIp) {
  Gtpv1uMsgHeaderT      *msgHdr = (Gtpv1uMsgHeaderT *) msgBuf;

  if ( msgHdr->version != 1 ||  msgHdr->PT != 1 ) {
    LOG_E(GTPU, "Received a packet that is not GTP header\n");
    return GTPNOK;
  }

  pthread_mutex_lock(&globGtp.gtp_lock);
  // the socket Linux file handler is the instance id
  auto inst=&globGtp.instances[h];
  auto tunnel=inst->te2ue_mapping.find(ntohl(msgHdr->teid));

  if ( tunnel == inst->te2ue_mapping.end() ) {
    LOG_E(GTPU,"Received a incoming packet on non open teid (%d)\n", msgHdr->teid);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return GTPNOK;
  }

  int offset=8;
  if( msgHdr->E ||  msgHdr->S ||msgHdr->PN)
    offset+=4;
  /*
  gtpv1u_enb_tunnel_data_ind_t tmp;
  protocol_ctxt_t ctxt;
      ctxt.module_id = 0;
    ctxt.enb_flag = 1;
    ctxt.instance = 0;
    ctxt.frame = 0;
    ctxt.subframe = 0;
    ctxt.eNB_index = 0;
    ctxt.configured = 1;
    ctxt.brOption = 0;
      result = pdcp_data_req(&ctxt,
                             SRB_FLAG_YES,
                             RRC_DCCH_DATA_REQ(msg_p).rb_id,
                             RRC_DCCH_DATA_REQ(msg_p).muip,
                             RRC_DCCH_DATA_REQ(msg_p).confirmp,
                             RRC_DCCH_DATA_REQ(msg_p).sdu_size,
                             RRC_DCCH_DATA_REQ(msg_p).sdu_p,
                             RRC_DCCH_DATA_REQ(msg_p).mode,
                             NULL, NULL);


    ctxt.rnti = rnti;
    pdcp_data_req(&ctxt, SRB_FLAG_NO, rb_id, RLC_MUI_UNDEFINED,
                  RLC_SDU_CONFIRM_NO, len, (unsigned char *)rx_buf,
                  PDCP_TRANSMISSION_MODE_DATA, NULL, NULL);

  MessageDef *msg_p=itti_alloc_new_message_sized(OCP_GTPV1_U,GTPV1U_ENB_TUNNEL_DATA_IND,
                    sizeof(MessageDef) + msgBufLen);
  GTPV1U_ENB_TUNNEL_DATA_IND(msg_p).buffer=(uint8_t *)(msg_p+1);
  memcpy(GTPV1U_ENB_TUNNEL_DATA_IND(msg_p).buffer,msgBuf,msgBufLen);
  GTPV1U_ENB_TUNNEL_DATA_IND(msg_p).length=msgBufLen;
  GTPV1U_ENB_TUNNEL_DATA_IND(msg_p).offset=offset;
  pthread_mutex_unlock(&globGtp.gtp_lock);
  inst->addr.dlCallBack(msg_p);
  */
  // could be a full itti msg, make it in the call back if you need
  //itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(0), msg_p);
  LOG_D(GTPU,"Received a %d bytes packet for: teid:%x\n",
        msgBufLen-offset,
        ntohl(msgHdr->teid));
  return !GTPNOK;
}

void gtpv1uReceiver(int h) {
  uint8_t                   udpData[65536];
  int               udpDataLen;
  socklen_t          from_len;
  struct sockaddr_in addr;
  from_len = (socklen_t)sizeof(struct sockaddr_in);

  if ((udpDataLen = recvfrom(h, udpData, sizeof(udpData), 0,
                             (struct sockaddr *)&addr, &from_len)) < 0) {
    LOG_E(GTPU, "Recvfrom failed %s\n", strerror(errno));
    return;
  } else if (udpDataLen == 0) {
    LOG_W(GTPU, "Recvfrom returned 0\n");
    return;
  } else {
    uint8_t msgType = *((uint8_t *)(udpData + 1));
    LOG_D(GTPU, "Received GTP data, msg type: %x\n", msgType);

    switch(msgType) {
      case GTP_ECHO_RSP:
        break;

      case GTP_ECHO_REQ:
        Gtpv1uHandleEchoReq( h, udpData, udpDataLen, htons(addr.sin_port), addr.sin_addr.s_addr);
        break;

      case GTP_ERROR_INDICATION:
        Gtpv1uHandleError( h, udpData, udpDataLen, htons(addr.sin_port), addr.sin_addr.s_addr);
        break;
	
    case GTP_SUPPORTED_EXTENSION_HEADER_INDICATION:
        Gtpv1uHandleSupportedExt( h, udpData, udpDataLen, htons(addr.sin_port), addr.sin_addr.s_addr);
	break;
	
    case GTP_END_MARKER:
      Gtpv1uHandleEndMarker( h, udpData, udpDataLen, htons(addr.sin_port), addr.sin_addr.s_addr);
      break;
      
    case GTP_GPDU:
      Gtpv1uHandleGpdu( h, udpData, udpDataLen, htons(addr.sin_port), addr.sin_addr.s_addr);
      break;
      
    default:
      LOG_E(GTPU, "Received a GTP packet of unknown type: %d\n",msgType);
      break;
    }
  }
}

void *ocp_gtpv1uTask(void *args)  {
  while(1) {
    /* Trying to fetch a message from the message queue.
       If the queue is empty, this function will block till a
       message is sent to the task.
    */
    MessageDef *message_p = NULL;
    itti_receive_msg(OCP_GTPV1_U, &message_p);

    if (message_p != NULL ) {
      switch (ITTI_MSG_ID(message_p)) {
        // DATA TO BE SENT TO UDP
        case GTPV1U_ENB_TUNNEL_DATA_REQ: {
          gtpv1uSend(ITTI_MESSAGE_GET_INSTANCE(message_p),
                     &GTPV1U_ENB_TUNNEL_DATA_REQ(message_p), false, false);
          itti_free(OCP_GTPV1_U, GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).buffer);
        }
        break;

        case TERMINATE_MESSAGE:
          break;

        case TIMER_HAS_EXPIRED:
          LOG_E(GTPU, "Received unexpected timer expired (no need of timers in this version) %s\n", ITTI_MSG_NAME(message_p));
          break;

	  
      case GTPV1U_ENB_DATA_FORWARDING_REQ:
      case GTPV1U_ENB_DATA_FORWARDING_IND:
      case GTPV1U_ENB_END_MARKER_REQ:
      case GTPV1U_ENB_END_MARKER_IND:
      case GTPV1U_ENB_S1_REQ:
	LOG_E(GTPU, "to be developped %s\n", ITTI_MSG_NAME(message_p));
	abort();
	break;
	  
      default:
	LOG_E(GTPU, "Received unexpected message %s\n", ITTI_MSG_NAME(message_p));
	  abort();
          break;
      }

      AssertFatal(EXIT_SUCCESS==itti_free(OCP_GTPV1_U, message_p), "Failed to free memory!\n");
    }

    struct epoll_event *events;

    int nb_events = itti_get_events(OCP_GTPV1_U, &events);

    if (nb_events > 0 && events!= NULL )
      for (int i = 0; i < nb_events; i++)
        gtpv1uReceiver(events[i].data.fd);
  }

  return NULL;
}

#ifdef __cplusplus
}
#endif
