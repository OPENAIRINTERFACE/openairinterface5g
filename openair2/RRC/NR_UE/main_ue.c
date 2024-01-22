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

/* \file main_ue.c
 * \brief RRC layer top level initialization
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include "defs.h"
#include "rrc_proto.h"
#include "common/utils/LOG/log.h"
#include "executables/softmodem-common.h"

void init_nsa_message(NR_UE_RRC_INST_t *rrc, char* reconfig_file, char* rbconfig_file)
{
  if (get_softmodem_params()->phy_test == 1 || get_softmodem_params()->do_ra == 1) {
    // read in files for RRCReconfiguration and RBconfig

    LOG_I(NR_RRC, "using %s for rrc init[1/2]\n", reconfig_file);
    FILE *fd = fopen(reconfig_file, "r");
    AssertFatal(fd,
                "cannot read file %s: errno %d, %s\n",
                reconfig_file,
                errno,
                strerror(errno));
    char buffer[1024];
    int msg_len = fread(buffer, 1, 1024, fd);
    fclose(fd);
    process_nsa_message(rrc, nr_SecondaryCellGroupConfig_r15, buffer, msg_len);

    LOG_I(NR_RRC, "using %s for rrc init[2/2]\n", rbconfig_file);
    fd = fopen(rbconfig_file, "r");
    AssertFatal(fd,
                "cannot read file %s: errno %d, %s\n",
                rbconfig_file,
                errno,
                strerror(errno));
    msg_len = fread(buffer, 1, 1024, fd);
    fclose(fd);
    process_nsa_message(rrc, nr_RadioBearerConfigX_r15, buffer,msg_len);
  }
  else
    LOG_D(NR_RRC, "In NSA mode \n");
}
