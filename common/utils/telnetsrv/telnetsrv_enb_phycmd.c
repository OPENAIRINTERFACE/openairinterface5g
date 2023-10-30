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

/*! \file telnetsrv_enb_phycmd.c
 * \brief: implementation of telnet commands related to softmodem linux process
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE
#include <string.h>
#include <pthread.h>


#define TELNETSERVERCODE
#include "telnetsrv.h"
#define TELNETSRV_PHYCMD_MAIN
#include "telnetsrv_phycmd.h"

#include "PHY/defs_eNB.h"
#include "LAYER2/MAC/mac_proto.h"

char *prnbuff;
extern int dump_eNB_stats(PHY_VARS_eNB *eNB, char *buffer, int length);

void init_phytelnet(void) {
  prnbuff=malloc(get_phybsize() );

  if (prnbuff == NULL) {
    fprintf(stderr,"Error %s on malloc in init_phytelnet()\n",strerror(errno));
  }
}

void display_uestatshead( telnet_printfunc_t prnt) {
  prnt("cc  ue  rnti Dmcs Umcs tao  tau   Dbr  Dtb   \n");
}

void dump_uestats(int debug, telnet_printfunc_t prnt, uint8_t prntflag) {
  int p;
  p=dump_eNB_l2_stats( prnbuff, 0);

  if(prntflag>=1)
    prnt("%s\n",prnbuff);

  if(debug>=1)
    prnt("%i bytes printed\n",p);
}

void display_uestats(int debug, telnet_printfunc_t prnt, int ue) {
  for (int cc=0; cc<1 ; cc++) {
  }
}

void display_phycounters(char *buf, int debug, telnet_printfunc_t prnt) {
  prnt("  DLSCH kb      DLSCH kb/s\n");
  dump_uestats(debug, prnt,0);
}

int dump_phyvars(char *buf, int debug, telnet_printfunc_t prnt) {
  if (debug > 0)
    prnt("phy interface module received %s\n",buf);

  if (strcasestr(buf,"phycnt") != NULL) {
    display_phycounters(buf, debug, prnt);
  }

  if (strcasestr(buf,"uestat") != NULL) {
    char *cptr=strcasestr(buf+sizeof("uestat"),"UE");
    display_uestatshead(prnt);

    if (cptr != NULL) {
      int ueidx = strtol( cptr+sizeof("UE"), NULL, 10);

      if (ueidx < NUMBER_OF_UE_MAX && ueidx >= 0) {
        display_uestats(debug, prnt,ueidx);
      }
    } /* if cptr != NULL */
    else {
      for (int ue=0; ue<NUMBER_OF_UE_MAX ; ue++) {
        display_uestats(debug, prnt,ue);
      }
    } /* else cptr != NULL */
  } /* uestat */

  if (strcasestr(buf,"uedump") != NULL) {
    dump_uestats(debug, prnt,1);
  }

  return 0;
}



telnetshell_cmddef_t phy_cmdarray[] = {
  {"disp","[phycnt,uedump,uestat UE<x>]", dump_phyvars},

  {"","",NULL},
};


/*-------------------------------------------------------------------------------------*/
void add_phy_cmds(void) {
  init_phytelnet();
  add_telnetcmd("phy", phy_vardef, phy_cmdarray);
}
