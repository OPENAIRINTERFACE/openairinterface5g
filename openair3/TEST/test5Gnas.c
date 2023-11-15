#include <common/config/config_userapi.h>
#include <openair3/NAS/COMMON/NR_NAS_defs.h>

volatile int oai_exit;
configmodule_interface_t *uniqCfg = NULL;

int main(int argc, char **argv) {
  if ((uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY)) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }

  logInit();
  char *resp;
  nr_user_nas_t UErrc= {0};
  NRUEcontext_t UEnas= {0};
  // Network generate identity request after a sucessfull radio attach
  int size=identityRequest((void **)&resp, &UEnas);
  log_dump(NAS, resp, size, LOG_DUMP_CHAR,"   identity Request:\n" );
  // UE process the message that it has received from phy layer in a "DL transfer" message
  UEprocessNAS(resp, &UErrc);
  // UE Scheduler should later call the response
  size=identityResponse((void **)&resp, &UErrc);
  log_dump(NAS, resp, size, LOG_DUMP_CHAR,"   identity Response:\n" );
  // Now the gNB process the identity response
  processNAS(resp, &UEnas);
  // gNB scheduler should call the next query
  size=authenticationRequest((void **)&resp, &UEnas);
  log_dump(NAS, resp, size, LOG_DUMP_CHAR,"   authentication request:\n" );
  // as above
  UEprocessNAS(resp, &UErrc);
  size=authenticationResponse((void **)&resp, &UErrc);
  log_dump(NAS, resp, size, LOG_DUMP_CHAR,"   authentication response:\n" );
  processNAS(resp, &UEnas);
  return 0;
}
