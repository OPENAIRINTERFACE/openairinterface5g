### Initializing and configuring the logging facility

#### logging facility APIs
```C
int  logInit (void);
```
Allocate the internal data used by the logging utility, set the configuration, using the [configuration  module](../../../config/DOC/config.md)

```C
void logClean (void)
```
Reset console attributes (color settings) and possibly close opened log files. Logging facility is still usable after this call, the internal data are not freed.

```C
int  set_log(int component, int level)
void set_glog(int level)
```
Set a component or  all components logiing level. Messages which level is lower than this level are not printed to the console.

```C
void set_glog_onlinelog(int enable)
```
Enable or disable all logging messages. Even error level messages are discarded, which is not advised. This can be usefull to temporarily workaround high rate of logging messages.

```C
void set_component_filelog(int comp)
void close_component_filelog(int comp)
```

Redirect or reset to stdout the output stream used by the logging facility. When the output stream is redirected to a file, it is created under /tmp with a hard-coded filename including the componemt name.

```C
SET_LOG_DEBUG(flag)
CLEAR_LOG_DEBUG(flag)
SET_LOG_DUMP(flag)
CLEAR_LOG_DUMP(flag)
```
 These macros are used to set or clear the corresponding bit flag, trigerring the activation or un-activation of conditional code or memory dumps generation.

Example of using the logging utility APIs can be found, for initialization and cleanup,  in [lte-softmodem.c](../../../../targets/RT/USER/lte-softmodem.c) and in the [telnet server log command implementation](../../telnetsrv/telnetsrv_proccmd.c) for a complete access to the logging facility features.

#### components and debug flags definitions

Adding a new component is just adding an item in the `comp_name_t` enum defined in [log.h](../log.h) . You must also declare it in the T Tracer facility [message fefinitions](../../T/T_messages.txt).
To add a flag than can then be used for adding conditional code or memory dumps you have to add the flag definition in the `LOG_MASKMAP_INIT` macro, in [log.h](../log.h).

[logging facility developer main page](devusage.md)
[logging facility  main page](log.md)
[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
