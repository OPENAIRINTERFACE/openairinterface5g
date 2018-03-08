/*
 *Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
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

/*! \file openair2/ENB_APP/NB_IoT_interface.h
 * \brief: api interface for nb-iot application
 * \date 2018
 * \version 0.1
 * \note
 * \warning
 */
#ifndef NBIOT_INTERFACE_H
#define NBIOT_INTERFACE_H


#define NBIOT_MODULENAME "NB_IoT"

typedef void(*RCConfig_NbIoT_f_t)(RAN_CONTEXT_t *RC);
#define NBIOT_RCCONFIG_FNAME "RCConfig_NbIoT"

#ifdef NBIOT_INTERFACE_SOURCE

#define NBIOT_INTERFACE_FLIST {\
{NBIOT_RCCONFIG_FNAME,NULL},\
}

#else /* NBIOT_INTERFACE_SOURCE */

extern int load_NB_IoT(void); 

#endif

#endif
