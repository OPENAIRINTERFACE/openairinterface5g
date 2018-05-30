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

/*! \file main.c
 * \brief top init of Layer 2
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \version 1.0
 * \email: navid.nikaein@eurecom.fr
 * @ingroup _mac

 */

#include "defs.h"
#include "proto.h"
#include "extern.h"
#include "assertions.h"

static NR_UE_MAC_INST_t *nr_ue_mac_inst; 

int
nr_l2_init_ue(void)
{
    LOG_I(MAC, "[MAIN] MAC_INIT_GLOBAL_PARAM IN...\n");

    LOG_I(MAC, "[MAIN] init UE MAC functions \n");
    
    //init mac here
    nr_ue_mac_inst = (NR_UE_MAC_INST_t *)malloc(sizeof(NR_UE_MAC_INST_t)*NB_NR_UE_MAC_INST);
    

    return (1);
}

NR_UE_MAC_INST_t *get_mac_inst(Module_id_t Mod_idP){
    return &nr_ue_mac_inst[(int)Mod_idP];
}
