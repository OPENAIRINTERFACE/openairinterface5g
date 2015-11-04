/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*
                                generate_scenario.c
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
#include <unistd.h>
#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxml/xmlIO.h>
#include <libxml/DOCBparser.h>
#include <libxml/xinclude.h>
#include <libxml/catalog.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "assertions.h"
#include "generate_scenario.h"
#include "s1ap_eNB.h"
#include "intertask_interface.h"

#define ENB_CONFIG_STRING_ACTIVE_ENBS                   "Active_eNBs"

#define ENB_CONFIG_STRING_ENB_LIST                      "eNBs"
#define ENB_CONFIG_STRING_ENB_ID                        "eNB_ID"
#define ENB_CONFIG_STRING_CELL_TYPE                     "cell_type"
#define ENB_CONFIG_STRING_ENB_NAME                      "eNB_name"

#define ENB_CONFIG_STRING_TRACKING_AREA_CODE            "tracking_area_code"
#define ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE           "mobile_country_code"
#define ENB_CONFIG_STRING_MOBILE_NETWORK_CODE           "mobile_network_code"


#define ENB_CONFIG_STRING_MME_IP_ADDRESS                "mme_ip_address"
#define ENB_CONFIG_STRING_MME_IPV4_ADDRESS              "ipv4"
#define ENB_CONFIG_STRING_MME_IPV6_ADDRESS              "ipv6"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE         "active"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE     "preference"

#define ENB_CONFIG_STRING_SCTP_CONFIG                    "SCTP"
#define ENB_CONFIG_STRING_SCTP_INSTREAMS                 "SCTP_INSTREAMS"
#define ENB_CONFIG_STRING_SCTP_OUTSTREAMS                "SCTP_OUTSTREAMS"

#define ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG     "NETWORK_INTERFACES"
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME "ENB_INTERFACE_NAME_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME   "ENB_IPV4_ADDRESS_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U    "ENB_INTERFACE_NAME_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U         "ENB_IPV4_ADDRESS_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_PORT_FOR_S1U              "ENB_PORT_FOR_S1U"

#define ENB_CONFIG_PROPERTIES_INDEX_OLD 0
#define ENB_CONFIG_PROPERTIES_INDEX_NEW 1

#define ENB_CONFIG_MAX_XSLT_PARAMS 32

Enb_properties_array_t g_enb_properties[2];
char                  *g_openair_dir      = NULL;
char                  *g_test_dir         = NULL;
char                  *g_pdml_in_origin   = NULL;
char                  *g_generic_scenario = NULL;
extern int             xmlLoadExtDtdDefaultValue;

#define GENERATE_PDML_FILE           1
#define GENERATE_GENERIC_SCENARIO    2
#define GENERATE_DEDICATED_SCENARIO  4

#define GS_IS_FILE           1
#define GS_IS_DIR            2

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
int strip_extension(char *in_filename)
{
  static const uint8_t name_min_len = 1;
  static const uint8_t max_ext_len = 5; // .pdml !
  fprintf(stdout, "strip_extension %s\n", in_filename);

  if (NULL != in_filename) {
    /* Check chars starting at end of string to find last '.' */
    for (ssize_t i = strlen(in_filename); i > (name_min_len + max_ext_len); i--) {
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
int generate_generic_scenario(const char const * test_nameP, const char const * pdml_in_basenameP)
//------------------------------------------------------------------------------
{
  //int fd_pdml_in;
  xsltStylesheetPtr cur = NULL;
  xmlDocPtr         doc, res;
  FILE             *generic_scenario_file = NULL;
  const char        generic_scenario_filename[NAME_MAX];
  const char       *params[2*ENB_CONFIG_MAX_XSLT_PARAMS];
  int               nb_params = 0;
  int               i,j;
  char              astring[1024];
  struct in_addr    addr;

  memset(generic_scenario_filename, 0, sizeof(generic_scenario_filename));
  memset(astring, 0, sizeof(astring));
  if (getcwd(astring, sizeof(astring)) != NULL) {
    fprintf(stdout, "working in %s directory\n", astring);
  } else {
    perror("getcwd() error");
    exit(1);
  }

  memset(astring, 0, sizeof(astring));
  strcat(astring, g_openair_dir);
  strcat(astring, "/openair3/TEST/EPC_TEST/generic_scenario.xsl");

  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue = 1;
  cur = xsltParseStylesheetFile(astring);
  if (NULL == cur) {
    AssertFatal (0, "Could not parse stylesheet file %s (check OPENAIR_DIR env variable)!\n", astring);
  } else {
    fprintf(stdout, "XSLT style sheet: %s\n", astring);
  }

  doc = xmlParseFile(pdml_in_basenameP);
  if (NULL == doc) {
    AssertFatal (0, "Could not parse pdml file %s!\n", pdml_in_basenameP);
  } else {
    fprintf(stdout, "pdml file: %s\n", pdml_in_basenameP);
  }
  params[nb_params++] = "test_name";
  sprintf(astring, "\"%s\"", test_nameP);
  params[nb_params++] = strdup(astring);

  for (i = 0; i < g_enb_properties[ENB_CONFIG_PROPERTIES_INDEX_OLD].number; i++) {
    // eNB S1-C IPv4 address
    sprintf(astring, "enb_s1c%d", i);
    params[nb_params++] = strdup(astring);
    addr.s_addr = g_enb_properties[ENB_CONFIG_PROPERTIES_INDEX_OLD].properties[i]->enb_ipv4_address_for_S1_MME;
    sprintf(astring, "\"%s\"", inet_ntoa(addr));
    params[nb_params++] = strdup(astring);

    // MME S1-C IPv4 address
    for (j = 0; j < g_enb_properties[ENB_CONFIG_PROPERTIES_INDEX_OLD].properties[i]->nb_mme; j++) {
      sprintf(astring, "mme_s1c%d_%d", i, j);
      params[nb_params++] = strdup(astring);
      AssertFatal (g_enb_properties[ENB_CONFIG_PROPERTIES_INDEX_OLD].properties[i]->mme_ip_address[j].ipv4_address,
          "Only support MME IPv4 address\n");
      sprintf(astring, "\"%s\"", g_enb_properties[ENB_CONFIG_PROPERTIES_INDEX_OLD].properties[i]->mme_ip_address[j].ipv4_address);
      params[nb_params++] = strdup(astring);
    }
  }
  params[nb_params] = NULL;
  res = xsltApplyStylesheet(cur, doc, params);
  if (NULL != res) {
    // since pdml filename is not relative (no path), just filename in current directory we can safely remove
    sprintf(generic_scenario_filename,"%s",pdml_in_basenameP);
    if (strip_extension(generic_scenario_filename) > 0) {
      strcat(generic_scenario_filename, "_generic_scenario.xml");
      generic_scenario_file = fopen( generic_scenario_filename, "w+");
      if (NULL != generic_scenario_file) {
        xsltSaveResultToFile(generic_scenario_file, res, cur);
        fclose(generic_scenario_file);
        fprintf(stdout, "Wrote generic scenario to %s\n", generic_scenario_filename);
      } else {
        fprintf(stderr, "Error in fopen(%s)\n", generic_scenario_filename);
      }
    } else {
      fprintf(stderr, "Error in strip_extension()\n");
    }
  } else {
    fprintf(stderr, "Error in xsltApplyStylesheet()\n");
  }
  xsltFreeStylesheet(cur);
  xmlFreeDoc(res);
  xmlFreeDoc(doc);

  xsltCleanupGlobals();
  xmlCleanupParser();
}

//------------------------------------------------------------------------------
static void enb_config_display(const boolean_t new_config_fileP)
//------------------------------------------------------------------------------
{
  int i;

  printf( "\n----------------------------------------------------------------------\n");
  printf( " %s ENB CONFIG FILE CONTENT LOADED:\n", new_config_fileP == ENB_CONFIG_PROPERTIES_INDEX_OLD ? "Old":"New");
  printf( "----------------------------------------------------------------------\n");
  for (i = 0; i < g_enb_properties[new_config_fileP].number; i++) {
    printf( "ENB CONFIG for instance %u:\n\n", i);
    printf( "\teNB name:           \t%s\n",g_enb_properties[new_config_fileP].properties[i]->eNB_name);
    printf( "\teNB ID:             \t%"PRIu32"\n",g_enb_properties[new_config_fileP].properties[i]->eNB_id);
    printf( "\tCell type:          \t%s\n",g_enb_properties[new_config_fileP].properties[i]->cell_type == CELL_MACRO_ENB ? "CELL_MACRO_ENB":"CELL_HOME_ENB");
    printf( "\tTAC:                \t%"PRIu16"\n",g_enb_properties[new_config_fileP].properties[i]->tac);
    printf( "\tMCC:                \t%"PRIu16"\n",g_enb_properties[new_config_fileP].properties[i]->mcc);

    if (g_enb_properties[new_config_fileP].properties[i]->mnc_digit_length == 3) {
      printf( "\tMNC:                \t%03"PRIu16"\n",g_enb_properties[new_config_fileP].properties[i]->mnc);
    } else {
      printf( "\tMNC:                \t%02"PRIu16"\n",g_enb_properties[new_config_fileP].properties[i]->mnc);
    }
    printf( "\n--------------------------------------------------------\n");
  }
}


#ifdef LIBCONFIG_LONG
#define libconfig_int long
#else
#define libconfig_int int
#endif
//------------------------------------------------------------------------------
void enb_config_init(const  char const * lib_config_file_name_pP, const boolean_t new_config_fileP)
//------------------------------------------------------------------------------
{
  config_t          cfg;
  config_setting_t *setting                       = NULL;
  config_setting_t *subsetting                    = NULL;
  config_setting_t *setting_mme_addresses         = NULL;
  config_setting_t *setting_mme_address           = NULL;
  config_setting_t *setting_enb                   = NULL;
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
  const char*       active_enb[EPC_TEST_SCENARIO_MAX_ENB];
  char*             enb_interface_name_for_S1U    = NULL;
  char*             enb_ipv4_address_for_S1U      = NULL;
  libconfig_int     enb_port_for_S1U              = 0;
  char*             enb_interface_name_for_S1_MME = NULL;
  char*             enb_ipv4_address_for_S1_MME   = NULL;
  char             *address                       = NULL;
  char             *cidr                          = NULL;

  AssertFatal ((new_config_fileP == 0)  ||  (new_config_fileP == 1),
               "Bad parameter new_config_fileP %d \n",
               new_config_fileP);
  AssertFatal (lib_config_file_name_pP != NULL,
               "Bad parameter lib_config_file_name_pP %s , must reference a valid eNB config file\n",
               lib_config_file_name_pP);

  memset((char*)active_enb,     0 , EPC_TEST_SCENARIO_MAX_ENB * sizeof(char*));

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
                   "Failed to parse config file %s, %uth attribute %s \n",
                   lib_config_file_name_pP, i, ENB_CONFIG_STRING_ACTIVE_ENBS);
      active_enb[i] = strdup(active_enb[i]);
      num_enb_properties += 1;
    }
  }

  /* Output a list of all eNBs. */
  setting = config_lookup(&cfg, ENB_CONFIG_STRING_ENB_LIST);

  if (setting != NULL) {
    enb_properties_index = g_enb_properties[new_config_fileP].number;
    parse_errors      = 0;
    num_enbs = config_setting_length(setting);

    for (i = 0; i < num_enbs; i++) {
      setting_enb = config_setting_get_elem(setting, i);

      if (! config_setting_lookup_int(setting_enb, ENB_CONFIG_STRING_ENB_ID, &enb_id)) {
        /* Calculate a default eNB ID */
# if defined(ENABLE_USE_MME)
        uint32_t hash;

        hash = s1ap_generate_eNB_id ();
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
                     "Failed to parse eNB configuration file %s, %u th enb\n",
                     lib_config_file_name_pP, i);
        continue; // FIXME this prevents segfaults below, not sure what happens after function exit
      }

      // search if in active list
      for (j=0; j < num_enb_properties; j++) {
        if (strcmp(active_enb[j], enb_name) == 0) {
          g_enb_properties[new_config_fileP].properties[enb_properties_index] = calloc(1, sizeof(Enb_properties_t));

          g_enb_properties[new_config_fileP].properties[enb_properties_index]->eNB_id   = enb_id;

          if (strcmp(cell_type, "CELL_MACRO_ENB") == 0) {
            g_enb_properties[new_config_fileP].properties[enb_properties_index]->cell_type = CELL_MACRO_ENB;
          } else  if (strcmp(cell_type, "CELL_HOME_ENB") == 0) {
            g_enb_properties[new_config_fileP].properties[enb_properties_index]->cell_type = CELL_HOME_ENB;
          } else {
            AssertError (0, parse_errors ++,
                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
                         lib_config_file_name_pP, i, cell_type);
          }

          g_enb_properties[new_config_fileP].properties[enb_properties_index]->eNB_name         = strdup(enb_name);
          g_enb_properties[new_config_fileP].properties[enb_properties_index]->tac              = (uint16_t)atoi(tac);
          g_enb_properties[new_config_fileP].properties[enb_properties_index]->mcc              = (uint16_t)atoi(mcc);
          g_enb_properties[new_config_fileP].properties[enb_properties_index]->mnc              = (uint16_t)atoi(mnc);
          g_enb_properties[new_config_fileP].properties[enb_properties_index]->mnc_digit_length = strlen(mnc);
          AssertFatal((g_enb_properties[new_config_fileP].properties[enb_properties_index]->mnc_digit_length == 2) ||
                      (g_enb_properties[new_config_fileP].properties[enb_properties_index]->mnc_digit_length == 3),
                      "BAD MNC DIGIT LENGTH %d",
                      g_enb_properties[new_config_fileP].properties[i]->mnc_digit_length);


          setting_mme_addresses = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_MME_IP_ADDRESS);
          num_mme_address     = config_setting_length(setting_mme_addresses);
          g_enb_properties[new_config_fileP].properties[enb_properties_index]->nb_mme = 0;

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
                           "Failed to parse eNB configuration file %s, %u th enb %u th mme address !\n",
                           lib_config_file_name_pP, i, j);
              continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
            }

            g_enb_properties[new_config_fileP].properties[enb_properties_index]->nb_mme += 1;

            g_enb_properties[new_config_fileP].properties[enb_properties_index]->mme_ip_address[j].ipv4_address = strdup(ipv4);
            g_enb_properties[new_config_fileP].properties[enb_properties_index]->mme_ip_address[j].ipv6_address = strdup(ipv6);

            if (strcmp(active, "yes") == 0) {
              g_enb_properties[new_config_fileP].properties[enb_properties_index]->mme_ip_address[j].active = 1;
            } // else { (calloc)

            if (strcmp(preference, "ipv4") == 0) {
              g_enb_properties[new_config_fileP].properties[enb_properties_index]->mme_ip_address[j].ipv4 = 1;
            } else if (strcmp(preference, "ipv6") == 0) {
              g_enb_properties[new_config_fileP].properties[enb_properties_index]->mme_ip_address[j].ipv6 = 1;
            } else if (strcmp(preference, "no") == 0) {
              g_enb_properties[new_config_fileP].properties[enb_properties_index]->mme_ip_address[j].ipv4 = 1;
              g_enb_properties[new_config_fileP].properties[enb_properties_index]->mme_ip_address[j].ipv6 = 1;
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
              g_enb_properties[new_config_fileP].properties[enb_properties_index]->enb_interface_name_for_S1U = strdup(enb_interface_name_for_S1U);
              cidr = enb_ipv4_address_for_S1U;
              address = strtok(cidr, "/");

              if (address) {
                IPV4_STR_ADDR_TO_INT_NWBO ( address, g_enb_properties[new_config_fileP].properties[enb_properties_index]->enb_ipv4_address_for_S1U, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );
              }

              g_enb_properties[new_config_fileP].properties[enb_properties_index]->enb_port_for_S1U = enb_port_for_S1U;

              g_enb_properties[new_config_fileP].properties[enb_properties_index]->enb_interface_name_for_S1_MME = strdup(enb_interface_name_for_S1_MME);
              cidr = enb_ipv4_address_for_S1_MME;
              address = strtok(cidr, "/");

              if (address) {
                IPV4_STR_ADDR_TO_INT_NWBO ( address, g_enb_properties[new_config_fileP].properties[enb_properties_index]->enb_ipv4_address_for_S1_MME, "BAD IP ADDRESS FORMAT FOR eNB S1_MME !\n" );
              }
            }
          } // if (subsetting != NULL) {
          enb_properties_index += 1;
        } // if (strcmp(active_enb[j], enb_name) == 0)
      } // for (j=0; j < num_enb_properties; j++)
    } // for (i = 0; i < num_enbs; i++)
  } //   if (setting != NULL) {

  g_enb_properties[new_config_fileP].number += num_enb_properties;


  AssertFatal (parse_errors == 0,
               "Failed to parse eNB configuration file %s, found %d error%s !\n",
               lib_config_file_name_pP, parse_errors, parse_errors > 1 ? "s" : "");
}


//------------------------------------------------------------------------------
static void usage (
    int argc,
    char *argv[])
//------------------------------------------------------------------------------
{
  fprintf (stdout, "Please report any bug to: openair4g-devel@lists.eurecom.fr\n\n");
  fprintf (stdout, "Usage: %s [options]\n\n", argv[0]);
  fprintf (stdout, "Mandatory options:\n");
  fprintf (stdout, "\t-t | --test-dir <dir>              Directory where a set of files related to a particular test are located\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "Available options:\n");
  fprintf (stdout, "\t-h | --help                        Print this help and return\n");
  fprintf (stdout, "\t-n | --new-enb-conf-file <file>    Provide an updated eNB config file for generating a copy of the original test\n");
  fprintf (stdout, "                                     This option is set as many times as there are some eNB config files in the original test\n");
  fprintf (stdout, "\t-o | --old-enb-conf-file <file>    Provide the old eNB config file for generating a copy of the original test\n");
  fprintf (stdout, "                                     This option is set as many times as there are some eNB config files in the original test\n");
  fprintf (stdout, "\t-p | --pdml             <file>     File name (with no path) of an original scenario that has to be reworked (IP addresses) with new testbed\n");
  fprintf (stdout, "\t-s | --scenario-generic <file>     File name (with no path) of a scenario that has to be customized (IP addresses) with new testbed\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "Example of generate_scenario use cases:  \n");
  fprintf (stdout, "\n");
  fprintf (stdout, "  Case 1: generate a generix xml scenario from a captured pcap file:        \n");
  fprintf (stdout, "           +---------------------+ \n");
  fprintf (stdout, "           |captured pcap-ng file| \n");
  fprintf (stdout, "           +----------+----------+ \n");
  fprintf (stdout, "                      |\n");
  fprintf (stdout, "             mme_test_s1_pcap2pdml \n");
  fprintf (stdout, "                      |\n");
  fprintf (stdout, "             +--------V----------+    +------------------------+\n");
  fprintf (stdout, "             |'pdml-in-orig' file|    |'old-enb-conf-file' file|\n");
  fprintf (stdout, "             +--------+----------+    +------------+-----------+\n");
  fprintf (stdout, "                      |                            |\n");
  fprintf (stdout, "                      +----------------------------+\n");
  fprintf (stdout, "                      |\n");
  fprintf (stdout, "      generate_scenario -t <dir> -p <file>  -o <file> \n");
  fprintf (stdout, "                      |\n");
  fprintf (stdout, "         +------------V--------------+\n");
  fprintf (stdout, "         +'xml-scenario-generic' file|\n");
  fprintf (stdout, "         +---------------------------+\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "  Case 2: generate a dedicated xml scenario from generic xml scenario:\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "         +---------------------------+    +------------------------+\n");
  fprintf (stdout, "         |'xml-scenario-generic' file|    |'new-enb-conf-file' file|\n");
  fprintf (stdout, "         +--------+------------------+    +------------+-----------+\n");
  fprintf (stdout, "                  |                                    |\n");
  fprintf (stdout, "                  +--------------------+---------------+\n");
  fprintf (stdout, "                                       |\n");
  fprintf (stdout, "                            generate_scenario -t <dir> -s <file> -n <file> \n");
  fprintf (stdout, "                                       |\n");
  fprintf (stdout, "                        +--------------V--------------+\n");
  fprintf (stdout, "                        +'xml-scenario-dedicated' file|\n");
  fprintf (stdout, "                        +-----------------------------+\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "  Case 3 = case 1 + case 2: generate a dedicated xml scenario from a captured pcap file:\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "                           +---------------------+\n");
  fprintf (stdout, "                           |captured pcap-ng file|\n");
  fprintf (stdout, "                           +----------+----------+\n");
  fprintf (stdout, "                                      |\n");
  fprintf (stdout, "                             mme_test_s1_pcap2pdml  \n");
  fprintf (stdout, "                                      |\n");
  fprintf (stdout, "  +------------------------+ +--------V----------+ +------------------------+\n");
  fprintf (stdout, "  |'old-enb-conf-file' file| |'pdml-in-orig' file| |'new-enb-conf-file' file|\n");
  fprintf (stdout, "  +------------+-----------+ +--------+----------+ +------------+-----------+\n");
  fprintf (stdout, "               |                      |                         |\n");
  fprintf (stdout, "               +----------------------+-------------------------+\n");
  fprintf (stdout, "                                      |\n");
  fprintf (stdout, "                                      V\n");
  fprintf (stdout, "                  generate_scenario -t <dir> -p <file>  -o <file>  -n <file> \n");
  fprintf (stdout, "                       |  ^                           |\n");
  fprintf (stdout, "                       |  |                           |\n");
  fprintf (stdout, "        +--------------V------------+    +------------V----------------+\n");
  fprintf (stdout, "        |'xml-scenario-generic' file|    |'xml-scenario-dedicated' file|\n");
  fprintf (stdout, "        +---------------------------+    +-----------------------------+\n");
}


//------------------------------------------------------------------------------
int
config_parse_opt_line (
  int argc,
  char *argv[])
//------------------------------------------------------------------------------
{
  int                           option;
  int                           rv                         = 0;
  char                         *new_enb_config_file_name   = NULL;
  char                         *old_enb_config_file_name   = NULL;
  char                         *pdml_in_file_name          = NULL;
  char                         *generic_scenario_file_name = NULL;
  char                         *test_dir_name              = NULL;

  enum long_option_e {
    LONG_OPTION_START = 0x100, /* Start after regular single char options */
    LONG_OPTION_NEW_ENB_CONF_FILE,
    LONG_OPTION_OLD_ENB_CONF_FILE,
    LONG_OPTION_PDML,
    LONG_OPTION_TEST_DIR,
    LONG_OPTION_SCENARIO_GENERIC,
    LONG_OPTION_HELP,
  };

  static struct option long_options[] = {
    {"old-enb-conf-file",      required_argument, 0, LONG_OPTION_OLD_ENB_CONF_FILE},
    {"new-enb-conf-file",      required_argument, 0, LONG_OPTION_NEW_ENB_CONF_FILE},
    {"pdml ",                  required_argument, 0, LONG_OPTION_PDML},
    {"test-dir",               required_argument, 0, LONG_OPTION_TEST_DIR},
    {"scenario-generic",       required_argument, 0, LONG_OPTION_SCENARIO_GENERIC},
    {"help",                   required_argument, 0, LONG_OPTION_HELP},
    {NULL, 0, NULL, 0}
  };

  /*
   * Parsing command line
   */
  while ((option = getopt_long (argc, argv, "hp:n:o:s:t:", long_options, NULL)) != -1) {
    switch (option) {
      case LONG_OPTION_OLD_ENB_CONF_FILE:
      case 'o':
        if (optarg) {
          old_enb_config_file_name = optarg;
          printf("Old eNB config file name is %s\n", old_enb_config_file_name);
          rv |= GENERATE_GENERIC_SCENARIO;
        }
        break;

      case LONG_OPTION_NEW_ENB_CONF_FILE:
      case 'n':
        if (optarg) {
          new_enb_config_file_name = optarg;
          printf("New eNB config file name is %s\n", new_enb_config_file_name);
          rv |= GENERATE_DEDICATED_SCENARIO;
        }
        break;

      case LONG_OPTION_PDML:
      case 'p':
        if (optarg) {
          pdml_in_file_name = strdup(optarg);
          printf("PDML input file name is %s\n", pdml_in_file_name);
          rv |= GENERATE_GENERIC_SCENARIO;
        }
        break;

      case LONG_OPTION_TEST_DIR:
      case 't':
        if (optarg) {
          test_dir_name = strdup(optarg);
          if (is_file_exists(test_dir_name, "test dirname") != GS_IS_DIR) {
            fprintf(stderr, "Please provide a valid test dirname, %s is not a valid directory name\n", test_dir_name);
            exit(1);
          }
          printf("Test dir name is %s\n", test_dir_name);
        }
        break;

      case LONG_OPTION_SCENARIO_GENERIC:
      case 's':
        if (optarg) {
          generic_scenario_file_name = strdup(optarg);
          printf("Generic XML scenario input file name is %s\n", generic_scenario_file_name);
          rv |= GENERATE_DEDICATED_SCENARIO;
        }
        break;

      case LONG_OPTION_HELP:
      case 'h':
      default:
        usage (argc, argv);
        exit (0);
    }
  }
  if (NULL == test_dir_name) {
    fprintf(stderr, "Please provide a valid test dirname\n");
    exit(1);
  }
  g_test_dir = test_dir_name; test_dir_name = NULL;
  if (chdir(g_test_dir) != 0) {
    fprintf(stderr, "Error: chdir %s returned %s\n", g_test_dir, strerror(errno));
    exit(1);
  }
  if (rv & GENERATE_GENERIC_SCENARIO) {
    if (NULL == old_enb_config_file_name) {
      fprintf(stderr, "Error: please provide the original eNB config file name that should be in %s\n", g_test_dir);
    }
    if (is_file_exists(old_enb_config_file_name, "Old eNB config file") != GS_IS_FILE) {
      fprintf(stderr, "Error: original eNB config file name %s is not found in dir %s\n", old_enb_config_file_name, g_test_dir);
    }
    enb_config_init(old_enb_config_file_name, ENB_CONFIG_PROPERTIES_INDEX_OLD);
    enb_config_display(ENB_CONFIG_PROPERTIES_INDEX_OLD);

    if (NULL == pdml_in_file_name) {
      fprintf(stderr, "Error: please provide the PDML file name that should be in %s\n", g_test_dir);
    }
    if (is_file_exists(pdml_in_file_name, "PDML file") != GS_IS_FILE) {
      fprintf(stderr, "Error: PDML file name %s is not found in dir %s\n", pdml_in_file_name, g_test_dir);
    }
    g_pdml_in_origin = pdml_in_file_name; pdml_in_file_name = NULL;
  }

  if (rv & GENERATE_DEDICATED_SCENARIO) {
    if (NULL == new_enb_config_file_name) {
      fprintf(stderr, "Error: please provide the new eNB config file name that should be in %s\n", g_test_dir);
    }
    if (is_file_exists(new_enb_config_file_name, "New eNB config file") != GS_IS_FILE) {
      fprintf(stderr, "Error: New eNB config file name %s is not found in dir %s\n", new_enb_config_file_name, g_test_dir);
    }
    enb_config_init(new_enb_config_file_name, ENB_CONFIG_PROPERTIES_INDEX_NEW);
    enb_config_display(ENB_CONFIG_PROPERTIES_INDEX_NEW);

    if (NULL == generic_scenario_file_name) {
      fprintf(stderr, "Error: please provide the Generic scenario file name that should be in %s\n", g_test_dir);
    }
    if (is_file_exists(generic_scenario_file_name, "Generic scenario file") != GS_IS_FILE) {
      fprintf(stderr, "Error: Generic scenario file name %s is not found in dir %s\n", generic_scenario_file_name, g_test_dir);
    }
    g_generic_scenario = generic_scenario_file_name; generic_scenario_file_name = NULL;
  }
  return rv;
}

//------------------------------------------------------------------------------
int main( int argc, char **argv )
//------------------------------------------------------------------------------
{
  int     actions = 0;

  g_openair_dir = getenv("OPENAIR_DIR");
  if (NULL == g_openair_dir) {
    fprintf(stderr, "Error: Could not get OPENAIR_DIR environment variable\n");
    exit(1);
  }

  memset((char*) &g_enb_properties[ENB_CONFIG_PROPERTIES_INDEX_OLD], 0 , sizeof(g_enb_properties[ENB_CONFIG_PROPERTIES_INDEX_OLD]));
  memset((char*) &g_enb_properties[ENB_CONFIG_PROPERTIES_INDEX_NEW], 0 , sizeof(g_enb_properties[ENB_CONFIG_PROPERTIES_INDEX_NEW]));

  actions = config_parse_opt_line (argc, argv); //Command-line options
  if  (actions & GENERATE_GENERIC_SCENARIO) {
    generate_generic_scenario(g_test_dir, g_pdml_in_origin);
  }
  /*if  (actions & GENERATE_DEDICATED_SCENARIO) {
    generate_dedicated_scenario(g_test_dir, g_pdml_in_origin);
  }*/

  return 0;
}
