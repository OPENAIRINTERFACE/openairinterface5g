#ifndef _CONFIG_USERAPI_H_
#define _CONFIG_USERAPI_H_

typedef int paramdef_t;

#ifdef CMDLINE_TTRACEPARAMS_DESC
#undef CMDLINE_TTRACEPARAMS_DESC
#endif
#define CMDLINE_TTRACEPARAMS_DESC {}

#define config_get(dummy, ...) (void)(dummy)
#define config_process_cmdline(...) /**/

#endif /* _CONFIG_USERAPI_H_ */
