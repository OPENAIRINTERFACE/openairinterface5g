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

/*
  This library offers some functions to remotely program the R&S SMBV100A.
*/


#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <netdb.h>
#ifndef CONFIG_SMBV
#include "../impl_defs_lte.h"
#include "../impl_defs_top.h"
#include "../defs.h"
#include "../LTE_TRANSPORT/defs.h"
#else
#define msg printf
#endif

#define DEFAULT_SMBV_IP "192.168.12.201"
#define DEFAULT_SMBV_FNAME "smbv_config_file.smbv"
#define MYPORT "5025"
#define BUFFER_LENGTH 256
#define MAX_INST_LENGTH 256
#define MAX_CONFIG_FRAMES 4

unsigned short slen; // sequence length in frames, max is 4
/*
#ifdef EXMIMO
const unsigned short config_frames[MAX_CONFIG_FRAMES];
#else
extern const unsigned short config_frames[MAX_CONFIG_FRAMES];
#endif
*/

// function sends the config file "fname" to SMBV through socket sockfd
int smbv_configure_from_file(int sockfd, const char* fname);

// Initiates the connection to SMBV and sends all commands in config file "fname"
int smbv_send_config (const char* fname, char* smbv_ip);

#ifndef CONFIG_SMBV
// writes basic init commands to config file
int smbv_init_config(const char* fname, uint16_t sequence_length);

// writes config from frame_parms to config file
int smbv_write_config_from_frame_parms(const char* fname, LTE_DL_FRAME_PARMS *frame_parms);

// This function creates a datalist for an allocation containing the data in buffer
// len is in bytes
int smbv_configure_datalist_for_alloc(const char* fname, uint8_t alloc, uint8_t subframe, uint8_t *buffer, uint32_t len);

// checks if frame is part of the config_frames[]
int smbv_is_config_frame(uint32_t frame);

// This function creates a datalist for user containing the data in buffer
// len is in bytes
int smbv_configure_datalist_for_user(const char* fname, uint8_t user, uint8_t *buffer, uint32_t len);

// configures one of maximal 4 users
int smbv_configure_user(const char* fname, uint8_t user, uint8_t transmission_mode, uint16_t rnti);

// Configures the PDCCH
int smbv_configure_pdcch(const char* fname,uint8_t subframe,uint8_t num_pdcch_symbols,uint8_t num_dci);

// Configures the common DCIs SI, RA and PA
// type is either "SI", "RA" or "PA"
// item is the row in the DCI table
int smbv_configure_common_dci(const char* fname, uint8_t subframe, const char* type, DCI_ALLOC_t *dci_alloc, uint8_t item);

// Configures UE-spec DCI for user 1,2,3,4
// item is the row in the DCI table
int smbv_configure_ue_spec_dci(const char* fname, uint8_t subframe, uint8_t user, DCI_ALLOC_t *dci_alloc, uint8_t item);
#endif
