# code example of adding a command to the telnet server

The following example is extracted from [the oai `openair1/PHY/CODING/coding_load.c` file](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/openair1/PHY/CODING/coding_load.c).

```c
/*
include the telnet server data structures and API definitions
*/
#include "common/utils/telnetsrv/telnetsrv.h"


/*
define the null terminated array of telnetshell_cmddef_t structures
which map each sub-command string to a function implementing it.
you may also provide a help string which will be printed when
the global help command is used. The prototype for the function
implementing sub commands must match the `cmdfunc_t` type defined
in `telnetsrv.h`
*/
static int coding_setmod_cmd(char *buff, int debug, telnet_printfunc_t prnt);
static telnetshell_cmddef_t coding_cmdarray[] = {
    {"mode", "[sse,avx2,stdc,none]", coding_setmod_cmd, {NULL}, 0, NULL},
    {"", "", NULL, {NULL}, 0, NULL},
};

/*
define the null terminated list of telnetshell_vardef_t structures defining the
variables that can be set and get using the pre-defined get and set command
of the telnet server
*/
telnetshell_vardef_t coding_vardef[] = {
{"maxiter",TELNET_VARTYPE_INT32,&max_turbo_iterations},
{"",0,NULL}
};
.................
/*
 look for telnet server, if it is loaded, add the coding commands to it
 we use the shared library loader API to check the telnet server availibility
The telnet server TELNET_ADDCMD_FNAME function takes three arguments:
1.  The name of the telnet command to be added, here "coding"
1.  The `coding_cmdarray` list of "coding" sub-commands we defined earlier
1.  The `coding_varde f`list of variables we defined earlier
*/
     add_telnetcmd_func_t addcmd = (add_telnetcmd_func_t)get_shlibmodule_fptr("telnetsrv", TELNET_ADDCMD_FNAME);
     if (addcmd != NULL) {
         addcmd("coding",coding_vardef,coding_cmdarray);
.......
/*
  functions implementing the "coding mode" sub command, as defined in
  the `coding_cmdarray` passed earlier to the TELNET_ADDCMD_FNAME function.
  This function will be called by the telnet server, when the `coding_cmdarray`
  command is received from the telnet client
*/
int coding_setmod_cmd(char *buff, int debug, telnet_printfunc_t prnt)
{
  /*
  1. buff argument is an input argument, pointer to the string received
  from the telnet client, the command and sub-command parts are removed
  In this case it points after "coding setmod" and is of no use as
  we don't have second level sub-commands.
  1. debug argument is an input argument set by the telnet server
  1. prnt arguments is also an input argument, a function pointer, to be used
  in place of printf to print messages on the telnet client interface. As this function
  is called by the telnet server stdout points to the main executable console,
  */
   if (debug > 0)
       prnt( "coding_setmod_cmd received %s\n",buff);

      if (strcasestr(buff,"sse") != NULL) {
         decoding_setmode(MODE_DECODE_SSE);
      } else if (strcasestr(buff,"avx2") != NULL) {
         decoding_setmode(MODE_DECODE_AVX2);
      } else if (strcasestr(buff,"stdc") != NULL) {
         decoding_setmode(MODE_DECODE_C);
      } else if (strcasestr(buff,"none") != NULL) {
         decoding_setmode(MODE_DECODE_NONE);
      } else {
          prnt("%s: wrong setmod parameter...\n",buff);
      }
   prnt("Coding and decoding current mode: %s\n",modedesc[curmode]);
   return 0;
}

..............

```
# telnet server API

```c
int add_telnetcmd(char *modulename, telnetshell_vardef_t *var, telnetshell_cmddef_t *cmd)
```
Add a command and the `cmd` list of sub-commands to the telnet server. After a successful call to `add_telnetcmd` function, the telnet server calls the function defined for each sub-commands in the null terminated `cmd` array, when the character string received from the telnet client matches the command and sub-command strings.
Also adds the list of variables described in the `var` array to the list of variable which can be set and read.
The function returns -1 if one argument is NULL.
The telnet server is dynamically loaded, to use  the `add_telnetcmd` function, the shared library loader API should be used to check the availability of the telnet server and retrieve it's address, as shown in [the example at the top of this page](telnetaddcmd.md#code-example-of-adding-a-command-to-the-telnet-server).

# telnet server public data types
## `telnetshell_vardef_t`structure
 This structure is used by developers to describe the variables that can be set or read using the get,set and getall sub-commands.

| Fields     | type |Description                                                       |
|:-----------|:------:|:-----------------------|
| `varname`    | `char[TELNET_CMD_MAXSIZE]`  | variable name, as specified when using the get and set commands. |
| `vartype`     | `char` |  Defines the type of the variable pointed by the `varvalptr`field. Supported values: TELNET_VARTYPE_INT32  TELNET_VARTYPE_INT16 TELNET_VARTYPE_INT64  TELNET_VARTYPE_STRING   TELNET_VARTYPE_DOUBLE |
| `varvalptr`     | `void*` |  Defines the type of the variable pointed by the `varvalptr`field |

## `telnetshell_cmddef_t`structure
 This structure is used by developers to describe the first level sub-commands to be added to the telnet server.

| Fields     | type |Description                                                       |
|:-----------|:------:|:-----------------------|
| `cmdname`    | `char[TELNET_CMD_MAXSIZE]`  | command name, as tested by the telnet server or web server to check it should call the `cmdfunc` or `webfunc` function  |
| `helpstr`     | `char[TELNET_HELPSTR_SIZE]` |  character string to print when the help`command is received from the telnet client |
| `cmdfunc`     | `cmdfunc_t` |  pointer to the function implementing the `cmdname` telnet or web server sub command. |
| `webfunc`     | `webfunc_t` or `webfunc_getdata_t` |  pointer to the function implementing the `cmdname` web server sub command. |
| `cmdflags`     | unsigned int bit mask | possible bits are defined in the next table |
| `qptr`     | pointer to a thread pool queue |  when `TELNETSRV_CMDFLAG_PUSHINTPOOLQ` bit is set in `cmdflags` , contains the queue where the command is pushed|

### `cmdflags ` bit definitions
 These flags possibly modify the way the  `telnetshell_cmddef_t` structure is used by the telnet server and/or  the web server

| Fields     | Description                                                       |
|:------------------------------------:|:-----------------------|
| TELNETSRV_CMDFLAG_PUSHINTPOOLQ | function defined in `cmdfunc` field is pushed in a thread pool queue instead of beeing executed synchronously. The queue is created by the telnet server code in the  `add_telnetcmd` function|
| TELNETSRV_CMDFLAG_GETWEBTBLDATA | web server will use the `webfunc_getdata_t` version of the `webfunc` union. command results are written in a `webdatadef_t` structure instead of being printed |
| TELNETSRV_CMDFLAG_TELNETONLY | this command definition is only for the telnet server, web server is ignoring it |
| TELNETSRV_CMDFLAG_WEBSRVONLY | this command definition is only for the web server, telnet server is ignoring it |
| TELNETSRV_CMDFLAG_CONFEXEC  | Currently only used by the web server, command execution has to be confirmed by user |
| TELNETSRV_CMDFLAG_NEEDPARAM  | Currently only used by the web server, user must provide a parameter before command execution |
| TELNETSRV_CMDFLAG_WEBSRV_SETRETURNTBL  | Currently only used by the web server, when a parameter modification requires a new page |
| TELNETSRV_CMDFLAG_AUTOUPDATE |  Currently only used by the web server, the command can be rescheduled for result update |
[oai telnet server home](telnetsrv.md)
