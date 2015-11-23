#include <string.h>
#include <math.h>
#include <unistd.h>
#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "MAC_INTERFACE/vars.h"

#include "rlc_am.h"
#include "rlc_am_test.h"
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/vars.h"
#include "UTIL/LOG/log_if.h"
#include "UTIL/LOG/log_extern.h"
#include "RRC/LITE/vars.h"
#include "PHY_INTERFACE/vars.h"
#include "UTIL/OCG/OCG.h"



int main(int argc, char **argv)
{

  //initialize the log generator
  logInit();
  mac_xface = malloc(sizeof(MAC_xface));
  rlc_am_v9_3_0_test();
  return(0);
}


