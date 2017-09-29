#ifndef LOAD_SHLIB_H
#define LOAD_SHLIB_H


typedef int(*initfunc_t)(void);
#ifdef LOAD_MODULE_SHLIB_MAIN
#else
extern int load_module_shlib(char *modname);
#endif

#endif
