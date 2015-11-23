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
#include <libconfig.h>
#include <inttypes.h>
#include <getopt.h>

#include "assertions.h"
#include "enb_config.h"
#include "s1ap_eNB.h"
#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#define EPC_TEST_SCENARIO_MAX_ENB                       2

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


Enb_properties_array_t g_enb_properties;

//------------------------------------------------------------------------------
static void enb_config_display(void)
//------------------------------------------------------------------------------
{
  int i,j;

  printf( "\n----------------------------------------------------------------------\n");
  printf( " ENB CONFIG FILE CONTENT LOADED (TBC):\n");
  printf( "----------------------------------------------------------------------\n");
  for (i = 0; i < g_enb_properties.number; i++) {
    printf( "ENB CONFIG for instance %u:\n\n", i);
    printf( "\teNB name:           \t%s:\n",g_enb_properties.properties[i]->eNB_name);
    printf( "\teNB ID:             \t%"PRIu32":\n",g_enb_properties.properties[i]->eNB_id);
    printf( "\tCell type:          \t%s:\n",g_enb_properties.properties[i]->cell_type == CELL_MACRO_ENB ? "CELL_MACRO_ENB":"CELL_HOME_ENB");
    printf( "\tTAC:                \t%"PRIu16":\n",g_enb_properties.properties[i]->tac);
    printf( "\tMCC:                \t%"PRIu16":\n",g_enb_properties.properties[i]->mcc);

    if (g_enb_properties.properties[i]->mnc_digit_length == 3) {
      printf( "\tMNC:                \t%03"PRIu16":\n",g_enb_properties.properties[i]->mnc);
    } else {
      printf( "\tMNC:                \t%02"PRIu16":\n",g_enb_properties.properties[i]->mnc);
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
const Enb_properties_array_t *enb_config_init(char* lib_config_file_name_pP)
//------------------------------------------------------------------------------
{
  config_t          cfg;
  config_setting_t *setting                       = NULL;
  config_setting_t *subsetting                    = NULL;
  config_setting_t *setting_srb1                  = NULL;
  config_setting_t *setting_mme_addresses         = NULL;
  config_setting_t *setting_mme_address           = NULL;
  config_setting_t *setting_enb                   = NULL;
  int               num_enb_properties            = 0;
  int               enb_properties_index          = 0;
  int               num_enbs                      = 0;
  int               num_mme_address               = 0;
  int               num_otg_elements              =0;
  int               num_component_carriers        =0;
  int               i                             = 0;
  int               j                             = 0;
  int               parse_errors                  = 0;
  libconfig_int     enb_id                        = 0;
  const char*       cell_type                     = NULL;
  const char*       tac                           = 0;
  const char*       enb_name                      = NULL;
  const char*       mcc                           = 0;
  const char*       mnc                           = 0;
  const char*       frame_type                    = NULL;
  const char*            prefix_type              = NULL;
  libconfig_int     Nid_cell                      = 0;

  libconfig_int     my_int;

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
  char             *astring                       = NULL;

  memset((char*)active_enb,     0 , EPC_TEST_SCENARIO_MAX_ENB * sizeof(char*));

  config_init(&cfg);

  if (lib_config_file_name_pP != NULL) {
    /* Read the file. If there is an error, report it and exit. */
    if (! config_read_file(&cfg, lib_config_file_name_pP)) {
      config_destroy(&cfg);
      AssertFatal (0, "Failed to parse eNB configuration file %s!\n", lib_config_file_name_pP);
    }
  } else {
    config_destroy(&cfg);
    AssertFatal (0, "No eNB configuration file provided!\n");
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
    enb_properties_index = 0;
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
                           "Failed to parse eNB configuration file %s, %u th enb %u th mme address !\n",
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
          }
        }
      }
    }
  }

  g_enb_properties.number = num_enb_properties;

  AssertError (enb_properties_index == num_enb_properties, parse_errors ++,
               "Failed to parse eNB configuration file %s, mismatch between %u active eNBs and %u corresponding defined eNBs !\n",
               lib_config_file_name_pP, num_enb_properties, enb_properties_index);

  AssertFatal (parse_errors == 0,
               "Failed to parse eNB configuration file %s, found %d error%s !\n",
               lib_config_file_name_pP, parse_errors, parse_errors > 1 ? "s" : "");
  enb_config_display();
  return &g_enb_properties;

}

//------------------------------------------------------------------------------
const Enb_properties_array_t *enb_config_get(void)
//------------------------------------------------------------------------------
{
  return &g_enb_properties;
}


//------------------------------------------------------------------------------
static void usage (
    int argc,
    char *argv[])
//------------------------------------------------------------------------------
{
  fprintf (stdout, "Please report any bug to: openair4g-devel@lists.eurecom.fr\n\n");
  fprintf (stdout, "Usage: %s [options]\n\n", argv[0]);
  fprintf (stdout, "Available options:\n");
  fprintf (stdout, "\t--help, -h          Print this help and return\n");
  fprintf (stdout, "\t--test-dir <path>\n");
  fprintf (stdout, "                      Set the test directory where pdml and original enb(s) config files are located\n");
  fprintf (stdout, "                      See README in openair3/TEST/EPC_TEST\n");
  fprintf (stdout, "\t--new-enb-conf-file <file>\n");
  fprintf (stdout, "                      Provide an updated eNB config file for generating a copy of the original test\n");
  fprintf (stdout, "                      This option is set as many times as there are some eNB in the original test\n");
}


//------------------------------------------------------------------------------
int
config_parse_opt_line (
  int argc,
  char *argv[])
//------------------------------------------------------------------------------
{
  int                           option;
  char                         *enb_config_file_name = NULL;
  char                         *test_dir             = NULL;

  enum long_option_e {
    LONG_OPTION_START = 0x100, /* Start after regular single char options */
    LONG_OPTION_TEST_DIR,
    LONG_OPTION_NEW_ENB_CONF_FILE,
    LONG_OPTION_HELP,
  };

  static struct option long_options[] = {
    {"test-dir",               required_argument, 0, LONG_OPTION_TEST_DIR},
    {"new-enb-conf-file",      required_argument, 0, LONG_OPTION_NEW_ENB_CONF_FILE},
    {"help",                   required_argument, 0, LONG_OPTION_HELP},
    {NULL, 0, NULL, 0}
  };

  /*
   * Parsing command line
   */
  while ((option = getopt_long (argc, argv, "h", long_options, NULL)) != -1) {
    switch (option) {
      case LONG_OPTION_TEST_DIR:
        if (optarg) {
          test_dir = strdup(optarg);
          printf("TEST DIRECTORY IS %s\n", test_dir);
        }
        break;

      case LONG_OPTION_NEW_ENB_CONF_FILE:
        if (optarg) {
          enb_config_file_name = strdup(optarg);
          printf("eNB config file name is %s\n", enb_config_file_name);
          enb_config_init(enb_config_file_name);
        }
        break;

        if (optarg) {
          test_dir = strdup(optarg);
          printf("TEST DIRECTORY IS %s\n", test_dir);
        }
        break;

      case LONG_OPTION_HELP:
      case 'h':                  /* Fall through */
      default:
        usage (argc, argv);
        exit (0);
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int main( int argc, char **argv )
//------------------------------------------------------------------------------
{
  memset((char*) &g_enb_properties, 0 , sizeof(g_enb_properties));
  config_parse_opt_line (argc, argv); //Command-line options

  return 0;
}
