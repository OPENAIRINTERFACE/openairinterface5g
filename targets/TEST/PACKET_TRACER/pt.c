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
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <string.h>
#include <math.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

//#include "SIMULATION/TOOLS/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "MAC_INTERFACE/vars.h"

#include "ARCH/CBMIMO1/DEVICE_DRIVER/vars.h"
#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/vars.h"
#include "RRC/LTE/vars.h"
#include "PHY_INTERFACE/vars.h"
#include "UTIL/LOG/log.h"
#include "OCG_vars.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg.h"
#include "UTIL/OTG/otg_vars.h"


char binary_table[16][5] = {{"0000\0"},{"0001\0"},{"0010\0"},{"0011\0"},{"0100\0"},{"0101\0"},{"0110\0"},{"0111\0"},{"1000\0"},{"1001\0"},{"1010\0"},{"1011\0"},{"1100\0"},{"1101\0"},{"1110\0"},{"1111\0"}};

typedef enum {
  RRCConnectionRequest=0,
  RRCConnectionSetup,
  RRCConnectionSetupComplete,
  RRCConnectionReconfiguration,
  RRCConnectionReconfigurationComplete,
  RRCSIB1,
  RRCSIB2_3
} RRC_MESSAGES_t;

typedef enum {
  eNB_RRC=0,
  eNB_S1U,
  eNB_PDCP,
  eNB_RLC_UM,
  eNB_RLC_AM,
  eNB_RLC_TM,
  eNB_MAC,
  UE_RRC,
  UE_S1U,
  UE_PDCP,
  UE_RLC_UM,
  UE_RLC_AM,
  UE_RLC_TM,
  eNB_SI
} SDUsource_t;


typedef struct {
  int verbose;
  int RRCmessage;
  int lcid;
  int rbid;
  int SDUsize;
  int TBsize;
  SDUsource_t SDUsource;
  char input1_file[128];
  char input2_file[128];
  char output_file[128];
  unsigned char input1_sdu[1024];
  unsigned char input2_sdu[1024];
  int input1_sdu_flag;
  unsigned short input1_sdu_len;
  int input2_sdu_flag;
  unsigned short input2_sdu_len;
  int frame;
  int input_sib;
  int input_eNB_MAC;
} args_t;

int input_text(char *file,char *sdu)
{

  FILE *fd;
  int i,b_ind=0;
  char c,c0,c1;
  unsigned char byte;

  fd = fopen(file,"r");

  while (!feof(fd)) {
    c0 = fgetc(fd);

    if (!feof(fd))
      c1 = fgetc(fd);
    else
      return(-1);

    byte = 0;

    for (i=0,c=c0; i<2; i++,c=c1)
      switch(c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        byte += (c-'0')<<(4*(1-i));
        break;

      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        byte += (10+c-'A')<<(4*(1-i));
        break;

      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        byte += (10+c-'a')<<(4*(1-i));
        break;

      case '\n':
      case '\0':
        return(b_ind);
        break;

      default:
        printf("Unknown hex character %c in file\n",c);
        return(-1);
        break;
      }

    sdu[b_ind]=byte;
    printf("byte %d: %x (%s%s)\n",b_ind,byte,binary_table[byte>>4],binary_table[byte&0xf]);
    b_ind++;
  }
  if (fd)
    fclose(fd);
  return(b_ind);
}


int parse_args(int argc, char** argv, args_t* args)
{
  int c;
  int k;
  const struct option long_options[] = {
    {"lcid", required_argument, NULL, 256},
    {"rbid", required_argument, NULL, 257},
    {"TBsize", required_argument, NULL, 258},
    {"SDUsize", required_argument, NULL, 259},
    {"eNB_RRC", no_argument, NULL, 260},
    {"eNB_S1U", no_argument, NULL, 261},
    {"eNB_PDCP", no_argument, NULL, 262},
    {"eNB_RLC_UM", no_argument, NULL, 263},
    {"eNB_RLC_AM", no_argument, NULL, 264},
    {"eNB_RLC_TM", no_argument, NULL, 265},
    {"eNB_MAC"   , no_argument, NULL, 266},
    {"UE_RRC", no_argument, NULL, 267},
    {"UE_S1U", no_argument, NULL, 268},
    {"UE_PDCP", no_argument, NULL, 269},
    {"UE_RLC_UM", no_argument, NULL, 270},
    {"UE_RLC_AM", no_argument, NULL, 271},
    {"UE_RLC_TM", no_argument, NULL, 272},
    {"RRCConnectionRequest", no_argument, NULL, 273},
    {"RRCConnectionSetup", no_argument, NULL, 274},
    {"RRCConnectionSetupComplete", no_argument, NULL, 275},
    {"RRCConnectionReconfiguration", no_argument, NULL, 276},
    {"RRCConnectionReconfigurationComplete", no_argument, NULL, 277},
    {"RRCSIB1", no_argument, NULL, 278},
    {"RRCSIB2_3", no_argument, NULL, 279},
    {NULL, 0, NULL, 0}
  };
  args->frame = 0;

  while((c = getopt_long(argc, argv, "ho:I:J:v", long_options, NULL)) != -1) {
    switch(c) {
    case 'h':
      return 1;

    case 'o':
      strcpy(args->output_file,optarg);
      break;

    case 'f':
      args->frame = atoi(optarg);
      break;

    case 'I':
      strcpy(args->input1_file,optarg);
      args->input1_sdu_len = input_text(args->input1_file,args->input1_sdu);
      printf("Got sdu1 of length %u bytes\n",args->input1_sdu_len);
      args->input1_sdu_flag=1;
      break;

    case 'J':
      strcpy(args->input2_file,optarg);
      args->input2_sdu_len = input_text(args->input2_file,args->input2_sdu);
      printf("Got sdu2 of length %u bytes\n",args->input2_sdu_len);
      args->input2_sdu_flag=1;
      break;

    case 'v':
      args->verbose++;
      break;

    case 256:  // lchan
      args->lcid = atoi(optarg);
      break;

    case 257:  //
      args->rbid = atoi(optarg);
      break;

    case 258:
      args->TBsize = atoi(optarg);
      break;

    case 259:
      args->SDUsize = atoi(optarg);
      break;

    case 260:  //eNB_RRC
    case 261:  //eNB_S1U
    case 262:  //eNB_PDCP
    case 263:  //eNB_RLC_UM
    case 264:  //eNB_RLC_AM
    case 265:  //eNB_RLC_TM
    case 266:  //eNB_MAC
    case 267:  //UE_RRC
    case 268:  //UE_S1U
    case 269:  //UE_PDCP
    case 270:  //UE_RLC_UM
    case 271:  //UE_RLC_AM
    case 272:  //UE_RLC_TM
      args->SDUsource = c-260;
      break;

    case 273:  //RRCConnectionRequest
    case 274:  //RRCConnectionSetup
    case 275:  //RRCConnectionSetupComplete
    case 276:  //RRCConnectionReconfiguration
    case 277:  //RRCConnectionReconfigurationComplete
    case 278:  //RRCSIB1
    case 279:  //RRCSIB2_3
      args->RRCmessage = c-273;
      break;

    default:
      return 1;
    }
  }

  return 0;
}

void print_usage(const char* prog)
{
  printf("Usage: %s [options]\n", prog);
  printf("\n");
  printf("    General options:\n");
  printf("  -h         : print usage\n");
  printf("  -v         : increase verbosity level [0]\n");
  printf("  -o         : output .dat file (binary packet data)\n");
  printf("  -i         : input .dat file (binary packet data)\n");
  printf("  -I         : input .txt file (ASCII packet data)\n");
  printf("\n");
  printf("    Packet options:\n");
  printf("  --eNB_RRC    : set source of packet to eNB_RRC\n");
  printf("  --eNB_S1U    : set source of packet to eNB_S1U (e.g. IP)\n");
  printf("  --eNB_PDCP   : set source of packet to eNB_PDCP\n");
  printf("  --eNB_RLC_UM : set source of packet to eNB_RLC_UM\n");
  printf("  --eNB_RLC_AM : set source of packet to eNB_RLC_AM\n");
  printf("  --eNB_RLC_TM : set source of packet to eNB_RLC_TM\n");
  printf("  --eNB_MAC    : set source of packet to eNB_MAC\n");
  printf("  --UE_RRC     : set source of packet to UE_RRC\n");
  printf("  --UE_S1U     : set source of packet to UE_S1U (e.g. IP)\n");
  printf("  --UE_PDCP    : set source of packet to UE_PDCP\n");
  printf("  --UE_RLC_UM  : set source of packet to UE_RLC_UM\n");
  printf("  --UE_RLC_AM  : set source of packet to UE RLC_AM\n");
  printf("  --UE_RLC_TM  : set source of packet to UE RLC_TM\n");
  printf("  --lchan      : set logical channel id\n");
  printf("  --rbid       : set radio bearer id\n");
  printf("  --TBsize     : set Transport Block Size at MAC layer\n");
  printf("  --SDUsize    : set the SDU size used by the originating entity (except RRC)\n");
  printf("  --RRCConnectionSetup                         : set the SDU to be an RRCConnectionSetup\n");
  printf("  --RRCConnectionSetupComplete                 : set the SDU to be an RRCConnectionSetupComplete\n");
  printf("  --RRCConnectionRequest                       : set the SDU to be an RRCConnectionRequest\n");
  printf("  --RRCConnectionReconfiguration               : set the SDU to be an RRCConnectionReconfiguration\n");
  printf("  --RRCConnectionReconfigurationComplete       : set the SDU to be an RRCConnectionReconfigurationComplete\n");
  printf("  --RRCSIB1                                    : set the SDU to be an RRCSIB1\n");
  printf("  --RRCSIB2_3                                  : set the SDU to be an RRCSIB2_3\n");
}

u8 attach_ue0(char *sdu)
{
  u8 Size;
  u16 Size16;
  u8 lcid=0;

  UE_rrc_inst[0].Info[0].State = RRC_SI_RECEIVED;
  rrc_ue_generate_RRCConnectionRequest(0,131,0);

  Size = mac_rrc_data_req(0,
                          131,
                          CCCH,1,
                          &sdu[sizeof(SCH_SUBHEADER_FIXED)],0,
                          0);
  Size16 = (u16)Size;

  generate_ulsch_header((u8 *)sdu,  // mac header
                        1,      // num sdus
                        0,            // short padding
                        &Size16,  // sdu length
                        &lcid,    // sdu lcid
                        NULL,  // power headroom
                        NULL,  // crnti
                        NULL,  // truncated bsr
                        NULL, // short bsr
                        NULL,
                        0); // post padding

  return(Size+sizeof(SCH_SUBHEADER_FIXED));
}

// This retrieves the RRCConnectionSetup RRC SDU
u8 attach_ue1(char *sdu)
{
  // simulate reception of CCCH at eNB(RRCConnectionRequest)
  mac_rrc_lite_data_ind(0,131,0,UE_rrc_inst[0].Srb0[0].Tx_buffer.Payload,
                        UE_rrc_inst[0].Srb0[0].Tx_buffer.payload_size,1,0);

  return(mac_rrc_data_req(0,
                          132,
                          0,1,
                          sdu,
                          1,
                          0));
}

// This retrieves the RRCConnectionSetupComplete RRC SDU
u16 attach_ue2(char *dcch_sdu_eNB, char dcch_sdu_eNB_len, char *dcch_sdu)
{

  mac_rlc_status_resp_t rlc_status;
  u16 sdu_len;

  printf("************* attach_ue2 (CCCH payload %d)\n",
         eNB_rrc_inst[0].Srb0.Tx_buffer.payload_size);
  // simulate reception of CCCH at UE (RRCConnectionSetup)
  mac_rrc_lite_data_ind(0,132,0,dcch_sdu_eNB,dcch_sdu_eNB_len,0,0);

  // simulate transmission of RRCConnectionSetupComplete
  rlc_status = mac_rlc_status_ind(1,134,0,
                                  DCCH,
                                  100);
  sdu_len = mac_rlc_data_req(1,133,
                             DCCH,
                             dcch_sdu);

  return(sdu_len);
}

// This retrieves the RRCConnectionReconfiguration RRC SDU
u16 attach_ue3(char *dcch_sdu_ue, char dcch_sdu_ue_len, char *dcch_sdu_eNB)
{

  mac_rlc_status_resp_t rlc_status;
  u16 sdu_len;

  // simulation reception of RRCConnectionSetupComplete

  mac_rlc_data_ind(0,134,1,
                   DCCH,
                   dcch_sdu_ue,
                   dcch_sdu_ue_len,
                   1,
                   NULL);

  /// ... and generation of RRCConnectionReconfiguration
  /// First RLC-AM Control SDU (ACK)
  rlc_status = mac_rlc_status_ind(0,135,1,
                                  DCCH,
                                  100);

  sdu_len = mac_rlc_data_req(0,135,
                             DCCH,
                             dcch_sdu_eNB);

  mac_rlc_data_ind(1,135,1,
                   DCCH,
                   dcch_sdu_eNB,
                   sdu_len,
                   1,
                   NULL);

  // now RRC message
  rlc_status = mac_rlc_status_ind(0,136,1,
                                  DCCH,
                                  100);

  sdu_len = mac_rlc_data_req(0,136,
                             DCCH,
                             dcch_sdu_eNB);

  return(sdu_len);
}

// This retrieves the RRCConnectionReconfigurationComplete RRC SDU
u16 attach_ue4(char *dcch_sdu_eNB, char dcch_sdu_eNB_len, char *dcch_sdu_ue)
{

  mac_rlc_status_resp_t rlc_status;
  u16 sdu_len;

  // simulation UE reception of RRCConnectionReconfiguration ....

  mac_rlc_data_ind(1,135,0,
                   DCCH,
                   dcch_sdu_eNB,
                   dcch_sdu_eNB_len,
                   1,
                   NULL);

  // ... and generation of RRCConnectionReconfigurationComplete


  rlc_status = mac_rlc_status_ind(1,135,0,
                                  DCCH,
                                  100);

  sdu_len = mac_rlc_data_req(1,135,
                             DCCH,
                             dcch_sdu_ue);

  rlc_status = mac_rlc_status_ind(1,136,0,
                                  DCCH,
                                  100);

  sdu_len = mac_rlc_data_req(1,136,
                             DCCH,
                             dcch_sdu_ue);

  return(sdu_len);
}

//u8 NB_INST=2;

int main (int argc, char **argv)
{

  args_t args;
  LTE_DL_FRAME_PARMS frame_parms0;
  LTE_DL_FRAME_PARMS *frame_parms=&frame_parms;
  char sdu0[16],sdu1[64],sdu2[1024],sdu3[1024],sdu4[1024];
  unsigned short sdu_len0,sdu_len1,sdu_len2,sdu_len3,sdu_len4;
  char ulsch_buffer[1024],dlsch_buffer[1024];
  u8 lcid;
  u8 payload_offset;
  int i,comp;

  logInit();

  NB_UE_INST  = 1;
  NB_eNB_INST = 1;
  NB_INST=2;

  // Parse arguments
  if(parse_args(argc, argv, &args) > 0) {
    print_usage(argv[0]);
    exit(1);
  }

  set_taus_seed(0);
  logInit();
  set_glog(LOG_TRACE, 1);

  for (comp = PHY; comp < MAX_LOG_COMPONENTS ; comp++)
    set_comp_log(comp,
                 LOG_TRACE,
                 LOG_NONE,
                 1);

  /*
  set_log(OMG,  LOG_INFO, 20);
   set_log(EMU,  LOG_INFO, 10);
   set_log(OCG,  LOG_INFO, 1);
   set_log(MAC,  LOG_TRACE, 1);
   set_log(RLC,  LOG_TRACE, 1);
   set_log(PHY,  LOG_DEBUG, 1);
   set_log(PDCP, LOG_TRACE, 1);
   set_log(RRC,  LOG_TRACE, 1);
  */
  mac_xface = (MAC_xface *)malloc(sizeof(MAC_xface));
  init_lte_vars (&frame_parms, 0, 1, 0, 0, 25, 0, 0, 1, 1);
  l2_init(frame_parms);

  // Generate eNB SI
  if (args.input_sib == 0) {
    openair_rrc_lite_eNB_init(0);
  } else {
    printf("Got SI from files (%d,%d,%d)\n",
            args.input_sib,args.input1_sdu_flag,args.input2_sdu_flag);
  }

  openair_rrc_on(0,0);
  openair_rrc_lite_ue_init(0,0);

  switch (args.SDUsource) {
  case eNB_RRC:
    if (args.RRCmessage == RRCSIB1) {
      if (args.input1_sdu_flag == 1) {
        for (i=0; i<args.input1_sdu_len; i++)
          printf("%02x",args.input1_sdu[i]);

        printf("\n");
        ue_decode_si(0,142,0,args.input1_sdu,args.input1_sdu_len);
      } else {
        printf("\n\nSIB1\n\n");

        for (i=0; i<eNB_rrc_inst[0].sizeof_SIB1; i++)
          printf("%02x",eNB_rrc_inst[0].SIB1[i]);

        printf("\n");
      }
    } else if (args.RRCmessage == RRCSIB2_3) {
      if (args.input1_sdu_flag == 1) {
        for (i=0; i<args.input2_sdu_len; i++)
          printf("%02x",args.input2_sdu[i]);

        printf("\n");
        ue_decode_si(0,149,0,args.input1_sdu,args.input1_sdu_len);
      } else {
        printf("\n\nSIB2_3\n\n");

        for (i=0; i<eNB_rrc_inst[0].sizeof_SIB23; i++)
          printf("%02x",eNB_rrc_inst[0].SIB23[i]);

        printf("\n");
      }
    } else if ((args.input1_sdu_flag == 1)&&
               ((args.RRCmessage == RRCConnectionSetup) ||
                (args.RRCmessage == RRCConnectionReconfiguration))) {
      sdu_len0 = attach_ue0(sdu0);
      ue_send_sdu(0,143,args.input1_sdu,args.input1_sdu_len,0);

      if (args.RRCmessage == RRCConnectionReconfiguration)
        ue_send_sdu(0,144,args.input2_sdu,args.input2_sdu_len,0);
    } else {
      switch (args.RRCmessage) {

      case RRCConnectionSetup:
        printf("Doing eNB RRCConnectionSetup\n");
        sdu_len0 = attach_ue0(sdu0);
        sdu_len1 = attach_ue1(sdu1);
        lcid = 0;
        payload_offset = generate_dlsch_header(dlsch_buffer,
                                               1,              //num_sdus
                                               &sdu_len1,      //
                                               &lcid,          // sdu_lcid
                                               255,                                   // no drx
                                               0,                                   // no timing advance
                                               sdu0,        // contention res id
                                               0,0);
        memcpy(&dlsch_buffer[payload_offset],sdu1,sdu_len1);
        printf("\nRRCConnectionSetup (DLSCH input / MAC output)\n\n");

        for (i=0; i<sdu_len1+payload_offset; i++)
          printf("%02x ",(unsigned char)sdu1[i]);

        printf("\n");

        break;

      case RRCConnectionReconfiguration:
        printf("Doing eNB RRCConnectionReconfiguration\n");
        sdu_len0 = attach_ue0(sdu0);
        sdu_len1 = attach_ue1(sdu1);
        sdu_len2 = attach_ue2(sdu1,sdu_len1,sdu2);
        sdu_len3 = attach_ue3(sdu2,sdu_len2,sdu3);

        lcid=1;
        payload_offset = generate_dlsch_header(dlsch_buffer,
                                               // offset = generate_dlsch_header((unsigned char*)eNB_mac_inst[0].DLSCH_pdu[0][0].payload[0],
                                               1,              //num_sdus
                                               &sdu_len3,  //
                                               &lcid,
                                               255,                                   // no drx
                                               1,      // timing advance
                                               NULL,                                  // contention res idk
                                               0,0);
        memcpy(&dlsch_buffer[payload_offset],sdu3,sdu_len3);
        printf("\nRRCConnectionReconfiguration (DLSCH input / MAC output)\n\n");

        for (i=0; i<sdu_len3+payload_offset; i++)
          printf("%02x ",(unsigned char)dlsch_buffer[i]);

        printf("\n");

        break;

      default:
        printf("Unknown eNB_RRC SDU (%d), exiting\n",args.RRCmessage);
        break;
      }

      break;
    }

    break;

  case eNB_MAC:
    sdu_len0 = attach_ue0(sdu0);
    ue_send_sdu(0,143,args.input1_sdu,args.input1_sdu_len,0);
    break;

  case UE_RRC:
    switch (args.RRCmessage) {

    case RRCConnectionRequest:

      sdu_len0 = attach_ue0(sdu0);
      printf("\n\nRRCConnectionRequest\n\n");

      for (i=0; i<sdu_len0; i++)
        printf("%02x ",(unsigned char)sdu0[i]);

      printf("\n");

      break;

    case RRCConnectionSetupComplete:

      sdu_len0=attach_ue0(sdu0);
      sdu_len1 = attach_ue1(sdu1);
      sdu_len2 = attach_ue2(sdu1,sdu_len1,sdu2);


      lcid=1;
      printf("Got sdu of length %d\n",sdu_len2);
      payload_offset = generate_ulsch_header((u8*)ulsch_buffer,  // mac header
                                             1,      // num sdus
                                             0,            // short pading
                                             &sdu_len2,  // sdu length
                                             &lcid,    // sdu lcid
                                             NULL,  // power headroom
                                             NULL,  // crnti
                                             NULL,  // truncated bsr
                                             NULL, // short bsr
                                             NULL, // long_bsr
                                             0);
      printf("Got MAC header of length %d\n",payload_offset);
      memcpy(&ulsch_buffer[payload_offset],sdu2,sdu_len2);

      printf("\n\nRRCConnectionSetupComplete (ULSCH input / MAC output)\n\n");

      for (i=0; i<sdu_len2+payload_offset; i++)
        printf("%02x ",(unsigned char)ulsch_buffer[i]);

      printf("\n");
      break;

    case RRCConnectionReconfigurationComplete:

      sdu_len0=attach_ue0(sdu0);
      sdu_len1=attach_ue1(sdu1);
      sdu_len2 = attach_ue2(sdu1,sdu_len1,sdu2);
      sdu_len3 = attach_ue3(sdu2,sdu_len2,sdu3);
      sdu_len4 = attach_ue4(sdu3,sdu_len3,sdu4);

      lcid=1;
      printf("Got sdu of length %d\n",sdu_len4);
      payload_offset = generate_ulsch_header((u8*)ulsch_buffer,  // mac header
                                             1,      // num sdus
                                             0,            // short pading
                                             &sdu_len4,  // sdu length
                                             &lcid,    // sdu lcid
                                             NULL,  // power headroom
                                             NULL,  // crnti
                                             NULL,  // truncated bsr
                                             NULL, // short bsr
                                             NULL, // long_bsr
                                             0);
      printf("Got MAC header of length %d\n",payload_offset);
      memcpy(&ulsch_buffer[payload_offset],sdu4,sdu_len4);

      printf("\n\nRRCConnectionReconfigurationComplete (ULSCH input / MAC output)\n\n");

      for (i=0; i<sdu_len4+payload_offset; i++)
        printf("%02x ",(unsigned char)ulsch_buffer[i]);

      printf("\n");

      break;

    default:
      printf("Unknown UE_RRC SDU (%d), exiting\n",args.RRCmessage);
      break;
    }

    break;
  }


  free(mac_xface);

  return(0);
}
