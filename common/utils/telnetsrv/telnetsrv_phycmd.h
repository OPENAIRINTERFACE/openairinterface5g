

#ifdef TELNETSRV_PHYCMD_MAIN

#include "UTIL/LOG/log.h"


#include "openair1/PHY/extern.h"


#define TELNETVAR_PHYCC0    0
#define TELNETVAR_PHYCC1    1

telnetshell_vardef_t phy_vardef[] = {
{"phycc1",TELNET_VARTYPE_PTR,NULL},
{"phycc2",TELNET_VARTYPE_PTR,NULL},
//{"iqmax",TELNET_VARTYPE_INT16,NULL},
//{"iqmin",TELNET_VARTYPE_INT16,NULL},
//{"loglvl",TELNET_VARTYPE_INT32,NULL},
//{"sndslp",TELNET_VARTYPE_INT32,NULL},
//{"rxrescale",TELNET_VARTYPE_INT32,NULL},
//{"txshift",TELNET_VARTYPE_INT32,NULL},
//{"rachemin",TELNET_VARTYPE_INT32,NULL},
//{"rachdmax",TELNET_VARTYPE_INT32,NULL},
{"",0,NULL}
};

#else

extern void add_phy_cmds();

#endif

/*-------------------------------------------------------------------------------------*/

