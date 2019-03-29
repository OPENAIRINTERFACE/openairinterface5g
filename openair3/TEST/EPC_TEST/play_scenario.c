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
                                play_scenario.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */

#include <string.h>
#include <limits.h>
#include <libconfig.h>
#include <inttypes.h>
#include <getopt.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


#include "assertions.h"
#include "s1ap_common.h"
#include "intertask_interface.h"
#include "play_scenario.h"
#include "sctp_eNB_task.h"
#include "sctp_default_values.h"
#include "log.h"
//------------------------------------------------------------------------------
#define PLAY_SCENARIO              1
#define GS_IS_FILE                 1
#define GS_IS_DIR                  2
//------------------------------------------------------------------------------
Enb_properties_array_t g_enb_properties;
int                    g_max_speed = 0;
//------------------------------------------------------------------------------
extern et_scenario_t  *g_scenario;
extern int             xmlLoadExtDtdDefaultValue;
extern int             asn_debug;
extern int             asn1_xer_print;
extern pthread_mutex_t g_fsm_lock;

//------------------------------------------------------------------------------
// MEMO:
// Scenario with several eNBs: We may have to create ethx.y interfaces
//



//------------------------------------------------------------------------------
// test if file exist in current directory
int is_file_exists( const char const * file_nameP, const char const *file_roleP)
{
  struct stat s;
  int err = stat(file_nameP, &s);
  if(-1 == err) {
    if(ENOENT == errno) {
      fprintf(stderr, "Please provide a valid %s, %s does not exist\n", file_roleP, file_nameP);
    } else {
      perror("stat");
      exit(1);
    }
  } else {
    if(S_ISREG(s.st_mode)) {
      return GS_IS_FILE;
    } else if(S_ISDIR(s.st_mode)) {
      return GS_IS_DIR;
    } else {
      fprintf(stderr, "Please provide a valid test %s, %s exists but is not found valid\n", file_roleP, file_nameP);
    }
  }
  return 0;
}


//------------------------------------------------------------------------------
int et_strip_extension(char *in_filename)
{
  static const uint8_t name_min_len = 1;
  static const uint8_t max_ext_len = 5; // .pdml !
  fprintf(stdout, "strip_extension %s\n", in_filename);

  if (NULL != in_filename) {
    /* Check chars starting at end of string to find last '.' */
    for (ssize_t i = strlen(in_filename); i > name_min_len; i--) {
      if (in_filename[i] == '.') {
        in_filename[i] = '\0';
        return i;
      }
    }
  }
  return -1;
}
//------------------------------------------------------------------------------
// return number of splitted items
void et_get_shift_arg( char * line_argument, shift_packet_t * const shift)
{
  int  len       = strlen(line_argument);
  int  i         = 0;
  int  j         = 0;
  int  num_milli = 0;
  char my_num[64];
  int  negative  = 0;


  while ((line_argument[i] != ':') && (i < len)) {
    if (isdigit(line_argument[i])) { // may occur '\"'
      my_num[j++] = line_argument[i];
    }
    i += 1;
  }
  AssertFatal(':' == line_argument[i], "Bad format");
  i += 1; // ':'
  my_num[j++] = '\0';
  shift->frame_number = atoi(my_num);
  AssertFatal(i<len, "Shift argument %s bad format", line_argument);

  if (line_argument[i] == '-') {
    negative = 1;
    i += 1;
  } else if (line_argument[i] == '+') {
    i += 1;
  }
  AssertFatal(i<len, "Shift argument %s bad format", line_argument);
  j = 0;
  while ((line_argument[i] != '.') && (i < len)) {
    my_num[j++] = line_argument[i++];
  }
  my_num[j] = '\0';
  j = 0;
  i += 1;
  shift->shift_seconds = atoi(my_num);
  // may omit .mmm, accept .m or .mm or .mmm or ...
  while ((i < len) && (num_milli++ < 3)){
    my_num[j++] = line_argument[i++];
  }
  while (num_milli++ < 6){
    my_num[j++] = '0';
  }
  my_num[j] = '\0';
  shift->shift_microseconds = atoi(my_num);
  if (negative == 1) {
    shift->shift_seconds      = - shift->shift_seconds;
    shift->shift_microseconds = - shift->shift_microseconds;
  }
}
//------------------------------------------------------------------------------
// return number of splitted items
int split_path( char * pathP, char *** resP)
{
  char *  saveptr1;
  char *  p    = strtok_r (pathP, "/", &saveptr1);
  int     n_spaces = 0;

  /// split string and append tokens to 'res'
  while (p) {
    *resP = realloc (*resP, sizeof (char*) * ++n_spaces);
    AssertFatal (*resP, "realloc failed");
    (*resP)[n_spaces-1] = p;
    p = strtok_r (NULL, "/", &saveptr1);
  }
  return n_spaces;
}
//------------------------------------------------------------------------------
void et_free_packet(et_packet_t* packet)
{
  if (packet) {
    switch (packet->sctp_hdr.chunk_type) {
      case SCTP_CID_DATA:
        et_free_pointer(packet->sctp_hdr.u.data_hdr.payload.binary_stream);
        break;
      default:
        ;
    }
    et_free_pointer(packet);
  }
}

//------------------------------------------------------------------------------
void et_free_scenario(et_scenario_t* scenario)
{
  et_packet_t *packet = NULL;
  et_packet_t *next_packet = NULL;
  if (scenario) {
    packet = scenario->list_packet;
    while (packet) {
      next_packet = packet->next;
      et_free_packet(packet);
      packet = next_packet->next;
    }
    et_free_pointer(scenario);
    pthread_mutex_destroy(&g_fsm_lock);
  }
}

//------------------------------------------------------------------------------
char * et_ip2ip_str(const et_ip_t * const ip)
{
  static char str[INET6_ADDRSTRLEN];

  sprintf(str, "ERROR");
  switch (ip->address_family) {
    case AF_INET6:
      inet_ntop(AF_INET6, &(ip->address.ipv6), str, INET6_ADDRSTRLEN);
      break;
    case AF_INET:
      inet_ntop(AF_INET, &(ip->address.ipv4), str, INET_ADDRSTRLEN);
      break;
    default:
      ;
  }
  return str;
}
//------------------------------------------------------------------------------
//convert hexstring to len bytes of data
//returns 0 on success, negative on error
//data is a buffer of at least len bytes
//hexstring is upper or lower case hexadecimal, NOT prepended with "0x"
int et_hex2data(unsigned char * const data, const unsigned char * const hexstring, const unsigned int len)
{
  unsigned const char *pos = hexstring;
  char *endptr = NULL;
  size_t count = 0;

  fprintf(stdout, "%s(%s,%u)\n", __FUNCTION__, hexstring, len);

  if ((len > 1) && (strlen((const char*)hexstring) % 2)) {
    //or hexstring has an odd length
    return -3;
  }

  if (len == 1)  {
    char buf[5] = {'0', 'x', 0, pos[0], '\0'};
    data[0] = strtol(buf, &endptr, 16);
    /* Check for various possible errors */
    AssertFatal ((errno == 0) || (data[0] != 0), "ERROR %s() strtol: %s\n", __FUNCTION__, strerror(errno));
    AssertFatal (endptr != buf, "ERROR %s() No digits were found\n", __FUNCTION__);
    return 0;
  }

  for(count = 0; count < len/2; count++) {
    char buf[5] = {'0', 'x', pos[0], pos[1], 0};
    data[count] = strtol(buf, &endptr, 16);
    pos += 2 * sizeof(char);
    AssertFatal (endptr[0] == '\0', "ERROR %s() non-hexadecimal character encountered buf %p endptr %p buf %s count %zu pos %p\n", __FUNCTION__, buf, endptr, buf, count, pos);
    AssertFatal (endptr != buf, "ERROR %s() No digits were found\n", __FUNCTION__);
  }
  return 0;
}
//------------------------------------------------------------------------------
sctp_cid_t et_chunk_type_str2cid(const xmlChar * const chunk_type_str)
{
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"DATA")))              { return SCTP_CID_DATA;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"INIT")))              { return SCTP_CID_INIT;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"INIT_ACK")))          { return SCTP_CID_INIT_ACK;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"SACK")))              { return SCTP_CID_SACK;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"HEARTBEAT")))         { return SCTP_CID_HEARTBEAT;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"HEARTBEAT_ACK")))     { return SCTP_CID_HEARTBEAT_ACK;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ABORT")))             { return SCTP_CID_ABORT;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"SHUTDOWN")))          { return SCTP_CID_SHUTDOWN;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"SHUTDOWN_ACK")))      { return SCTP_CID_SHUTDOWN_ACK;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ERROR")))             { return SCTP_CID_ERROR;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"COOKIE_ECHO")))       { return SCTP_CID_COOKIE_ECHO;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"COOKIE_ACK")))        { return SCTP_CID_COOKIE_ACK;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ECN_ECNE")))          { return SCTP_CID_ECN_ECNE;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ECN_CWR")))           { return SCTP_CID_ECN_CWR;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"SHUTDOWN_COMPLETE"))) { return SCTP_CID_SHUTDOWN_COMPLETE;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"AUTH")))              { return SCTP_CID_AUTH;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"FWD_TSN")))           { return SCTP_CID_FWD_TSN;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ASCONF")))            { return SCTP_CID_ASCONF;}
  if ((!xmlStrcmp(chunk_type_str, (const xmlChar *)"ASCONF_ACK")))        { return SCTP_CID_ASCONF_ACK;}
  AssertFatal (0, "ERROR: %s() cannot convert: %s\n", __FUNCTION__, chunk_type_str);
}
//------------------------------------------------------------------------------
const char * const et_chunk_type_cid2str(const sctp_cid_t chunk_type)
{
  switch (chunk_type) {
    case  SCTP_CID_DATA:              return "DATA"; break;
    case  SCTP_CID_INIT:              return "INIT"; break;
    case  SCTP_CID_INIT_ACK:          return "INIT_ACK"; break;
    case  SCTP_CID_SACK:              return "SACK"; break;
    case  SCTP_CID_HEARTBEAT:         return "HEARTBEAT"; break;
    case  SCTP_CID_HEARTBEAT_ACK:     return "HEARTBEAT_ACK"; break;
    case  SCTP_CID_ABORT:             return "ABORT"; break;
    case  SCTP_CID_SHUTDOWN:          return "SHUTDOWN"; break;
    case  SCTP_CID_SHUTDOWN_ACK:      return "SHUTDOWN_ACK"; break;
    case  SCTP_CID_ERROR:             return "ERROR"; break;
    case  SCTP_CID_COOKIE_ECHO:       return "COOKIE_ECHO"; break;
    case  SCTP_CID_COOKIE_ACK:        return "COOKIE_ACK"; break;
    case  SCTP_CID_ECN_ECNE:          return "ECN_ECNE"; break;
    case  SCTP_CID_ECN_CWR:           return "ECN_CWR"; break;
    case  SCTP_CID_SHUTDOWN_COMPLETE: return "SHUTDOWN_COMPLETE"; break;
    case  SCTP_CID_AUTH:              return "AUTH"; break;
    case  SCTP_CID_FWD_TSN:           return "FWD_TSN"; break;
    case  SCTP_CID_ASCONF:            return "ASCONF"; break;
    case  SCTP_CID_ASCONF_ACK:        return "ASCONF_ACK"; break;
    default:
      AssertFatal (0, "ERROR: Unknown chunk_type %d!\n", chunk_type);
  }
}
//------------------------------------------------------------------------------
const char * const et_error_match2str(const int err)
{
  switch (err) {
    // from asn_compare.h
    case  COMPARE_ERR_CODE_NO_MATCH:                   return "CODE_NO_MATCH"; break;
    case  COMPARE_ERR_CODE_TYPE_MISMATCH:              return "TYPE_MISMATCH"; break;
    case  COMPARE_ERR_CODE_TYPE_ARG_NULL:              return "TYPE_ARG_NULL"; break;
    case  COMPARE_ERR_CODE_VALUE_NULL:                 return "VALUE_NULL"; break;
    case  COMPARE_ERR_CODE_VALUE_ARG_NULL:             return "VALUE_ARG_NULL"; break;
    case  COMPARE_ERR_CODE_CHOICE_NUM:                 return "CHOICE_NUM"; break;
    case  COMPARE_ERR_CODE_CHOICE_PRESENT:             return "CHOICE_PRESENT"; break;
    case  COMPARE_ERR_CODE_CHOICE_MALFORMED:           return "CHOICE_MALFORMED"; break;
    case  COMPARE_ERR_CODE_SET_MALFORMED:              return "SET_MALFORMED"; break;
    case  COMPARE_ERR_CODE_COLLECTION_NUM_ELEMENTS:    return "COLLECTION_NUM_ELEMENTS"; break;
    // from play_scenario.h
    case  ET_ERROR_MATCH_PACKET_SCTP_CHUNK_TYPE:       return "SCTP_CHUNK_TYPE"; break;
    case  ET_ERROR_MATCH_PACKET_SCTP_PPID:             return "SCTP_PPID"; break;
    case  ET_ERROR_MATCH_PACKET_SCTP_ASSOC_ID:         return "SCTP_ASSOC_ID"; break;
    case  ET_ERROR_MATCH_PACKET_SCTP_STREAM_ID:        return "SCTP_STREAM_ID"; break;
    case  ET_ERROR_MATCH_PACKET_SCTP_SSN:              return "SCTP_SSN"; break;
    case  ET_ERROR_MATCH_PACKET_S1AP_PRESENT:          return "S1AP_PRESENT"; break;
    case  ET_ERROR_MATCH_PACKET_S1AP_PROCEDURE_CODE:   return "S1AP_PROCEDURE_CODE"; break;
    case  ET_ERROR_MATCH_PACKET_S1AP_CRITICALITY:      return "S1AP_CRITICALITY"; break;
    default:
      AssertFatal (0, "ERROR: Unknown match error %d!(TODO handle an1c error codes)\n", err);
  }
}
//------------------------------------------------------------------------------
et_packet_action_t et_action_str2et_action_t(const xmlChar * const action)
{
  if ((!xmlStrcmp(action, (const xmlChar *)"SEND")))              { return ET_PACKET_ACTION_S1C_SEND;}
  if ((!xmlStrcmp(action, (const xmlChar *)"RECEIVE")))              { return ET_PACKET_ACTION_S1C_RECEIVE;}
  AssertFatal (0, "ERROR: cannot convert: %s\n", action);
  //if (NULL == action) {return ACTION_S1C_NULL;}
}
//------------------------------------------------------------------------------
void et_ip_str2et_ip(const xmlChar  * const ip_str, et_ip_t * const ip)
{
  AssertFatal (NULL != ip_str, "ERROR Cannot convert null string to ip address!\n");
  AssertFatal (NULL != ip,     "ERROR out parameter pointer is NULL!\n");
  // store this IP address in sa:
  if (inet_pton(AF_INET, (const char*)ip_str, (void*)&(ip->address.ipv4)) > 0) {
    ip->address_family = AF_INET;
    strncpy((char *)ip->str, (const char *)ip_str, INET_ADDRSTRLEN+1);
  } else if (inet_pton(AF_INET6, (const char*)ip_str, (void*)&(ip->address.ipv6)) > 0) {
    ip->address_family = AF_INET6;
    strncpy((char *)ip->str, (const char *)ip_str, INET6_ADDRSTRLEN+1);
  } else {
    ip->address_family = AF_UNSPEC;
    AssertFatal (0, "ERROR %s() Could not parse ip address %s!\n", __FUNCTION__, ip_str);
  }
}
//------------------------------------------------------------------------------
int et_compare_et_ip_to_net_ip_address(const et_ip_t * const ip, const net_ip_address_t * const net_ip)
{
  AssertFatal (NULL != ip,     "ERROR ip parameter\n");
  AssertFatal (NULL != net_ip, "ERROR net_ip parameter\n");
  switch (ip->address_family) {
    case AF_INET:
      if (net_ip->ipv4) {
        //S1AP_DEBUG("%s(%s,%s)=%d\n",__FUNCTION__,ip->str, net_ip->ipv4_address, strcmp(ip->str, net_ip->ipv4_address));
        return strcmp(ip->str, net_ip->ipv4_address);
      }
      //S1AP_DEBUG("%s(%s,%s)=-1 (IP version (4) not matching)\n",__FUNCTION__,ip->str, net_ip->ipv4_address);
      return -1;
      break;
    case AF_INET6:
      if (net_ip->ipv6) {
        //S1AP_DEBUG("%s(%s,%s)=%d\n",__FUNCTION__,ip->str, net_ip->ipv4_address, strcmp(ip->str, net_ip->ipv6_address));
        return strcmp(ip->str, net_ip->ipv6_address);
      }
      //S1AP_DEBUG("%s(%s,%s)=-1 (IP version (6) not matching)\n",__FUNCTION__,ip->str, net_ip->ipv6_address);
      return -1;
      break;
    default:
      S1AP_DEBUG("%s(%s,...)=-1 (unknown IP version)\n",__FUNCTION__,ip->str);
      return -1;
  }
}

#ifdef LIBCONFIG_LONG
#define libconfig_int long
#else
#define libconfig_int int
#endif
//------------------------------------------------------------------------------
void et_enb_config_init(const  char const * lib_config_file_name_pP)
//------------------------------------------------------------------------------
{
  config_t          cfg;
  config_setting_t *setting                       = NULL;
  config_setting_t *subsetting                    = NULL;
  config_setting_t *setting_mme_addresses         = NULL;
  config_setting_t *setting_mme_address           = NULL;
  config_setting_t *setting_enb                   = NULL;
  libconfig_int     my_int;
  int               num_enb_properties            = 0;
  int               enb_properties_index          = 0;
  int               num_enbs                      = 0;
  int               num_mme_address               = 0;
  int               i                             = 0;
  int               j                             = 0;
  int               parse_errors                  = 0;
  libconfig_int     enb_id                        = 0;
  const char*       cell_type                     = NULL;
  const char*       tac                           = 0;
  const char*       enb_name                      = NULL;
  const char*       mcc                           = 0;
  const char*       mnc                           = 0;
  char*             ipv4                          = NULL;
  char*             ipv6                          = NULL;
  char*             active                        = NULL;
  char*             preference                    = NULL;
  const char*       active_enb[MAX_ENB];
  char*             enb_interface_name_for_S1U    = NULL;
  char*             enb_ipv4_address_for_S1U      = NULL;
  libconfig_int     enb_port_for_S1U              = 0;
  char*             enb_interface_name_for_S1_MME = NULL;
  char*             enb_ipv4_address_for_S1_MME   = NULL;
  char             *address                       = NULL;
  char             *cidr                          = NULL;


  AssertFatal (lib_config_file_name_pP != NULL,
               "Bad parameter lib_config_file_name_pP %s , must reference a valid eNB config file\n",
               lib_config_file_name_pP);

  memset((char*)active_enb,     0 , MAX_ENB * sizeof(char*));

  config_init(&cfg);

  /* Read the file. If there is an error, report it and exit. */
  if (! config_read_file(&cfg, lib_config_file_name_pP)) {
    config_destroy(&cfg);
    AssertFatal (0, "Failed to parse eNB configuration file %s!\n", lib_config_file_name_pP);
  }

  // Get list of active eNBs, (only these will be configured)
  setting = config_lookup(&cfg, ENB_CONFIG_STRING_ACTIVE_ENBS);

  if (setting != NULL) {
    num_enbs = config_setting_length(setting);

    for (i = 0; i < num_enbs; i++) {
      setting_enb   = config_setting_get_elem(setting, i);
      active_enb[i] = config_setting_get_string (setting_enb);
      AssertFatal (active_enb[i] != NULL,
                   "Failed to parse config file %s, %dth attribute %s \n",
                   lib_config_file_name_pP, i, ENB_CONFIG_STRING_ACTIVE_ENBS);
      active_enb[i] = strdup(active_enb[i]);
      num_enb_properties += 1;
    }
  }

  /* Output a list of all eNBs. */
  setting = config_lookup(&cfg, ENB_CONFIG_STRING_ENB_LIST);

  if (setting != NULL) {
    enb_properties_index = g_enb_properties.number;
    parse_errors      = 0;
    num_enbs = config_setting_length(setting);

    for (i = 0; i < num_enbs; i++) {
      setting_enb = config_setting_get_elem(setting, i);

      if (! config_setting_lookup_int(setting_enb, ENB_CONFIG_STRING_ENB_ID, &enb_id)) {
        /* Calculate a default eNB ID */
# if defined(ENABLE_USE_MME)
        uint32_t hash;

        hash = et_s1ap_generate_eNB_id ();
        enb_id = i + (hash & 0xFFFF8);
# else
        enb_id = i;
# endif
      }

      if (  !(       config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_CELL_TYPE,           &cell_type)
                    && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_ENB_NAME,            &enb_name)
                    && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_TRACKING_AREA_CODE,  &tac)
                    && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE, &mcc)
                    && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_MOBILE_NETWORK_CODE, &mnc)


            )
        ) {
        AssertError (0, parse_errors ++,
                     "Failed to parse eNB configuration file %s, %d th enb\n",
                     lib_config_file_name_pP, i);
        continue; // FIXME this prevents segfaults below, not sure what happens after function exit
      }

      // search if in active list
      for (j=0; j < num_enb_properties; j++) {
        if (strcmp(active_enb[j], enb_name) == 0) {
          g_enb_properties.properties[enb_properties_index] = calloc(1, sizeof(Enb_properties_t));

          g_enb_properties.properties[enb_properties_index]->eNB_id   = enb_id;

          if (strcmp(cell_type, "CELL_MACRO_ENB") == 0) {
            g_enb_properties.properties[enb_properties_index]->cell_type = CELL_MACRO_ENB;
          } else  if (strcmp(cell_type, "CELL_HOME_ENB") == 0) {
            g_enb_properties.properties[enb_properties_index]->cell_type = CELL_HOME_ENB;
          } else {
            AssertError (0, parse_errors ++,
                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
                         lib_config_file_name_pP, i, cell_type);
          }

          g_enb_properties.properties[enb_properties_index]->eNB_name         = strdup(enb_name);
          g_enb_properties.properties[enb_properties_index]->tac              = (uint16_t)atoi(tac);
          g_enb_properties.properties[enb_properties_index]->mcc              = (uint16_t)atoi(mcc);
          g_enb_properties.properties[enb_properties_index]->mnc              = (uint16_t)atoi(mnc);
          g_enb_properties.properties[enb_properties_index]->mnc_digit_length = strlen(mnc);
          AssertFatal((g_enb_properties.properties[enb_properties_index]->mnc_digit_length == 2) ||
                      (g_enb_properties.properties[enb_properties_index]->mnc_digit_length == 3),
                      "BAD MNC DIGIT LENGTH %d",
                      g_enb_properties.properties[i]->mnc_digit_length);


          setting_mme_addresses = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_MME_IP_ADDRESS);
          num_mme_address     = config_setting_length(setting_mme_addresses);
          g_enb_properties.properties[enb_properties_index]->nb_mme = 0;

          for (j = 0; j < num_mme_address; j++) {
            setting_mme_address = config_setting_get_elem(setting_mme_addresses, j);

            if (  !(
                   config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IPV4_ADDRESS, (const char **)&ipv4)
                   && config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IPV6_ADDRESS, (const char **)&ipv6)
                   && config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE, (const char **)&active)
                   && config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE, (const char **)&preference)
                 )
              ) {
              AssertError (0, parse_errors ++,
                           "Failed to parse eNB configuration file %s, %d th enb %d th mme address !\n",
                           lib_config_file_name_pP, i, j);
              continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
            }

            g_enb_properties.properties[enb_properties_index]->nb_mme += 1;

            g_enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv4_address = strdup(ipv4);
            g_enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv6_address = strdup(ipv6);

            if (strcmp(active, "yes") == 0) {
              g_enb_properties.properties[enb_properties_index]->mme_ip_address[j].active = 1;
            } // else { (calloc)

            if (strcmp(preference, "ipv4") == 0) {
              g_enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv4 = 1;
            } else if (strcmp(preference, "ipv6") == 0) {
              g_enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv6 = 1;
            } else if (strcmp(preference, "no") == 0) {
              g_enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv4 = 1;
              g_enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv6 = 1;
            }
          }
          // SCTP SETTING
          g_enb_properties.properties[enb_properties_index]->sctp_out_streams = SCTP_OUT_STREAMS;
          g_enb_properties.properties[enb_properties_index]->sctp_in_streams  = SCTP_IN_STREAMS;
          subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_SCTP_CONFIG);

          if (subsetting != NULL) {
            if ( (config_setting_lookup_int( subsetting, ENB_CONFIG_STRING_SCTP_INSTREAMS, &my_int) )) {
              g_enb_properties.properties[enb_properties_index]->sctp_in_streams = (uint16_t)my_int;
            }

            if ( (config_setting_lookup_int( subsetting, ENB_CONFIG_STRING_SCTP_OUTSTREAMS, &my_int) )) {
              g_enb_properties.properties[enb_properties_index]->sctp_out_streams = (uint16_t)my_int;
            }
          }


          // NETWORK_INTERFACES
          subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

          if (subsetting != NULL) {
            if (  (
                   config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME,
                                                 (const char **)&enb_interface_name_for_S1_MME)
                   && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME,
                                                    (const char **)&enb_ipv4_address_for_S1_MME)
                   && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U,
                                                    (const char **)&enb_interface_name_for_S1U)
                   && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U,
                                                    (const char **)&enb_ipv4_address_for_S1U)
                   && config_setting_lookup_int(subsetting, ENB_CONFIG_STRING_ENB_PORT_FOR_S1U,
                                                &enb_port_for_S1U)
                 )
              ) {
              g_enb_properties.properties[enb_properties_index]->enb_interface_name_for_S1U = strdup(enb_interface_name_for_S1U);
              cidr = enb_ipv4_address_for_S1U;
              address = strtok(cidr, "/");

              if (address) {
                IPV4_STR_ADDR_TO_INT_NWBO ( address, g_enb_properties.properties[enb_properties_index]->enb_ipv4_address_for_S1U, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );
              }

              g_enb_properties.properties[enb_properties_index]->enb_port_for_S1U = enb_port_for_S1U;

              g_enb_properties.properties[enb_properties_index]->enb_interface_name_for_S1_MME = strdup(enb_interface_name_for_S1_MME);
              cidr = enb_ipv4_address_for_S1_MME;
              address = strtok(cidr, "/");

              if (address) {
                IPV4_STR_ADDR_TO_INT_NWBO ( address, g_enb_properties.properties[enb_properties_index]->enb_ipv4_address_for_S1_MME, "BAD IP ADDRESS FORMAT FOR eNB S1_MME !\n" );
              }
            }
          } // if (subsetting != NULL) {
          enb_properties_index += 1;
        } // if (strcmp(active_enb[j], enb_name) == 0)
      } // for (j=0; j < num_enb_properties; j++)
    } // for (i = 0; i < num_enbs; i++)
  } //   if (setting != NULL) {

  g_enb_properties.number += num_enb_properties;


  AssertFatal (parse_errors == 0,
               "Failed to parse eNB configuration file %s, found %d error%s !\n",
               lib_config_file_name_pP, parse_errors, parse_errors > 1 ? "s" : "");
}
/*------------------------------------------------------------------------------*/
const Enb_properties_array_t *et_enb_config_get(void)
{
  return &g_enb_properties;
}
/*------------------------------------------------------------------------------*/
void et_eNB_app_register(const Enb_properties_array_t *enb_properties)
{
  uint32_t         enb_id = 0;
  uint32_t         mme_id = 0;
  MessageDef      *msg_p  = NULL;
  char            *str    = NULL;
  struct in_addr   addr   = {.s_addr = 0};


  g_scenario->register_enb_pending = 0;
  for (enb_id = 0; (enb_id < enb_properties->number) ; enb_id++) {
    {
      s1ap_register_enb_req_t *s1ap_register_eNB = NULL;

      /* note:  there is an implicit relationship between the data structure and the message name */
      msg_p = itti_alloc_new_message (TASK_ENB_APP, S1AP_REGISTER_ENB_REQ);

      s1ap_register_eNB = &S1AP_REGISTER_ENB_REQ(msg_p);

      /* Some default/random parameters */
      s1ap_register_eNB->eNB_id           = enb_properties->properties[enb_id]->eNB_id;
      s1ap_register_eNB->cell_type        = enb_properties->properties[enb_id]->cell_type;
      s1ap_register_eNB->eNB_name         = enb_properties->properties[enb_id]->eNB_name;
      s1ap_register_eNB->tac              = enb_properties->properties[enb_id]->tac;
      s1ap_register_eNB->mcc              = enb_properties->properties[enb_id]->mcc;
      s1ap_register_eNB->mnc              = enb_properties->properties[enb_id]->mnc;
      s1ap_register_eNB->mnc_digit_length = enb_properties->properties[enb_id]->mnc_digit_length;

      s1ap_register_eNB->nb_mme =         enb_properties->properties[enb_id]->nb_mme;
      AssertFatal (s1ap_register_eNB->nb_mme <= S1AP_MAX_NB_MME_IP_ADDRESS, "Too many MME for eNB %d (%d/%d)!", enb_id, s1ap_register_eNB->nb_mme,
                   S1AP_MAX_NB_MME_IP_ADDRESS);

      for (mme_id = 0; mme_id < s1ap_register_eNB->nb_mme; mme_id++) {
        s1ap_register_eNB->mme_ip_address[mme_id].ipv4 = enb_properties->properties[enb_id]->mme_ip_address[mme_id].ipv4;
        s1ap_register_eNB->mme_ip_address[mme_id].ipv6 = enb_properties->properties[enb_id]->mme_ip_address[mme_id].ipv6;
        strncpy (s1ap_register_eNB->mme_ip_address[mme_id].ipv4_address,
                 enb_properties->properties[enb_id]->mme_ip_address[mme_id].ipv4_address,
                 sizeof(s1ap_register_eNB->mme_ip_address[0].ipv4_address));
        strncpy (s1ap_register_eNB->mme_ip_address[mme_id].ipv6_address,
                 enb_properties->properties[enb_id]->mme_ip_address[mme_id].ipv6_address,
                 sizeof(s1ap_register_eNB->mme_ip_address[0].ipv6_address));
      }

      s1ap_register_eNB->sctp_in_streams       = enb_properties->properties[enb_id]->sctp_in_streams;
      s1ap_register_eNB->sctp_out_streams      = enb_properties->properties[enb_id]->sctp_out_streams;


      s1ap_register_eNB->enb_ip_address.ipv6 = 0;
      s1ap_register_eNB->enb_ip_address.ipv4 = 1;
      addr.s_addr = enb_properties->properties[enb_id]->enb_ipv4_address_for_S1_MME;
      str = inet_ntoa(addr);
      strcpy(s1ap_register_eNB->enb_ip_address.ipv4_address, str);

      g_scenario->register_enb_pending++;
      itti_send_msg_to_task (TASK_S1AP, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
    }
  }
}
/*------------------------------------------------------------------------------*/
void *et_eNB_app_task(void *args_p)
{
  et_scenario_t                  *scenario = (et_scenario_t*)args_p;
  MessageDef                     *msg_p           = NULL;
  const char                     *msg_name        = NULL;
  instance_t                      instance        = 0;
  int                             result          = 0;

  itti_mark_task_ready (TASK_ENB_APP);

  do {
    // Wait for a message
    itti_receive_msg (TASK_ENB_APP, &msg_p);

    msg_name = ITTI_MSG_NAME (msg_p);
    instance = ITTI_MSG_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      itti_exit_task ();
      break;

    case S1AP_REGISTER_ENB_CNF:
      LOG_I(ENB_APP, "[eNB %d] Received %s: associated MME %d\n", instance, msg_name,
            S1AP_REGISTER_ENB_CNF(msg_p).nb_mme);

      DevAssert(scenario->register_enb_pending > 0);
      scenario->register_enb_pending--;

      /* Check if at least eNB is registered with one MME */
      if (S1AP_REGISTER_ENB_CNF(msg_p).nb_mme > 0) {
        scenario->registered_enb++;
      }

      /* Check if all register eNB requests have been processed */
      if (scenario->register_enb_pending == 0) {
        timer_remove(scenario->enb_register_retry_timer_id);
        if (scenario->registered_enb == scenario->enb_properties->number) {
          /* If all eNB are registered, start scenario */
          LOG_D(ENB_APP, " All eNB are now associated with a MME\n");
          et_event_t event;
          event.code = ET_EVENT_S1C_CONNECTED;
          et_scenario_fsm_notify_event(event);
        } else {
          uint32_t not_associated = scenario->enb_properties->number - scenario->registered_enb;

          LOG_W(ENB_APP, " %d eNB %s not associated with a MME, retrying registration in %d seconds ...\n",
                not_associated, not_associated > 1 ? "are" : "is", ET_ENB_REGISTER_RETRY_DELAY);

          /* Restart the eNB registration process in ENB_REGISTER_RETRY_DELAY seconds */
          if (timer_setup (ET_ENB_REGISTER_RETRY_DELAY, 0, TASK_ENB_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
                           NULL, &scenario->enb_register_retry_timer_id) < 0) {
            LOG_E(ENB_APP, " Can not start eNB register retry timer, use \"sleep\" instead!\n");

            sleep(ET_ENB_REGISTER_RETRY_DELAY);
            /* Restart the registration process */
            scenario->registered_enb = 0;
            et_eNB_app_register (scenario->enb_properties);
          }
        }
      }

      break;

    case S1AP_DEREGISTERED_ENB_IND:
      LOG_W(ENB_APP, "[eNB %d] Received %s: associated MME %d\n", instance, msg_name,
            S1AP_DEREGISTERED_ENB_IND(msg_p).nb_mme);

      /* TODO handle recovering of registration */
      break;

    case TIMER_HAS_EXPIRED:
      LOG_I(ENB_APP, " Received %s: timer_id %d\n", msg_name, TIMER_HAS_EXPIRED(msg_p).timer_id);

      if (TIMER_HAS_EXPIRED (msg_p).timer_id == scenario->enb_register_retry_timer_id) {
        /* Restart the registration process */
        scenario->registered_enb = 0;
        et_eNB_app_register (scenario->enb_properties);
      }
      break;

    default:
      LOG_E(ENB_APP, "Received unexpected message %s\n", msg_name);
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);
  return NULL;
}

//------------------------------------------------------------------------------
int et_play_scenario(et_scenario_t* const scenario, const struct shift_packet_s *shifts)
{
  et_event_t             event;
  struct shift_packet_s *shift                 = shifts;
  et_packet_t           *packet                = NULL;
  et_packet_t           *next_packet           = NULL;
  struct timeval         shift_all_packets     = { .tv_sec = 0, .tv_usec = 0 };
  struct timeval         relative_last_sent_packet     = { .tv_sec = 0, .tv_usec = 0 };
  struct timeval         relative_last_received_packet = { .tv_sec = 0, .tv_usec = 0 };
  struct timeval         initial_time          = { .tv_sec = 0, .tv_usec = 0 };
  char                   first_packet          = 1;
  char                   first_sent_packet     = 1;
  char                   first_received_packet = 1;

  // first apply timing shifts if requested
  while (shift) {
    packet = scenario->list_packet;
    while (packet) {
      //fprintf(stdout, "*shift: %p\n", shift);
      //fprintf(stdout, "\tframe_number:       %d\n", shift->frame_number);
      //fprintf(stdout, "\tshift_seconds:      %ld\n", shift->shift_seconds);
      //fprintf(stdout, "\tshift_microseconds: %ld\n", shift->shift_microseconds);
      //fprintf(stdout, "\tsingle:             %d\n\n", shift->single);
      //fprintf(stdout, "\tshift_all_packets_seconds:      %ld\n", shift_all_packets.tv_sec);
      //fprintf(stdout, "\tshift_all_packets_microseconds: %ld\n", shift_all_packets.tv_usec);

      AssertFatal((packet->time_relative_to_first_packet.tv_sec >= 0) && (packet->time_relative_to_first_packet.tv_usec >= 0),
          "Bad timing result time_relative_to_first_packet=%d.%d packet num %u, original frame number %u",
          packet->time_relative_to_first_packet.tv_sec,
          packet->time_relative_to_first_packet.tv_usec,
          packet->packet_number,
          packet->original_frame_number);
      AssertFatal((packet->time_relative_to_last_received_packet.tv_sec >= 0) && (packet->time_relative_to_last_received_packet.tv_usec >= 0),
          "Bad timing result time_relative_to_last_received_packet=%d.%d packet num %u, original frame number %u",
          packet->time_relative_to_last_received_packet.tv_sec,
          packet->time_relative_to_last_received_packet.tv_usec,
          packet->packet_number,
          packet->original_frame_number);
      AssertFatal((packet->time_relative_to_last_sent_packet.tv_sec >= 0) && (packet->time_relative_to_last_sent_packet.tv_usec >= 0),
          "Bad timing result time_relative_to_last_sent_packet=%d.%d packet num %u, original frame number %u",
          packet->time_relative_to_last_sent_packet.tv_sec,
          packet->time_relative_to_last_sent_packet.tv_usec,
          packet->packet_number,
          packet->original_frame_number);
//      fprintf(stdout, "\tpacket num %u, original frame number %u time_relative_to_first_packet=%d.%d\n",
//          packet->packet_number,
//          packet->original_frame_number,
//          packet->time_relative_to_first_packet.tv_sec,
//          packet->time_relative_to_first_packet.tv_usec);
//      fprintf(stdout, "\tpacket num %u, original frame number %u time_relative_to_last_received_packet=%d.%d\n",
//          packet->packet_number,
//          packet->original_frame_number,
//          packet->time_relative_to_last_received_packet.tv_sec,
//          packet->time_relative_to_last_received_packet.tv_usec);
//      fprintf(stdout, "\tpacket num %u, original frame number %u time_relative_to_last_sent_packet=%d.%d\n",
//          packet->packet_number,
//          packet->original_frame_number,
//          packet->time_relative_to_last_sent_packet.tv_sec,
//          packet->time_relative_to_last_sent_packet.tv_usec);

      if ((shift->single) && (shift->frame_number == packet->original_frame_number)) {
        struct timeval t_offset     = { .tv_sec = shift->shift_seconds, .tv_usec = shift->shift_microseconds };
        et_packet_shift_timing(packet, &t_offset);
        next_packet = packet->next;
        if (next_packet) {
          t_offset.tv_sec  = -t_offset.tv_sec;
          t_offset.tv_usec = -t_offset.tv_usec;

          if (packet->action == ET_PACKET_ACTION_S1C_SEND) {
            timeval_add(&next_packet->time_relative_to_last_sent_packet, &next_packet->time_relative_to_last_sent_packet, &t_offset);
          } else if (packet->action == ET_PACKET_ACTION_S1C_RECEIVE) {
            timeval_add(&next_packet->time_relative_to_last_received_packet, &next_packet->time_relative_to_last_received_packet, &t_offset);
          }
        }
      }
      if ((0 == shift->single) && (shift->frame_number == packet->original_frame_number)) {
        shift_all_packets.tv_sec = shift->shift_seconds;
        shift_all_packets.tv_usec = shift->shift_microseconds;
        timeval_add(&packet->time_relative_to_first_packet, &packet->time_relative_to_first_packet, &shift_all_packets);
        fprintf(stdout, "\tpacket num %u, now original frame number %u time_relative_to_first_packet=%d.%d\n",
            packet->packet_number,
            packet->original_frame_number,
            packet->time_relative_to_first_packet.tv_sec,
            packet->time_relative_to_first_packet.tv_usec);
        AssertFatal((packet->time_relative_to_first_packet.tv_sec >= 0) && (packet->time_relative_to_first_packet.tv_usec >= 0),
            "Bad timing result time_relative_to_first_packet=%d.%d packet num %u, original frame number %u",
            packet->time_relative_to_first_packet.tv_sec,
            packet->time_relative_to_first_packet.tv_usec,
            packet->packet_number,
            packet->original_frame_number);
      } else if ((0 == shift->single)  && (shift->frame_number < packet->original_frame_number)) {
        timeval_add(&packet->time_relative_to_first_packet, &packet->time_relative_to_first_packet, &shift_all_packets);
        fprintf(stdout, "\tpacket num %u, now original frame number %u time_relative_to_first_packet=%d.%d\n",
            packet->packet_number,
            packet->original_frame_number,
            packet->time_relative_to_first_packet.tv_sec,
            packet->time_relative_to_first_packet.tv_usec);
        AssertFatal((packet->time_relative_to_first_packet.tv_sec >= 0) && (packet->time_relative_to_first_packet.tv_usec >= 0),
            "Bad timing result time_relative_to_first_packet=%d.%d packet num %u, original frame number %u",
            packet->time_relative_to_first_packet.tv_sec,
            packet->time_relative_to_first_packet.tv_usec,
            packet->packet_number,
            packet->original_frame_number);
      }
      packet = packet->next;
    }
    shift = shift->next;
  }
  // now recompute time_relative_to_last_received_packet, time_relative_to_last_sent_packet
  packet = scenario->list_packet;
  while (packet) {
    if (first_packet > 0) {
      initial_time = packet->time_relative_to_first_packet;
      packet->time_relative_to_first_packet.tv_sec  = 0;
      packet->time_relative_to_first_packet.tv_usec = 0;
      first_packet = 0;
    } else {
      timersub(&packet->time_relative_to_first_packet, &initial_time,
          &packet->time_relative_to_first_packet);
    }
    if (packet->action == ET_PACKET_ACTION_S1C_SEND) {
      if (first_sent_packet > 0) {
        relative_last_sent_packet = packet->time_relative_to_first_packet;
        packet->time_relative_to_last_sent_packet.tv_sec  = 0;
        packet->time_relative_to_last_sent_packet.tv_usec = 0;
        first_sent_packet = 0;
      } else {
        timersub(&packet->time_relative_to_first_packet, &relative_last_sent_packet,
            &packet->time_relative_to_last_sent_packet);
        relative_last_sent_packet = packet->time_relative_to_first_packet;
      }
      if (first_received_packet > 0) {
        packet->time_relative_to_last_received_packet.tv_sec  = 0;
        packet->time_relative_to_last_received_packet.tv_usec = 0;
      } else {
        timersub(&packet->time_relative_to_first_packet, &relative_last_received_packet,
            &packet->time_relative_to_last_received_packet);
      }
    } else if (packet->action == ET_PACKET_ACTION_S1C_RECEIVE) {
      if (first_received_packet > 0) {
        relative_last_received_packet.tv_sec = packet->time_relative_to_first_packet.tv_sec;
        relative_last_received_packet.tv_usec = packet->time_relative_to_first_packet.tv_usec;
        packet->time_relative_to_last_received_packet.tv_sec  = 0;
        packet->time_relative_to_last_received_packet.tv_usec = 0;
        first_received_packet = 0;
      } else {
        timersub(&packet->time_relative_to_first_packet, &relative_last_received_packet,
            &packet->time_relative_to_last_received_packet);
        relative_last_received_packet = packet->time_relative_to_first_packet;
      }
      if (first_sent_packet > 0) {
        packet->time_relative_to_last_sent_packet.tv_sec  = 0;
        packet->time_relative_to_last_sent_packet.tv_usec = 0;
      } else {
        timersub(&packet->time_relative_to_first_packet, &relative_last_sent_packet,
            &packet->time_relative_to_last_sent_packet);
      }
    }
    packet = packet->next;
  }
  //et_display_scenario(scenario);

  // create SCTP ITTI task: same as eNB code
  if (itti_create_task (TASK_SCTP, sctp_eNB_task, NULL) < 0) {
    LOG_E(SCTP, "Create task for SCTP failed\n");
    return -1;
  }

  // create S1AP ITTI task: not as same as eNB code
  if (itti_create_task (TASK_S1AP, et_s1ap_eNB_task, NULL) < 0) {
    LOG_E(S1AP, "Create task for S1AP failed\n");
    return -1;
  }

  // create ENB_APP ITTI task: not as same as eNB code
  if (itti_create_task (TASK_ENB_APP, et_eNB_app_task, scenario) < 0) {
    LOG_E(ENB_APP, "Create task for ENB_APP failed\n");
    return -1;
  }

  event.code = ET_EVENT_INIT;
  event.u.init.scenario = scenario;
  et_scenario_fsm_notify_event(event);


  return 0;
}

//------------------------------------------------------------------------------
static void et_usage (
    int argc,
    char *argv[])
//------------------------------------------------------------------------------
{
  fprintf (stdout, "Please report any bug to: %s\n",PACKAGE_BUGREPORT);
  fprintf (stdout, "Usage: %s [options]\n\n", argv[0]);
  fprintf (stdout, "\n");
  fprintf (stdout, "\t-d | --test-dir       <dir>                  Directory where a set of files related to a particular test are located\n");
  fprintf (stdout, "\t-c | --enb-conf-file  <file>                 Provide an eNB config file, valid for the testbed\n");
  fprintf (stdout, "\t-D | --delay-on-exit  <delay-in-sec>         Wait delay-in-sec before exiting\n");
  fprintf (stdout, "\t-f | --shift-packet   <frame:[+|-]seconds[.usec]> Shift the timing of a packet'\n");
  fprintf (stdout, "\t-F | --shift-packets  <frame:[+|-]seconds[.usec]> Shift the timing of packets starting at frame 'frame' included\n");
  fprintf (stdout, "\t-m | --max-speed                             Play scenario as fast as possible without respecting frame timings\n");
  fprintf (stdout, "\t-s | --scenario       <file>                 File name (with no path) of a test scenario that has to be replayed ()\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "Other options:\n");
  fprintf (stdout, "\t-h | --help                                  Print this help and return\n");
  fprintf (stdout, "\t-v | --version                               Print informations about the version of this executable\n");
  fprintf (stdout, "\n");
}

//------------------------------------------------------------------------------
int
et_config_parse_opt_line (
  int argc,
  char *argv[],
  char **et_dir_name,
  char **scenario_file_name,
  char **enb_config_file_name,
  shift_packet_t **shifts,
  int *delay_on_exit)
//------------------------------------------------------------------------------
{
  int                 option   = 0;
  int                 rv       = 0;
  shift_packet_t      *shift   = NULL;

  enum long_option_e {
    LONG_OPTION_START = 0x100, /* Start after regular single char options */
    LONG_OPTION_ENB_CONF_FILE,
    LONG_OPTION_SCENARIO_FILE,
    LONG_OPTION_MAX_SPEED,
    LONG_OPTION_TEST_DIR,
    LONG_OPTION_DELAY_EXIT,
    LONG_OPTION_SHIFT_PACKET,
    LONG_OPTION_SHIFT_PACKETS,
    LONG_OPTION_HELP,
    LONG_OPTION_VERSION
  };

  static struct option long_options[] = {
    {"enb-conf-file",  required_argument, 0, LONG_OPTION_ENB_CONF_FILE},
    {"scenario ",      required_argument, 0, LONG_OPTION_SCENARIO_FILE},
    {"max-speed ",     no_argument,       0, LONG_OPTION_MAX_SPEED},
    {"test-dir",       required_argument, 0, LONG_OPTION_TEST_DIR},
    {"delay-on-exit",  required_argument, 0, LONG_OPTION_DELAY_EXIT},
    {"shift-packet",   required_argument, 0, LONG_OPTION_SHIFT_PACKET},
    {"shift-packets",  required_argument, 0, LONG_OPTION_SHIFT_PACKETS},
    {"help",           no_argument,       0, LONG_OPTION_HELP},
    {"version",        no_argument,       0, LONG_OPTION_VERSION},
     {NULL, 0, NULL, 0}
  };

  /*
   * Parsing command line
   */
  while ((option = getopt_long (argc, argv, "vhmc:s:d:f:F", long_options, NULL)) != -1) {
    switch (option) {
      case LONG_OPTION_ENB_CONF_FILE:
      case 'c':
        if (optarg) {
          *enb_config_file_name = strdup(optarg);
          printf("eNB config file name is %s\n", *enb_config_file_name);
          rv |= PLAY_SCENARIO;
        }
        break;

      case LONG_OPTION_SCENARIO_FILE:
      case 's':
        if (optarg) {
          *scenario_file_name = strdup(optarg);
          printf("Scenario file name is %s\n", *scenario_file_name);
          rv |= PLAY_SCENARIO;
        }
        break;

      case LONG_OPTION_TEST_DIR:
      case 'd':
        if (optarg) {
          *et_dir_name = strdup(optarg);
          if (is_file_exists(*et_dir_name, "test dirname") != GS_IS_DIR) {
            fprintf(stderr, "Please provide a valid test dirname, %s is not a valid directory name\n", *et_dir_name);
            exit(1);
          }
          printf("Test dir name is %s\n", *et_dir_name);
        }
        break;

      case LONG_OPTION_DELAY_EXIT:
      case 'D':
        if (optarg) {
          delay_on_exit = atoi(optarg);
          if (0 > delay_on_exit) {
            fprintf(stderr, "Please provide a valid -D/--delay-on-exit argument, %s is not a valid value\n", delay_on_exit);
            exit(1);
          }
          printf("Delay on exit is %d\n", (int) delay_on_exit);
        }
        break;


      case LONG_OPTION_SHIFT_PACKET:
      case 'f':
        if (optarg) {
          if (NULL == *shifts) {
            shift = calloc(1, sizeof (*shift));
            *shifts = shift;
          } else {
            shift->next = calloc(1, sizeof (*shift));
            shift = shift->next;
          }
          shift->single = 1;
          printf("Arg Shift packet %s\n", optarg);
          et_get_shift_arg(optarg, shift);
        }
        break;

      case LONG_OPTION_SHIFT_PACKETS:
      case 'F':
        if (optarg) {
          if (NULL == *shifts) {
            shift = calloc(1, sizeof (*shift));
            *shifts = shift;
          } else {
            shift->next = calloc(1, sizeof (*shift));
            shift = shift->next;
          }
          et_get_shift_arg(optarg, shift);
          printf("Arg Shift packets %s\n", optarg);
        }
        break;

      case LONG_OPTION_MAX_SPEED:
      case 'm':
        g_max_speed = 1;
        break;

      case LONG_OPTION_VERSION:
      case 'v':
        printf("Version %s\n", PACKAGE_VERSION);
        exit (0);
        break;

      case LONG_OPTION_HELP:
      case 'h':
      default:
        et_usage (argc, argv);
        exit (0);
    }
  }
  if (NULL == *et_dir_name) {
    fprintf(stderr, "Please provide a valid test dirname\n");
    exit(1);
  }
  if (chdir(*et_dir_name) != 0) {
    fprintf(stderr, "ERROR: chdir %s returned %s\n", *et_dir_name, strerror(errno));
    exit(1);
  }
  if (rv & PLAY_SCENARIO) {
    if (NULL == *enb_config_file_name) {
      fprintf(stderr, "ERROR: please provide the original eNB config file name that should be in %s\n", *et_dir_name);
    }
    if (is_file_exists(*enb_config_file_name, "eNB config file") != GS_IS_FILE) {
      fprintf(stderr, "ERROR: original eNB config file name %s is not found in dir %s\n", *enb_config_file_name, *et_dir_name);
    }
    et_enb_config_init(*enb_config_file_name);

    if (NULL == *scenario_file_name) {
      fprintf(stderr, "ERROR: please provide the scenario file name that should be in %s\n", *et_dir_name);
    }
    if (is_file_exists(*scenario_file_name, "Scenario file") != GS_IS_FILE) {
      fprintf(stderr, "ERROR: Scenario file name %s is not found in dir %s\n", *scenario_file_name, *et_dir_name);
    }
  }
  return rv;
}

//------------------------------------------------------------------------------
int main( int argc, char **argv )
//------------------------------------------------------------------------------
{
  int              actions              = 0;
  char            *et_dir_name          = NULL;
  char            *scenario_file_name   = NULL;
  char            *enb_config_file_name = NULL;
  struct shift_packet_s *shifts         = NULL;
  int              ret                  = 0;
  int              delay_on_exit        = 0;
  et_scenario_t   *scenario             = NULL;
  char             play_scenario_filename[NAME_MAX];

  memset(play_scenario_filename, 0, sizeof(play_scenario_filename));

  // logging
  logInit();
  set_glog(LOG_TRACE, LOG_MED);

  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info);

  set_comp_log(ENB_APP, LOG_TRACE, LOG_MED, 1);
  set_comp_log(S1AP, LOG_TRACE, LOG_MED, 1);
  set_comp_log(SCTP, LOG_TRACE, LOG_FULL, 1);
  asn_debug      = 0;
  asn1_xer_print = 1;

  //parameters
  actions = et_config_parse_opt_line (argc, argv, &et_dir_name, &scenario_file_name, &enb_config_file_name, &shifts, &delay_on_exit); //Command-line options
  if  (actions & PLAY_SCENARIO) {
    if (et_generate_xml_scenario(et_dir_name, scenario_file_name,enb_config_file_name, play_scenario_filename) == 0) {
      if (NULL != (scenario = et_generate_scenario(play_scenario_filename))) {
        ret = et_play_scenario(scenario, shifts);
      } else {
        fprintf(stderr, "ERROR: Could not generate scenario from tsml file\n");
        ret = -1;
      }
    } else {
      fprintf(stderr, "ERROR: Could not generate tsml scenario from xml file\n");
      ret = -1;
    }
    et_free_pointer(et_dir_name);
    et_free_pointer(scenario_file_name);
    et_free_pointer(enb_config_file_name);
  }
  itti_wait_tasks_end();
  if (0 < delay_on_exit) {
    sleep(delay_on_exit);
  }
  return ret;
}


