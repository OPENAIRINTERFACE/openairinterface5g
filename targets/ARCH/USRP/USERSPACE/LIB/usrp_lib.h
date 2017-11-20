#ifndef __USRP_LIB_H
#define __USRP_LIB_H
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

/** usrp_lib.h
 *
 * \author: bruno.mongazon-cazavet@nokia-bell-labs.com
 */

#if defined (USRP_REC_PLAY)

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common/config/config_paramdesc.h"
#include "common/config/config_userapi.h"

#define BELL_LABS_IQ_HEADER       0xabababababababab
#define BELL_LABS_IQ_PER_SF       7680 // Up to 5MHz bw for now
#define BELL_LABS_IQ_BYTES_PER_SF (BELL_LABS_IQ_PER_SF * 4)
typedef struct {
  int64_t       header;
  int64_t       ts;
  int64_t       rfu1;
  int64_t       rfu2; // pad for 256 bits alignement required by AVX2
  unsigned char samples[BELL_LABS_IQ_BYTES_PER_SF]; // iq's for one subframe
} iqrec_t;
#define DEF_NB_SF           120000               // default nb of sf or ms to capture (2 minutes at 5MHz)
#define DEF_SF_FILE         "/home/nokia/iqfile" // default subframes file name
#define DEF_SF_DELAY_READ   400                  // default read delay  µs (860=real)
#define DEF_SF_DELAY_WRITE  15                   // default write delay µs (15=real)
#define DEF_SF_NB_LOOP      5                    // default nb loops


/* help strings definition for command line options, used in CMDLINE_XXX_DESC macros and printed when -h option is used */
#define CONFIG_HLP_SF_FILE      "Path of the file used for subframes record or replay"
#define CONFIG_HLP_SF_REC       "Record subframes from USRP driver into a file for later replay"
#define CONFIG_HLP_SF_REP       "Replay subframes into USRP driver from a file"
#define CONFIG_HLP_SF_MAX       "Maximum count of subframes to be recorded in subframe file"
#define CONFIG_HLP_SF_LOOPS     "Number of loops to replay of the entire subframes file"
#define CONFIG_HLP_SF_RDELAY    "Delay in microseconds to read a subframe in replay mode"
#define CONFIG_HLP_SF_WDELAY    "Delay in microseconds to write a subframe in replay mode"

/* keyword strings for command line options, used in CMDLINE_XXX_DESC macros and printed when -h option is used */
#define CONFIG_OPT_SF_FILE      "subframes-file"
#define CONFIG_OPT_SF_REC       "subframes-record"
#define CONFIG_OPT_SF_REP       "subframes-replay"
#define CONFIG_OPT_SF_MAX       "subframes-max"
#define CONFIG_OPT_SF_LOOPS     "subframes-loops"
#define CONFIG_OPT_SF_RDELAY    "subframes-read-delay"
#define CONFIG_OPT_SF_WDELAY    "subframes-write-delay"

/* For information only - the macro is not usable in C++ */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters for USRP record/playback                                                                               */
/*   optname                     helpstr                paramflags                      XXXptr                  defXXXval                            type           numelt   */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define USRP_RECPLAY_PARAMS_DESC {  \
{"subframes-file",        	 CONFIG_HLP_SF_FILE,	0,		  strptr:(char **)&u_sf_filename,       defstrval:DEF_SF_FILE, 		   TYPE_STRING,   sizeof(u_sf_filename)}, \
{"subframes-record",      	 CONFIG_HLP_SF_REC,	PARAMFLAG_BOOL,	  uptr:&u_sf_record,	                defuintval:0,			   TYPE_UINT,	  0}, \
{"subframes-replay",      	 CONFIG_HLP_SF_REP,	PARAMFLAG_BOOL,	  uptr:&u_sf_replay,	                defuintval:0,			   TYPE_UINT,	  0}, \
{"subframes-max",              	 CONFIG_HLP_SF_MAX,    	0,                uptr:&u_sf_max,			defintval:DEF_NB_SF,	       	   TYPE_UINT,	  0}, \
{"subframes-loops",           	 CONFIG_HLP_SF_LOOPS,   0,                uptr:&u_sf_loops,			defintval:DEF_SF_NB_LOOP,	   TYPE_UINT,	  0}, \
{"subframes-read-delay",         CONFIG_HLP_SF_RDELAY,  0,                uptr:&u_sf_read_delay,	      	defintval:DEF_SF_DELAY_READ,	   TYPE_UINT,	  0}, \
{"subframes-write-delay",        CONFIG_HLP_SF_WDELAY,  0,                uptr:&u_sf_write_delay,		defintval:DEF_SF_DELAY_WRITE,	   TYPE_UINT,	  0}, \
}
#endif // BELL_LABS_MUST
#endif // __USRP_LIB_H

