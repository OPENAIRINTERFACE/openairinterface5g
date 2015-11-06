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
#include <unistd.h>
#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlIO.h>
#include <libxml/DOCBparser.h>
#include <libxml/xinclude.h>
#include <libxml/catalog.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "assertions.h"
#include "play_scenario.h"
#include "s1ap_eNB.h"
#include "intertask_interface.h"
#include "enb_config.h"
//------------------------------------------------------------------------------
#define ENB_CONFIG_MAX_XSLT_PARAMS 32
#define PLAY_SCENARIO              1
#define GS_IS_FILE                 1
#define GS_IS_DIR                  2
//------------------------------------------------------------------------------
char                  *g_openair_dir        = NULL;
//------------------------------------------------------------------------------
extern Enb_properties_array_t enb_properties;
extern int                    xmlLoadExtDtdDefaultValue;

//------------------------------------------------------------------------------

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
void display_node(xmlNodePtr node) {
  if (node->type == XML_ELEMENT_NODE) {
    xmlChar *path = xmlGetNodePath(node);
    if (node->children != NULL && node->children->type == XML_TEXT_NODE) {
      xmlChar *content = xmlNodeGetContent(node);
      printf("%s -> %s\n", path, content);
      xmlFree(content);
    } else {
      printf("%s\n", path);
    }
    xmlFree(path);
  }
}
//------------------------------------------------------------------------------
void free_scenario(test_scenario_t* scenario)
{
  //TODO
}
//------------------------------------------------------------------------------
sctp_cid_t chunk_type_str2cid(xmlChar *chunk_type_str)
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
  fprintf(stderr, "ERROR: Could not convert: %s\n", chunk_type_str);
  exit(-1);
}
//------------------------------------------------------------------------------
test_packet_t* parse_xml_packet(xmlNodePtr node) {

  test_packet_t *packet   = NULL;
  xmlNode       *cur_node = NULL;
  xmlChar       *xml_char = NULL;

  if (NULL != node) {
    packet = calloc(1, sizeof(*packet));

    for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
      if (cur_node->type == XML_ELEMENT_NODE) {
        printf("node type: Element, name: %s\n", cur_node->name);
        if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"frame.time_relative"))) {
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"ip.src"))) {
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"ip.dst"))) {
        } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"sctp.chunk_type_str"))) {
          xml_char = xmlGetProp(cur_node, (const xmlChar *)"value");
          packet->sctp_hdr.chunk_type = chunk_type_str2cid(xml_char);
          fprintf(stdout, "chunk_type_str2cid: %s\n", xml_char);
        }
      }
    }
  }
  return packet;
}
//------------------------------------------------------------------------------
int play_scenario(test_scenario_t* scenario) {
  //TODO
  return 0;
}
//------------------------------------------------------------------------------
test_scenario_t* generate_scenario(
    const char const * play_scenario_filename )
{
  xmlDocPtr         doc      = NULL;
  xmlNodePtr        root     = NULL;
  xmlNodePtr        node     = NULL;
  xmlChar          *xml_char = NULL;
  test_scenario_t  *scenario = NULL;
  test_packet_t    *packet   = NULL;
  test_packet_t   **next_packet   = NULL;

  doc = xmlParseFile(play_scenario_filename);
  if (NULL == doc) {
    AssertFatal (0, "Could not parse scenario xml file %s!\n", play_scenario_filename);
  } else {
    fprintf(stdout, "Test scenario file to play: %s\n", play_scenario_filename);
  }

  // Get root
  root = xmlDocGetRootElement(doc);
  if (NULL != root) {
    if ((!xmlStrcmp(root->name, (const xmlChar *)"scenario"))) {
      xml_char = xmlGetProp(root, (const xmlChar *)"name");
      printf("scenario name: %s\n", xml_char);
      scenario = calloc(1, sizeof(*scenario));
      scenario->name = xml_char; // nodup nofree
      next_packet = &scenario->list_packet;
      for (node = root->children; node != NULL; node = node->next) {
        if ((!xmlStrcmp(node->name, (const xmlChar *)"packet"))) {
          packet = parse_xml_packet(node);
          if (NULL != packet) {
            *next_packet = packet;
            next_packet = &packet->next;
          } else {
            fprintf(stdout, "WARNING omitted packet:\n");
            display_node(node);
          }
        }
      }
    }
  } else {
    fprintf(stderr, "Empty xml document\n");
  }
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return scenario;
}

//------------------------------------------------------------------------------
int generate_xml_scenario(
    const char const * test_dir_name,
    const char const * test_scenario_filename,
    const char const * enb_config_filename,
          char const * play_scenario_filename /* OUT PARAM*/)
//------------------------------------------------------------------------------
{
  //int fd_pdml_in;
  xsltStylesheetPtr cur = NULL;
  xmlDocPtr         doc, res;
  FILE             *play_scenario_file = NULL;
  const char       *params[2*ENB_CONFIG_MAX_XSLT_PARAMS];
  int               nb_params = 0;
  int               i,j;
  char              astring[1024];
  char             *astring2 = NULL;
  struct in_addr    addr;
  int               ret      = 0;

  memset(astring, 0, sizeof(astring));
  if (getcwd(astring, sizeof(astring)) != NULL) {
    fprintf(stdout, "working in %s directory\n", astring);
  } else {
    perror("getcwd() error");
    exit(1);
  }

  memset(astring, 0, sizeof(astring));
  strcat(astring, g_openair_dir);
  strcat(astring, "/openair3/TEST/EPC_TEST/play_scenario.xsl");

  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue = 1;
  cur = xsltParseStylesheetFile((const xmlChar *)astring);
  if (NULL == cur) {
    AssertFatal (0, "Could not parse stylesheet file %s (check OPENAIR_DIR env variable)!\n", astring);
  } else {
    fprintf(stdout, "XSLT style sheet: %s\n", astring);
  }

  doc = xmlParseFile(test_scenario_filename);
  if (NULL == doc) {
    AssertFatal (0, "Could not parse scenario xml file %s!\n", test_scenario_filename);
  } else {
    fprintf(stdout, "Test scenario file: %s\n", test_scenario_filename);
  }

  for (i = 0; i < enb_properties.number; i++) {
    // eNB S1-C IPv4 address
    sprintf(astring, "enb_s1c%d", i);
    params[nb_params++] = strdup(astring);
    addr.s_addr = enb_properties.properties[i]->enb_ipv4_address_for_S1_MME;
    sprintf(astring, "\"%s\"", inet_ntoa(addr));
    params[nb_params++] = strdup(astring);

    // MME S1-C IPv4 address
    for (j = 0; j < enb_properties.properties[i]->nb_mme; j++) {
      sprintf(astring, "mme_s1c%d_%d", i, j);
      params[nb_params++] = strdup(astring);
      AssertFatal (enb_properties.properties[i]->mme_ip_address[j].ipv4_address,
          "Only support MME IPv4 address\n");
      sprintf(astring, "\"%s\"", enb_properties.properties[i]->mme_ip_address[j].ipv4_address);
      params[nb_params++] = strdup(astring);
    }
  }
  params[nb_params] = NULL;
  res = xsltApplyStylesheet(cur, doc, params);
  if (NULL != res) {
    sprintf(play_scenario_filename,"%s",test_scenario_filename);
    if (strip_extension(play_scenario_filename) > 0) {
      strcat(play_scenario_filename, ".tsml");
      play_scenario_file = fopen( play_scenario_filename, "w+");
      if (NULL != play_scenario_file) {
        xsltSaveResultToFile(play_scenario_file, res, cur);
        fclose(play_scenario_file);
        fprintf(stdout, "Wrote test scenario to %s\n", play_scenario_filename);
      } else {
        fprintf(stderr, "Error in fopen(%s)\n", play_scenario_filename);
        ret = -1;
      }
    } else {
      fprintf(stderr, "Error in strip_extension()\n");
      ret = -1;
    }
  } else {
    fprintf(stderr, "Error in xsltApplyStylesheet()\n");
    ret = -1;
  }
  xsltFreeStylesheet(cur);
  xmlFreeDoc(doc);
  xmlFreeDoc(res);

  xsltCleanupGlobals();
  xmlCleanupParser();
  return ret;
}

//------------------------------------------------------------------------------
static void usage (
    int argc,
    char *argv[])
//------------------------------------------------------------------------------
{
  fprintf (stdout, "Please report any bug to: %s\n",PACKAGE_BUGREPORT);
  fprintf (stdout, "Usage: %s [options]\n\n", argv[0]);
  fprintf (stdout, "\n");
  fprintf (stdout, "Client options:\n");
  fprintf (stdout, "\t-S | --server         <server network @>  File name (with no path) of a test scenario that has to be replayed (TODO in future?)\n");
  fprintf (stdout, "Server options:\n");
  fprintf (stdout, "\t-d | --test-dir       <dir>               Directory where a set of files related to a particular test are located\n");
  fprintf (stdout, "\t-c | --enb-conf-file  <file>              Provide an eNB config file, valid for the testbed\n");
  fprintf (stdout, "\t-s | --scenario       <file>              File name (with no path) of a test scenario that has to be replayed ()\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "Other options:\n");
  fprintf (stdout, "\t-h | --help                               Print this help and return\n");
  fprintf (stdout, "\t-v | --version                            Print informations about the version of this executable\n");
  fprintf (stdout, "\n");
}

//------------------------------------------------------------------------------
int
config_parse_opt_line (
  int argc,
  char *argv[],
  char **test_dir_name,
  char **scenario_file_name,
  char **enb_config_file_name)
//------------------------------------------------------------------------------
{
  int                           option;
  int                           rv                   = 0;
  const Enb_properties_array_t *enb_properties_p     = NULL;

  enum long_option_e {
    LONG_OPTION_START = 0x100, /* Start after regular single char options */
    LONG_OPTION_ENB_CONF_FILE,
    LONG_OPTION_SCENARIO_FILE,
    LONG_OPTION_TEST_DIR,
    LONG_OPTION_HELP,
    LONG_OPTION_VERSION
  };

  static struct option long_options[] = {
    {"enb-conf-file",  required_argument, 0, LONG_OPTION_ENB_CONF_FILE},
    {"scenario ",      required_argument, 0, LONG_OPTION_SCENARIO_FILE},
    {"test-dir",       required_argument, 0, LONG_OPTION_TEST_DIR},
    {"help",           no_argument,       0, LONG_OPTION_HELP},
    {"version",        no_argument,       0, LONG_OPTION_VERSION},
     {NULL, 0, NULL, 0}
  };

  /*
   * Parsing command line
   */
  while ((option = getopt_long (argc, argv, "vhc:s:d:", long_options, NULL)) != -1) {
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
          *test_dir_name = strdup(optarg);
          if (is_file_exists(*test_dir_name, "test dirname") != GS_IS_DIR) {
            fprintf(stderr, "Please provide a valid test dirname, %s is not a valid directory name\n", *test_dir_name);
            exit(1);
          }
          printf("Test dir name is %s\n", *test_dir_name);
        }
        break;

      case LONG_OPTION_VERSION:
      case 'v':
        printf("Version %s\n", PACKAGE_VERSION);
        exit (0);
        break;

      case LONG_OPTION_HELP:
      case 'h':
      default:
        usage (argc, argv);
        exit (0);
    }
  }
  if (NULL == *test_dir_name) {
    fprintf(stderr, "Please provide a valid test dirname\n");
    exit(1);
  }
  if (chdir(*test_dir_name) != 0) {
    fprintf(stderr, "Error: chdir %s returned %s\n", *test_dir_name, strerror(errno));
    exit(1);
  }
  if (rv & PLAY_SCENARIO) {
    if (NULL == *enb_config_file_name) {
      fprintf(stderr, "Error: please provide the original eNB config file name that should be in %s\n", *test_dir_name);
    }
    if (is_file_exists(*enb_config_file_name, "eNB config file") != GS_IS_FILE) {
      fprintf(stderr, "Error: original eNB config file name %s is not found in dir %s\n", *enb_config_file_name, *test_dir_name);
    }
    enb_properties_p = enb_config_init(*enb_config_file_name);

    if (NULL == *scenario_file_name) {
      fprintf(stderr, "Error: please provide the scenario file name that should be in %s\n", *test_dir_name);
    }
    if (is_file_exists(*scenario_file_name, "Scenario file") != GS_IS_FILE) {
      fprintf(stderr, "Error: Scenario file name %s is not found in dir %s\n", *scenario_file_name, *test_dir_name);
    }
  }
  return rv;
}

//------------------------------------------------------------------------------
int main( int argc, char **argv )
//------------------------------------------------------------------------------
{
  int              actions              = 0;
  char            *test_dir_name        = NULL;
  char            *scenario_file_name   = NULL;
  char            *enb_config_file_name = NULL;
  char             play_scenario_filename[NAME_MAX];
  int              ret                  = 0;
  test_scenario_t *scenario             = NULL;

  memset(play_scenario_filename, 0, sizeof(play_scenario_filename));
  g_openair_dir = getenv("OPENAIR_DIR");
  if (NULL == g_openair_dir) {
    fprintf(stderr, "Error: Could not get OPENAIR_DIR environment variable\n");
    exit(1);
  }

  actions = config_parse_opt_line (argc, argv, &test_dir_name, &scenario_file_name, &enb_config_file_name); //Command-line options
  if  (actions & PLAY_SCENARIO) {
    if (generate_xml_scenario(test_dir_name, scenario_file_name,enb_config_file_name, play_scenario_filename) == 0) {
      if (NULL != (scenario = generate_scenario(play_scenario_filename))) {
        ret = play_scenario(scenario);
      } else {
        fprintf(stderr, "Error: Could not generate scenario from tsml file\n");
        ret = -1;
      }
    } else {
      fprintf(stderr, "Error: Could not generate tsml scenario from xml file\n");
      ret = -1;
    }
    free_pointer(test_dir_name);
    free_pointer(scenario_file_name);
    free_pointer(enb_config_file_name);
  }

  return ret;
}
