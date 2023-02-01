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

// These messages files are mandatory and must always be placed in first position
#include "intertask_messages_def.h"
#include "timer_messages_def.h"

// Messages files used between tasks
#include "phy_messages_def.h"
#include "mac_messages_def.h"
#include "rlc_messages_def.h"
#include "pdcp_messages_def.h"
#include "rrc_messages_def.h"
#include "nas_messages_def.h"
#if ENABLE_RAL
#include "ral_messages_def.h"
#endif
#include "s1ap_messages_def.h"
#include "f1ap_messages_def.h"
#include "x2ap_messages_def.h"
#include "m2ap_messages_def.h"
#include "m3ap_messages_def.h"
#include "sctp_messages_def.h"
#include "udp_messages_def.h"
#include "gtpv1_u_messages_def.h"
#include "e1ap_messages_def.h"
