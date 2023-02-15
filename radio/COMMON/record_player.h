#ifndef __RECORD_PLAYER_H
#define __RECORD_PLAYER_H
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

/** radio/COMMON/record-player.h
 *
 * \author: bruno.mongazon-cazavet@nokia-bell-labs.com
 */

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common/config/config_paramdesc.h"
#include "common/config/config_userapi.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* inclusions for record player */
#define RECPLAY_DISABLED     0
#define RECPLAY_RECORDMODE   1
#define RECPLAY_REPLAYMODE   2

#define BELL_LABS_IQ_HEADER       0xabababababababab
#define BELL_LABS_IQ_PER_SF       23040 // 23040 => 46080/2 slots => 3/4 40MHz (106 PRBs)
#define BELL_LABS_IQ_BYTES_PER_SF (BELL_LABS_IQ_PER_SF * 4)

#define OAIIQFILE_ID {'O', 'I','Q','F'}
typedef struct {
  uint64_t      devtype;
  uint64_t      tx_sample_advance;
  double        bw;
  unsigned int  nbSamplesBlocks;
  char          oaiid[4];
} iqfile_header_t;

typedef struct {
  int64_t       header;
  int64_t       ts;
  int64_t       nbBytes;
  int64_t       tv_sec;  // nb of secs since EPOCH
  int64_t       tv_usec; // nb of µsecs since EPOCH
  int64_t       rfu2;    // pad for 256 bits alignement required by AVX2
} iqrec_t;
#define DEF_NB_SF           120000               // default nb of sf or ms to capture (2 minutes at 5MHz)
#define DEF_SF_FILE         "/tmp/iqfile"        // default subframes file name
#define DEF_SF_DELAY_READ   700                  // default read delay  µs (860=real)
#define DEF_SF_DELAY_WRITE  15                   // default write delay µs (15=real)
#define DEF_SF_NB_LOOP      5                    // default nb loops


/* help strings definition for config options, used in CMDLINE_XXX_DESC macros and printed when -h option is used */
#define CONFIG_HLP_SF_FILE      "Path of the file used for subframes record or replay"
#define CONFIG_HLP_SF_REC       "Record subframes from device driver into a file for later replay"
#define CONFIG_HLP_SF_REP       "Replay subframes from a file using the oai replay driver"
#define CONFIG_HLP_SF_MAX       "Maximum count of subframes to be recorded in subframe file"
#define CONFIG_HLP_SF_LOOPS     "Number of loops to replay of the entire subframes file"
#define CONFIG_HLP_SF_RDELAY    "Delay in microseconds to read a subframe in replay mode"
#define CONFIG_HLP_SF_WDELAY    "Delay in microseconds to write a subframe in replay mode"
#define CONFIG_HLP_USE_MMAP     "In replay mode, map iq file in memory before replaying"
/* keyword strings for config options, used in CMDLINE_XXX_DESC macros and printed when -h option is used */
#define CONFIG_OPT_SF_FILE      "subframes-file"
#define CONFIG_OPT_SF_REC       "subframes-record"
#define CONFIG_OPT_SF_REP       "subframes-replay"
#define CONFIG_OPT_SF_MAX       "subframes-max"
#define CONFIG_OPT_SF_LOOPS     "subframes-loops"
#define CONFIG_OPT_SF_RDELAY    "subframes-read-delay"
#define CONFIG_OPT_SF_WDELAY    "subframes-write-delay"
#define CONFIG_OPT_USE_MMAP     "use-mmap"
#define DEVICE_RECPLAY_SECTION "device.recplay"
/* For information only - the macro is not usable in C++ */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters for USRP record/playback                                                                               */
/*   optname                     helpstr                paramflags                      XXXptr                           defXXXval                            type           numelt   */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define DEVICE_RECPLAY_PARAMS_DESC {  \
    {CONFIG_OPT_SF_FILE,      CONFIG_HLP_SF_FILE,   0,                .strptr=&((*recplay_conf)->u_sf_filename),   .defstrval=DEF_SF_FILE,           TYPE_STRING, 0}, \
    {CONFIG_OPT_SF_REC,       CONFIG_HLP_SF_REC,    PARAMFLAG_BOOL,   .uptr=&((*recplay_conf)->u_sf_record),       .defuintval=0,                    TYPE_UINT,   0}, \
    {CONFIG_OPT_SF_REP,       CONFIG_HLP_SF_REP,    PARAMFLAG_BOOL,   .uptr=&((*recplay_conf)->u_sf_replay),       .defuintval=0,                    TYPE_UINT,   0}, \
    {CONFIG_OPT_SF_MAX,       CONFIG_HLP_SF_MAX,    0,                .uptr=&((*recplay_conf)->u_sf_max),          .defintval=DEF_NB_SF,             TYPE_UINT,   0}, \
    {CONFIG_OPT_SF_LOOPS,     CONFIG_HLP_SF_LOOPS,  0,                .uptr=&((*recplay_conf)->u_sf_loops),        .defintval=DEF_SF_NB_LOOP,        TYPE_UINT,   0}, \
    {CONFIG_OPT_SF_RDELAY,    CONFIG_HLP_SF_RDELAY, 0,                .uptr=&((*recplay_conf)->u_sf_read_delay),   .defintval=DEF_SF_DELAY_READ,     TYPE_UINT,   0}, \
    {CONFIG_OPT_SF_WDELAY,    CONFIG_HLP_SF_WDELAY, 0,                .uptr=&((*recplay_conf)->u_sf_write_delay),  .defintval=DEF_SF_DELAY_WRITE,    TYPE_UINT,   0}, \
    {CONFIG_OPT_USE_MMAP,     CONFIG_HLP_USE_MMAP,  PARAMFLAG_BOOL,   .uptr=&((*recplay_conf)->use_mmap),          .defuintval=1,                    TYPE_UINT,   0}, \
  }/*! \brief Record Player Configuration and state */
typedef struct {
  char            *u_sf_filename;    // subframes file path
  unsigned int    u_sf_max ;         // max number of recorded subframes
  unsigned int    u_sf_loops ;       // number of loops in replay mode
  unsigned int    u_sf_read_delay;   // read delay in replay mode
  unsigned int    u_sf_write_delay ; // write delay in replay mode
  unsigned int    use_mmap;          // default is to use mmap
  unsigned int    u_sf_replay;       // replay mode (if 1)
  unsigned int    u_sf_record;       // record mode (if 1)
} recplay_conf_t;

typedef struct {
  size_t          mapsize;
  FILE            *pFile;
  int             fd;
  iqrec_t        *ms_sample;         // memory for all subframes
  unsigned int    nbSamplesBlocks;
  uint8_t        *currentPtr;
  uint64_t        currentTs;
  unsigned int    curSamplesBlock;
  int64_t         wrap_count;
  size_t          maxSizeBytes;
} recplay_state_t;

#ifdef __cplusplus
}
#endif
#endif // __RECORD_PLAYER_H

