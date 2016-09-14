#ifndef __PHY_INTERFACE_VARS_H__
#define __PHY_INTERFACE_VARS_H__

//#include "SIMULATION/PHY_EMULATION/spec_defs.h"
#include "defs.h"

#ifdef PHY_EMUL
#include "SIMULATION/PHY_EMULATION/DEVICE_DRIVER/defs.h"
#include "SIMULATION/simulation_defs.h"
#endif


unsigned int mac_debug;

//MAC_xface *mac_xface;

//MACPHY_PARAMS MACPHY_params;

unsigned int mac_registered;


#endif

#ifndef USER_MODE
EXPORT_SYMBOL(mac_xface);
#endif //PHY_EMUL

