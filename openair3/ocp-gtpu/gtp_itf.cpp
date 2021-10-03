#include <map>
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif
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
#include <openair2/LAYER2/PDCP_v10.1.0/pdcp.h>
//#include <openair1/PHY/phy_extern.h>

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
  teid_t teid;
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
  ebi_t incoming_rb_id;
  gtpCallback callBack;
} rntiData_t;

class gtpEndPoint {
 public:
  openAddr_t addr;
  uint8_t foundAddr[20];
  int foundAddrLen;
  int ipVersion;
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

// note TEid 0 is reserved for specific usage: echo req/resp, error and supported extensions
static  teid_t gtpv1uNewTeid(void) {
#ifdef GTPV1U_LINEAR_TEID_ALLOCATION
  g_gtpv1u_teid = g_gtpv1u_teid + 1;
  return g_gtpv1u_teid;
#else
  return random() + random() % (RAND_MAX - 1) + 1;
#endif
}

instance_t legacyInstanceMapping=0;
#define compatInst(a) ((a)==0 || (a)==INSTANCE_DEFAULT?legacyInstanceMapping:a)

#define GTPV1U_HEADER_SIZE                                  (8)
static  int gtpv1uCreateAndSendMsg(int h, uint32_t peerIp, uint16_t peerPort, teid_t teid, uint8_t *Msg,int msgLen,
                                   bool seqNumFlag, bool  npduNumFlag, bool extHdrFlag, int seqNum, int npduNum, int extHdrType) {
  AssertFatal(extHdrFlag==false,"Not developped");
  int headerAdditional=0;

  if ( seqNumFlag || npduNumFlag || extHdrFlag)
    headerAdditional=4;

  uint8_t *buffer;
  int fullSize=GTPV1U_HEADER_SIZE+headerAdditional+msgLen;
  AssertFatal((buffer=(uint8_t *) malloc(fullSize)) != NULL, "");
  Gtpv1uMsgHeaderT      *msgHdr = (Gtpv1uMsgHeaderT *)buffer ;
  // N should be 0 for us (it was used only in 2G and 3G)
  msgHdr->PN=npduNumFlag;
  msgHdr->S=seqNumFlag;
  msgHdr->E=extHdrFlag;
  msgHdr->spare=0;
  //PT=0 is for GTP' TS 32.295 (charging)
  msgHdr->PT=1;
  msgHdr->version=1;
  msgHdr->msgType=GTP_GPDU;
  msgHdr->msgLength=htons(msgLen);

  if ( seqNumFlag || extHdrFlag || npduNumFlag)
    msgHdr->msgLength+=4;

  msgHdr->teid=htonl(teid);

  if(seqNumFlag || extHdrFlag || npduNumFlag) {
    *((uint16_t *) (buffer+8)) = seqNumFlag ? htons(seqNum) : 0x0000;
    *((uint8_t *) (buffer+10)) = npduNumFlag ? htons(npduNum) : 0x00;
    *((uint8_t *) (buffer+11)) = extHdrFlag ? htons(extHdrType) : 0x00;
  }

  memcpy(buffer+GTPV1U_HEADER_SIZE+headerAdditional, Msg, msgLen);
  // Fix me: add IPv6 support, using flag ipVersion
  static struct sockaddr_in to= {0};
  to.sin_family      = AF_INET;
  to.sin_port        = htons(peerPort);
  to.sin_addr.s_addr = peerIp ;
  LOG_D(GTPU,"sending packet size: %d to %s\n",fullSize, inet_ntoa(to.sin_addr) );
  int ret;

  if ((ret=sendto(h, (void *)buffer, (size_t)fullSize, 0,(struct sockaddr *)&to, sizeof(to) )) != fullSize ) {
    LOG_E(GTPU, "[SD %d] Failed to send data to " IPV4_ADDR " on port %d, buffer size %u, ret: %d, errno: %d\n",
          h, IPV4_ADDR_FORMAT(peerIp), peerPort, fullSize, ret, errno);
    free(buffer);
    return GTPNOK;
  }

  free(buffer);
  return  !GTPNOK;
}

static void gtpv1uSend(instance_t instance, gtpv1u_enb_tunnel_data_req_t *req, bool seqNumFlag, bool npduNumFlag) {
  uint8_t *buffer=req->buffer+req->offset;
  size_t length=req->length;
  rnti_t rnti=req->rnti;
  int  rab_id=req->rab_id;
  pthread_mutex_lock(&globGtp.gtp_lock);
  auto inst=&globGtp.instances[compatInst(instance)];
  auto ptrRnti=inst->ue2te_mapping.find(rnti);

  if (  ptrRnti==inst->ue2te_mapping.end() ) {
    LOG_E(GTPU, "[%ld] gtpv1uSend failed: while getting ue rnti %x in hashtable ue_mapping\n", instance, rnti);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return;
  }

  map<int, ocp_gtpv1u_bearer_t>::iterator ptr2=ptrRnti->second.bearers.find(rab_id);

  if ( ptr2 == ptrRnti->second.bearers.end() ) {
    LOG_E(GTPU,"[%ld] GTP-U instance: sending a packet to a non existant RNTI:RAB: %x/%x\n", instance, rnti, rab_id);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return;
  }

  LOG_D(GTPU,"[%ld] sending a packet to RNTI:RAB:teid %x/%x/%x, len %lu, oldseq %d, oldnum %d\n",
        instance, rnti, rab_id,ptr2->second.teid_outgoing,length, ptr2->second.seqNum,ptr2->second.npduNum );

  if(seqNumFlag)
    ptr2->second.seqNum++;

  if(npduNumFlag)
    ptr2->second.npduNum++;

  // copy to release the mutex
  ocp_gtpv1u_bearer_t tmp=ptr2->second;
  pthread_mutex_unlock(&globGtp.gtp_lock);
  gtpv1uCreateAndSendMsg(compatInst(instance),
                         tmp.outgoing_ip_addr,
                         tmp.outgoing_port,
                         tmp.teid_outgoing,
                         buffer, length, seqNumFlag, npduNumFlag, false, tmp.seqNum, tmp.npduNum, 0) ;
}

static void gtpv1uSend2(instance_t instance, gtpv1u_gnb_tunnel_data_req_t *req, bool seqNumFlag, bool npduNumFlag) {
  uint8_t *buffer=req->buffer+req->offset;
  size_t length=req->length;
  rnti_t rnti=req->rnti;
  int  rab_id=req->pdusession_id;
  pthread_mutex_lock(&globGtp.gtp_lock);
  auto inst=&globGtp.instances[compatInst(instance)];
  auto ptrRnti=inst->ue2te_mapping.find(rnti);

  if (  ptrRnti==inst->ue2te_mapping.end() ) {
    LOG_E(GTPU, "[%ld] GTP-U gtpv1uSend failed: while getting ue rnti %x in hashtable ue_mapping\n", instance, rnti);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return;
  }

  map<int, ocp_gtpv1u_bearer_t>::iterator ptr2=ptrRnti->second.bearers.find(rab_id);

  if ( ptr2 == ptrRnti->second.bearers.end() ) {
    LOG_D(GTPU,"GTP-U instance: %ld sending a packet to a non existant RNTI:RAB: %x/%x\n", instance, rnti, rab_id);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return;
  }

  LOG_D(GTPU,"[%ld] GTP-U sending a packet to RNTI:RAB:teid %x/%x/%x, len %lu, oldseq %d, oldnum %d\n",
        instance, rnti, rab_id,ptr2->second.teid_outgoing,length, ptr2->second.seqNum,ptr2->second.npduNum );

  if(seqNumFlag)
    ptr2->second.seqNum++;

  if(npduNumFlag)
    ptr2->second.npduNum++;

  // copy to release the mutex
  ocp_gtpv1u_bearer_t tmp=ptr2->second;
  pthread_mutex_unlock(&globGtp.gtp_lock);
  gtpv1uCreateAndSendMsg(compatInst(instance),
                         tmp.outgoing_ip_addr,
                         tmp.outgoing_port,
                         tmp.teid_outgoing,
                         buffer, length, seqNumFlag, npduNumFlag, false, tmp.seqNum, tmp.npduNum, 0) ;
}

static void gtpv1uEndTunnel(instance_t instance, gtpv1u_enb_tunnel_data_req_t *req) {
  rnti_t rnti=req->rnti;
  int  rab_id=req->rab_id;
  pthread_mutex_lock(&globGtp.gtp_lock);
  auto inst=&globGtp.instances[compatInst(instance)];
  auto ptrRnti=inst->ue2te_mapping.find(rnti);

  if (  ptrRnti==inst->ue2te_mapping.end() ) {
    LOG_E(GTPU, "[%ld] gtpv1uSend failed: while getting ue rnti %x in hashtable ue_mapping\n", instance, rnti);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return;
  }

  map<int, ocp_gtpv1u_bearer_t>::iterator ptr2=ptrRnti->second.bearers.find(rab_id);

  if ( ptr2 == ptrRnti->second.bearers.end() ) {
    LOG_E(GTPU,"[%ld] GTP-U sending a packet to a non existant RNTI:RAB: %x/%x\n", instance, rnti, rab_id);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return;
  }

  LOG_D(GTPU,"[%ld] sending a end packet packet to RNTI:RAB:teid %x/%x/%x\n",
        instance, rnti, rab_id,ptr2->second.teid_outgoing);
  ocp_gtpv1u_bearer_t tmp=ptr2->second;
  pthread_mutex_unlock(&globGtp.gtp_lock);
  Gtpv1uMsgHeaderT  msgHdr;
  // N should be 0 for us (it was used only in 2G and 3G)
  msgHdr.PN=0;
  msgHdr.S=0;
  msgHdr.E=0;
  msgHdr.spare=0;
  //PT=0 is for GTP' TS 32.295 (charging)
  msgHdr.PT=1;
  msgHdr.version=1;
  msgHdr.msgType=GTP_END_MARKER;
  msgHdr.msgLength=htons(0);
  msgHdr.teid=htonl(tmp.teid_outgoing);
  // Fix me: add IPv6 support, using flag ipVersion
  static struct sockaddr_in to= {0};
  to.sin_family      = AF_INET;
  to.sin_port        = htons(tmp.outgoing_port);
  to.sin_addr.s_addr = tmp.outgoing_ip_addr;
  char ip4[INET_ADDRSTRLEN];
  //char ip6[INET6_ADDRSTRLEN];
  LOG_D(GTPU,"[%ld] sending end packet to %s\n", instance, inet_ntoa(to.sin_addr) );

  if (sendto(compatInst(instance), (void *)&msgHdr, sizeof(msgHdr), 0,(struct sockaddr *)&to, sizeof(to) ) !=  sizeof(msgHdr)) {
    LOG_E(GTPU,
          "[%ld] Failed to send data to %s on port %d, buffer size %lu\n",
          compatInst(instance), inet_ntop(AF_INET, &tmp.outgoing_ip_addr, ip4, INET_ADDRSTRLEN), tmp.outgoing_port, sizeof(msgHdr));
  }
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
    } else {
      // We create the gtp instance on the socket
      globGtp.instances[sockfd].addr=addr;

      if (p->ai_family == AF_INET) {
        struct sockaddr_in *ipv4=(struct sockaddr_in *)p->ai_addr;
        memcpy(globGtp.instances[sockfd].foundAddr,
               &ipv4->sin_addr.s_addr, sizeof(ipv4->sin_addr.s_addr));
        globGtp.instances[sockfd].foundAddrLen=sizeof(ipv4->sin_addr.s_addr);
        globGtp.instances[sockfd].ipVersion=4;
        break;
      } else if (p->ai_family == AF_INET6) {
        LOG_W(GTPU,"Local address is IP v6\n");
        struct sockaddr_in6 *ipv6=(struct sockaddr_in6 *)p->ai_addr;
        memcpy(globGtp.instances[sockfd].foundAddr,
               &ipv6->sin6_addr.s6_addr, sizeof(ipv6->sin6_addr.s6_addr));
        globGtp.instances[sockfd].foundAddrLen=sizeof(ipv6->sin6_addr.s6_addr);
        globGtp.instances[sockfd].ipVersion=6;
      } else
        AssertFatal(false,"Local address is not IPv4 or IPv6");
    }

    break; // if we get here, we must have connected successfully
  }

  if (p == NULL) {
    // looped off the end of the list with no successful bind
    LOG_E(GTPU,"failed to bind socket: %s %s \n", addr.originHost, addr.originService);
    return -1;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (strlen(addr.destinationHost)>1) {
    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_PASSIVE|AI_ADDRCONFIG;
    struct addrinfo *res=0;
    int err=getaddrinfo(addr.destinationHost,addr.destinationService,&hints,&res);

    if (err==0) {
      for(p = res; p != NULL; p = p->ai_next) {
        if ((err=connect(sockfd,  p->ai_addr, p->ai_addrlen))==0)
          break;
      }
    }

    if (err)
      LOG_E(GTPU,"Can't filter remote host: %s, %s\n", addr.destinationHost,addr.destinationService);
  }

  int sendbuff = 1000*1000*10;
  AssertFatal(0==setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff)),"");
  LOG_D(GTPU,"[%d] Created listener for paquets to: %s:%s, send buffer size: %d\n", sockfd, addr.originHost, addr.originService,sendbuff);
  return sockfd;
}

instance_t ocp_gtpv1Init(openAddr_t context) {
  pthread_mutex_lock(&globGtp.gtp_lock);
  int id=udpServerSocket(context);

  if (id>=0) {
    itti_subscribe_event_fd(OCP_GTPV1_U, id);
  } else
    LOG_E(GTPU,"can't create GTP-U instance\n");

  pthread_mutex_unlock(&globGtp.gtp_lock);
  LOG_I(GTPU, "Created gtpu instance id: %d\n", id);
  return id;
}

void GtpuUpdateTunnelOutgoingTeid(instance_t instance, rnti_t rnti, ebi_t bearer_id, teid_t newOutgoingTeid) {
  pthread_mutex_lock(&globGtp.gtp_lock);
  auto inst=&globGtp.instances[compatInst(instance)];
  auto ptrRnti=inst->ue2te_mapping.find(rnti);

  if ( ptrRnti == inst->ue2te_mapping.end() ) {
    LOG_E(GTPU,"[%ld] Update tunnel for a not existing rnti %x\n", instance, rnti);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return;
  }

  map<int, ocp_gtpv1u_bearer_t>::iterator ptr2=ptrRnti->second.bearers.find(bearer_id);

  if ( ptr2 == ptrRnti->second.bearers.end() ) {
    LOG_E(GTPU,"[%ld] Update tunnel for a existing rnti %x, but wrong bearer_id %u\n", instance, rnti, bearer_id);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return;
  }

  ptr2->second.teid_outgoing = newOutgoingTeid;
  LOG_I(GTPU, "[%ld] Tunnel Outgoing TEID updated to %x \n", instance, ptr2->second.teid_outgoing);
  pthread_mutex_unlock(&globGtp.gtp_lock);
  return;
}

teid_t newGtpuCreateTunnel(instance_t instance, rnti_t rnti, int incoming_bearer_id, int outgoing_bearer_id, teid_t outgoing_teid,
                           transport_layer_addr_t remoteAddr, int port, gtpCallback callBack) {
  pthread_mutex_lock(&globGtp.gtp_lock);
  instance=compatInst(instance);
  auto inst=&globGtp.instances[instance];
  auto it=inst->ue2te_mapping.find(rnti);

  if ( it != inst->ue2te_mapping.end() ) {
    LOG_W(GTPU,"[%ld] Create a config for a already existing GTP tunnel (rnti %x)\n", instance, rnti);
    inst->ue2te_mapping.erase(it);
  }

  teid_t incoming_teid=gtpv1uNewTeid();

  while ( inst->te2ue_mapping.find(incoming_teid) != inst->te2ue_mapping.end() ) {
    LOG_W(GTPU, "[%ld] generated a random Teid that exists, re-generating (%x)\n", instance, incoming_teid);
    incoming_teid=gtpv1uNewTeid();
  };

  inst->te2ue_mapping[incoming_teid].rnti=rnti;

  inst->te2ue_mapping[incoming_teid].incoming_rb_id= incoming_bearer_id;

  inst->te2ue_mapping[incoming_teid].callBack=callBack;

  ocp_gtpv1u_bearer_t *tmp=&inst->ue2te_mapping[rnti].bearers[outgoing_bearer_id];

  int addrs_length_in_bytes = remoteAddr.length / 8;

  switch (addrs_length_in_bytes) {
    case 4:
      memcpy(&tmp->outgoing_ip_addr,remoteAddr.buffer,4);
      break;

    case 16:
      memcpy(tmp->outgoing_ip6_addr.s6_addr,remoteAddr.buffer,
             16);
      break;

    case 20:
      memcpy(&tmp->outgoing_ip_addr,remoteAddr.buffer,4);
      memcpy(&tmp->outgoing_ip6_addr.s6_addr,
             remoteAddr.buffer+4,
             16);

    default:
      AssertFatal(false, "SGW Address size impossible");
  }

  tmp->teid_incoming = incoming_teid;
  tmp->outgoing_port=port;
  tmp->teid_outgoing= outgoing_teid;
  pthread_mutex_unlock(&globGtp.gtp_lock);
  char ip4[INET_ADDRSTRLEN];
  char ip6[INET6_ADDRSTRLEN];
  LOG_I(GTPU, "[%ld] Created tunnel for RNTI %x, teid for DL: %x, teid for UL %x to remote IPv4: %s, IPv6 %s\n",
        instance,
        rnti,
        tmp->teid_incoming,
        tmp->teid_outgoing,
        inet_ntop(AF_INET,(void *)&tmp->outgoing_ip_addr, ip4,INET_ADDRSTRLEN ),
        inet_ntop(AF_INET6,(void *)&tmp->outgoing_ip6_addr.s6_addr, ip6, INET6_ADDRSTRLEN));
  return incoming_teid;
}

int ocp_gtpv1u_create_s1u_tunnel(instance_t instance,
                                 const gtpv1u_enb_create_tunnel_req_t  *create_tunnel_req,
                                 gtpv1u_enb_create_tunnel_resp_t *create_tunnel_resp) {
  LOG_D(GTPU, "[%ld] Start create tunnels for RNTI %x, num_tunnels %d, sgw_S1u_teid %x\n",
        instance,
        create_tunnel_req->rnti,
        create_tunnel_req->num_tunnels,
        create_tunnel_req->sgw_S1u_teid[0]);

  for (int i = 0; i < create_tunnel_req->num_tunnels; i++) {
    AssertFatal(create_tunnel_req->eps_bearer_id[i] > 4,
                "From legacy code not clear, seems impossible (bearer=%d)\n",
                create_tunnel_req->eps_bearer_id[i]);
    int incoming_rb_id=create_tunnel_req->eps_bearer_id[i]-4;
    teid_t teid=newGtpuCreateTunnel(instance, create_tunnel_req->rnti,
                                    incoming_rb_id,
                                    create_tunnel_req->eps_bearer_id[i],
                                    create_tunnel_req->sgw_S1u_teid[i],
                                    create_tunnel_req->sgw_addr[i], 2152,
                                    pdcp_data_req);
    create_tunnel_resp->status=0;
    create_tunnel_resp->rnti=create_tunnel_req->rnti;
    create_tunnel_resp->num_tunnels=create_tunnel_req->num_tunnels;
    create_tunnel_resp->enb_S1u_teid[i]=teid;
    create_tunnel_resp->eps_bearer_id[i] = create_tunnel_req->eps_bearer_id[i];
    memcpy(create_tunnel_resp->enb_addr.buffer,globGtp.instances[compatInst(instance)].foundAddr,
           globGtp.instances[compatInst(instance)].foundAddrLen);
    create_tunnel_resp->enb_addr.length= globGtp.instances[compatInst(instance)].foundAddrLen;
  }

  return !GTPNOK;
}

int ocp_gtpv1u_update_s1u_tunnel(
  const instance_t                              instance,
  const gtpv1u_enb_create_tunnel_req_t *const  create_tunnel_req,
  const rnti_t                                  prior_rnti
) {
  LOG_D(GTPU, "[%ld] Start update tunnels for old RNTI %x, new RNTI %x, num_tunnels %d, sgw_S1u_teid %x, eps_bearer_id %x\n",
        instance,
        prior_rnti,
        create_tunnel_req->rnti,
        create_tunnel_req->num_tunnels,
        create_tunnel_req->sgw_S1u_teid[0],
        create_tunnel_req->eps_bearer_id[0]);
  pthread_mutex_lock(&globGtp.gtp_lock);
  auto inst=&globGtp.instances[compatInst(instance)];

  if ( inst->ue2te_mapping.find(create_tunnel_req->rnti) == inst->ue2te_mapping.end() ) {
    LOG_E(GTPU,"[%ld] Update not already existing tunnel (new rnti %x, old rnti %x)\n", instance, create_tunnel_req->rnti, prior_rnti);
  }

  auto it=inst->ue2te_mapping.find(prior_rnti);

  if ( it != inst->ue2te_mapping.end() ) {
    LOG_W(GTPU,"[%ld] Update a not existing tunnel, start create the new one (new rnti %x, old rnti %x)\n", instance, create_tunnel_req->rnti, prior_rnti);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    gtpv1u_enb_create_tunnel_resp_t tmp;
    (void)ocp_gtpv1u_create_s1u_tunnel(instance, create_tunnel_req, &tmp);
    return 0;
  }

  inst->ue2te_mapping[create_tunnel_req->rnti]=it->second;
  inst->ue2te_mapping.erase(it);
  pthread_mutex_unlock(&globGtp.gtp_lock);
  return 0;
}

int gtpv1u_create_ngu_tunnel(  const instance_t instance,
                               const gtpv1u_gnb_create_tunnel_req_t   *const create_tunnel_req,
                               gtpv1u_gnb_create_tunnel_resp_t *const create_tunnel_resp) {
  LOG_D(GTPU, "[%ld] Start create tunnels for RNTI %x, num_tunnels %d, sgw_S1u_teid %x\n",
        instance,
        create_tunnel_req->rnti,
        create_tunnel_req->num_tunnels,
        create_tunnel_req->outgoing_teid[0]);

  for (int i = 0; i < create_tunnel_req->num_tunnels; i++) {
    teid_t teid=newGtpuCreateTunnel(instance, create_tunnel_req->rnti,
                                    create_tunnel_req->incoming_rb_id[i],
                                    create_tunnel_req->pdusession_id[i],
                                    create_tunnel_req->outgoing_teid[i],
                                    create_tunnel_req->dst_addr[i], 2152,
                                    pdcp_data_req);
    create_tunnel_resp->status=0;
    create_tunnel_resp->rnti=create_tunnel_req->rnti;
    create_tunnel_resp->num_tunnels=create_tunnel_req->num_tunnels;
    create_tunnel_resp->gnb_NGu_teid[i]=teid;
    memcpy(create_tunnel_resp->gnb_addr.buffer,globGtp.instances[compatInst(instance)].foundAddr,
           globGtp.instances[compatInst(instance)].foundAddrLen);
    create_tunnel_resp->gnb_addr.length= globGtp.instances[compatInst(instance)].foundAddrLen;
  }

  return !GTPNOK;
}

int gtpv1u_update_ngu_tunnel(
  const instance_t instanceP,
  const gtpv1u_gnb_create_tunnel_req_t *const  create_tunnel_req_pP,
  const rnti_t prior_rnti
) {
  AssertFatal( false, "to be developped\n");
  return GTPNOK;
}

int ocp_gtpv1u_create_x2u_tunnel(
  const instance_t instanceP,
  const gtpv1u_enb_create_x2u_tunnel_req_t   *const create_tunnel_req_pP,
  gtpv1u_enb_create_x2u_tunnel_resp_t *const create_tunnel_resp_pP) {
  AssertFatal( false, "to be developped\n");
}

int newGtpuDeleteTunnel(instance_t instance, rnti_t rnti) {
  LOG_D(GTPU, "[%ld] Start delete tunnels for RNTI %x\n",
        instance, rnti);
  pthread_mutex_lock(&globGtp.gtp_lock);
  auto inst=&globGtp.instances[compatInst(instance)];
  auto it=inst->ue2te_mapping.find(rnti);

  if ( it == inst->ue2te_mapping.end() ) {
    LOG_W(GTPU,"[%ld] Delete GTP tunnels for rnti: %x, but no tunnel exits\n", instance, rnti);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return -1;
  }

  int nb=0;

  for (auto j=it->second.bearers.begin();
       j!=it->second.bearers.end();
       ++j) {
    inst->te2ue_mapping.erase(j->second.teid_incoming);
    nb++;
  }

  inst->ue2te_mapping.erase(it);
  pthread_mutex_unlock(&globGtp.gtp_lock);
  LOG_I(GTPU, "[%ld] Deleted all tunnels for RNTI %x (%d tunnels deleted)\n", instance, rnti, nb);
  return !GTPNOK;
}

// Legacy delete tunnel finish by deleting all the rnti
// so the list of bearer provided is only a design bug
int ocp_gtpv1u_delete_s1u_tunnel( const instance_t instance,
                                  const gtpv1u_enb_delete_tunnel_req_t *const req_pP) {
  return  newGtpuDeleteTunnel(instance, req_pP->rnti);
}

int gtpv1u_delete_x2u_tunnel( const instance_t instanceP,
                              const gtpv1u_enb_delete_tunnel_req_t *const req_pP,
                              int enbflag) {
  return 0;
}

static int Gtpv1uHandleEchoReq(int h,
                               uint8_t *msgBuf,
                               uint32_t msgBufLen,
                               uint16_t peerPort,
                               uint32_t peerIp) {
  LOG_E(GTPU,"EchoReq reply to be dev\n");
  int rc = GTPNOK;
  return rc;
}

static int Gtpv1uHandleError(int h,
                             uint8_t *msgBuf,
                             uint32_t msgBufLen,
                             uint16_t peerPort,
                             uint32_t peerIp) {
  LOG_E(GTPU,"Hadle error to be dev\n");
  int rc = GTPNOK;
  return rc;
}

static int Gtpv1uHandleSupportedExt(int h,
                                    uint8_t *msgBuf,
                                    uint32_t msgBufLen,
                                    uint16_t peerPort,
                                    uint32_t peerIp) {
  LOG_E(GTPU,"Supported extensions to be dev\n");
  int rc = GTPNOK;
  return rc;
}

// When end marker arrives, we notify the client with buffer size = 0
// The client will likely call "delete tunnel"
// nevertheless we don't take the initiative
static int Gtpv1uHandleEndMarker(int h,
                                 uint8_t *msgBuf,
                                 uint32_t msgBufLen,
                                 uint16_t peerPort,
                                 uint32_t peerIp) {
  Gtpv1uMsgHeaderT      *msgHdr = (Gtpv1uMsgHeaderT *) msgBuf;

  if ( msgHdr->version != 1 ||  msgHdr->PT != 1 ) {
    LOG_E(GTPU, "[%d] Received a packet that is not GTP header\n", h);
    return GTPNOK;
  }

  pthread_mutex_lock(&globGtp.gtp_lock);
  // the socket Linux file handler is the instance id
  auto inst=&globGtp.instances[h];
  auto tunnel=inst->te2ue_mapping.find(ntohl(msgHdr->teid));

  if ( tunnel == inst->te2ue_mapping.end() ) {
    LOG_E(GTPU,"[%d] Received a incoming packet on unknown teid (%x) Dropping!\n", h, msgHdr->teid);
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return GTPNOK;
  }

  // This context is not good for gtp
  // frame, ... has no meaning
  // manyother attributes may come from create tunnel
  protocol_ctxt_t ctxt;
  ctxt.module_id = 0;
  ctxt.enb_flag = 1;
  ctxt.instance = inst->addr.originInstance;
  ctxt.rnti = tunnel->second.rnti;
  ctxt.frame = 0;
  ctxt.subframe = 0;
  ctxt.eNB_index = 0;
  ctxt.brOption = 0;
  const srb_flag_t     srb_flag=SRB_FLAG_NO;
  const rb_id_t        rb_id=tunnel->second.incoming_rb_id;
  const mui_t          mui=RLC_MUI_UNDEFINED;
  const confirm_t      confirm=RLC_SDU_CONFIRM_NO;
  const pdcp_transmission_mode_t mode=PDCP_TRANSMISSION_MODE_DATA;
  const uint32_t sourceL2Id=0;
  const uint32_t destinationL2Id=0;
  pthread_mutex_unlock(&globGtp.gtp_lock);

  if ( !tunnel->second.callBack(&ctxt,
                                srb_flag,
                                rb_id,
                                mui,
                                confirm,
                                0,
                                NULL,
                                mode,
                                &sourceL2Id,
                                &destinationL2Id) )
    LOG_E(GTPU,"[%d] down layer refused incoming packet\n", h);

  LOG_D(GTPU,"[%d] Received END marker packet for: teid:%x\n", h, ntohl(msgHdr->teid));
  return !GTPNOK;
}

static int Gtpv1uHandleGpdu(int h,
                            uint8_t *msgBuf,
                            uint32_t msgBufLen,
                            uint16_t peerPort,
                            uint32_t peerIp) {
  Gtpv1uMsgHeaderT      *msgHdr = (Gtpv1uMsgHeaderT *) msgBuf;

  if ( msgHdr->version != 1 ||  msgHdr->PT != 1 ) {
    LOG_E(GTPU, "[%d] Received a packet that is not GTP header\n", h);
    return GTPNOK;
  }

  pthread_mutex_lock(&globGtp.gtp_lock);
  // the socket Linux file handler is the instance id
  auto inst=&globGtp.instances[h];
  auto tunnel=inst->te2ue_mapping.find(ntohl(msgHdr->teid));

  if ( tunnel == inst->te2ue_mapping.end() ) {
    LOG_E(GTPU,"[%d] Received a incoming packet on unknown teid (%x) Dropping!\n", h, ntohl(msgHdr->teid));
    pthread_mutex_unlock(&globGtp.gtp_lock);
    return GTPNOK;
  }

  int offset=8;

  if( msgHdr->E ||  msgHdr->S ||msgHdr->PN)
    offset+=8;

  // This context is not good for gtp
  // frame, ... has no meaning
  // manyother attributes may come from create tunnel
  protocol_ctxt_t ctxt;
  ctxt.module_id = 0;
  ctxt.enb_flag = 1;
  ctxt.instance = inst->addr.originInstance;
  ctxt.rnti = tunnel->second.rnti;
  ctxt.frame = 0;
  ctxt.subframe = 0;
  ctxt.eNB_index = 0;
  ctxt.brOption = 0;
  const srb_flag_t     srb_flag=SRB_FLAG_NO;
  const rb_id_t        rb_id=tunnel->second.incoming_rb_id;
  const mui_t          mui=RLC_MUI_UNDEFINED;
  const confirm_t      confirm=RLC_SDU_CONFIRM_NO;
  const sdu_size_t     sdu_buffer_size=msgBufLen-offset;
  unsigned char *const sdu_buffer=msgBuf+offset;
  const pdcp_transmission_mode_t mode=PDCP_TRANSMISSION_MODE_DATA;
  const uint32_t sourceL2Id=0;
  const uint32_t destinationL2Id=0;
  pthread_mutex_unlock(&globGtp.gtp_lock);

  if ( !tunnel->second.callBack(&ctxt,
                                srb_flag,
                                rb_id,
                                mui,
                                confirm,
                                sdu_buffer_size,
                                sdu_buffer,
                                mode,
                                &sourceL2Id,
                                &destinationL2Id) )
    LOG_E(GTPU,"[%d] down layer refused incoming packet\n", h);

  LOG_D(GTPU,"[%d] Received a %d bytes packet for: teid:%x\n", h,
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
    LOG_E(GTPU, "[%d] Recvfrom failed (%s)\n", h, strerror(errno));
    return;
  } else if (udpDataLen == 0) {
    LOG_W(GTPU, "[%d] Recvfrom returned 0\n", h);
    return;
  } else {
    uint8_t msgType = *((uint8_t *)(udpData + 1));
    LOG_D(GTPU, "[%d] Received GTP data, msg type: %x\n", h, msgType);

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
        LOG_E(GTPU, "[%d] Received a GTP packet of unknown type: %d\n", h, msgType);
        break;
    }
  }
}

#include <openair2/ENB_APP/enb_paramdef.h>

void *ocp_gtpv1uTask(void *args)  {
  while(1) {
    /* Trying to fetch a message from the message queue.
       If the queue is empty, this function will block till a
       message is sent to the task.
    */
    MessageDef *message_p = NULL;
    itti_receive_msg(OCP_GTPV1_U, &message_p);

    if (message_p != NULL ) {
      openAddr_t addr= {0};

      switch (ITTI_MSG_ID(message_p)) {
        // DATA TO BE SENT TO UDP
        case GTPV1U_ENB_TUNNEL_DATA_REQ: {
          gtpv1uSend(compatInst(ITTI_MSG_DESTINATION_INSTANCE(message_p)),
                     &GTPV1U_ENB_TUNNEL_DATA_REQ(message_p), false, false);
          itti_free(OCP_GTPV1_U, GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).buffer);
        }
        break;

        case GTPV1U_GNB_TUNNEL_DATA_REQ: {
          gtpv1uSend2(compatInst(ITTI_MSG_DESTINATION_INSTANCE(message_p)),
                      &GTPV1U_GNB_TUNNEL_DATA_REQ(message_p), false, false);
        }
        break;

        case TERMINATE_MESSAGE:
          break;

        case TIMER_HAS_EXPIRED:
          LOG_E(GTPU, "Received unexpected timer expired (no need of timers in this version) %s\n", ITTI_MSG_NAME(message_p));
          break;

        case GTPV1U_ENB_END_MARKER_REQ:
          gtpv1uEndTunnel(compatInst(ITTI_MSG_DESTINATION_INSTANCE(message_p)),
                          &GTPV1U_ENB_TUNNEL_DATA_REQ(message_p));
          itti_free(OCP_GTPV1_U, GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).buffer);
          break;

        case GTPV1U_ENB_DATA_FORWARDING_REQ:
        case GTPV1U_ENB_DATA_FORWARDING_IND:
        case GTPV1U_ENB_END_MARKER_IND:
          LOG_E(GTPU, "to be developped %s\n", ITTI_MSG_NAME(message_p));
          abort();
          break;

        case GTPV1U_ENB_S1_REQ:
          // to be dev: should be removed, to use API
          strcpy(addr.originHost, GTPV1U_ENB_S1_REQ(message_p).addrStr);
          strcpy(addr.originService, GTPV1U_ENB_S1_REQ(message_p).portStr);
          AssertFatal((legacyInstanceMapping=ocp_gtpv1Init(addr))!=0,"Instance 0 reserved for legacy\n");
          break;

        case GTPV1U_GNB_NG_REQ:
          // to be dev: should be removed, to use API
          strcpy(addr.originHost, GTPV1U_GNB_NG_REQ(message_p).addrStr);
          strcpy(addr.originService, GTPV1U_GNB_NG_REQ(message_p).portStr);
          AssertFatal((legacyInstanceMapping=ocp_gtpv1Init(addr))!=0,"Instance 0 reserved for legacy\n");
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

    for (int i = 0; i < nb_events; i++)
      if ((events[i].events&EPOLLIN))
        gtpv1uReceiver(events[i].data.fd);
  }

  return NULL;
}

#ifdef __cplusplus
}
#endif
