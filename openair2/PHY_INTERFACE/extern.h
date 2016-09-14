#ifndef __PHY_INTERFACE_EXTERN_H__
#define __PHY_INTERFACE_EXTERN_H__

//#include "SIMULATION/PHY_EMULATION/spec_defs.h"
#ifdef PHY_EMUL
#include "SIMULATION/PHY_EMULATION/DEVICE_DRIVER/defs.h"
#include "SIMULATION/PHY_EMULATION/DEVICE_DRIVER/extern.h"
#endif //PHY_EMUL

#include "defs.h"


extern unsigned int mac_debug;
#ifdef PHYSIM
extern MAC_xface *mac_xface;
#endif
//extern MACPHY_PARAMS MACPHY_params;

extern unsigned int mac_registered;


#endif
