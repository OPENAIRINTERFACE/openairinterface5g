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

/*! \file telnetsrv_phycmd.h
 * \brief: Include file defining telnet commands related to softmodem linux process
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#ifndef __TELNETSRV_PHYCMD__H__
#define __TELNETSRV_PHYCMD__H__

#ifdef TELNETSRV_PHYCMD_MAIN

#include "common/utils/LOG/log.h"


#include "openair1/PHY/phy_extern.h"


#define TELNETVAR_PHYCC0    0
#define TELNETVAR_PHYCC1    1

telnetshell_vardef_t phy_vardef[] = {{"", 0, 0, NULL}};

#else

extern void add_phy_cmds(void);

#endif

/*-------------------------------------------------------------------------------------*/

#endif
