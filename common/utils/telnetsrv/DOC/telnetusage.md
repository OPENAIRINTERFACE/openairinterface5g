# starting the softmodem with the telnet server
By default the embedded telnet server, which is implemented in a shared library, is not built. It can be built after compiling the softmodem executable using the `build_oai` script:

```bash
 cd \<oai repository\>/openairinterface5g
 source oaienv
 cd cmake_targets
 ./build_oai  --build-lib telnetsrv
```

This will create the `libtelnetsrv.so` and `libtelnetsrv_<app>` file in the `cmake_targets/ran_build/build` subdirectory of the oai repository. `<app>` can be "enb", "gnb", "4GUE", "5GUE", or "ci", each library containing functions specific to a given executable.

When starting the softmodem, you must specify the **_\-\-telnetsrv_** option to load and start the telnet server. The telnet server is loaded via the [oai shared library loader](loader).

# using the Command Line Interface
By default the telnet server listen on all the ip addresses configured on the system and on port 9090.  This behavior can be changed using the `listenaddr` and `listenport` parameters.
The telnet server includes a basic help, listing available commands and some commands also provide a specific detailed help sub-command.
Below are  examples of telnet sessions:

*  [getting help](telnethelp.md)
*  [using the history](telnethist.md)
*  [using the get and set commands](telnetgetset.md)
*  [using the loop command](telnetloop.md)
*  [loader command](telnetloader.md)
*  [log command](telnetlog.md)
*  [measur command](telnetmeasur.md)

# telnet server parameters
The telnet server is using the [oai configuration module](Config/Rtusage). Telnet parameters must be specified in the `telnetsrv` section. Some parameters can be modified via the telnet telnet server command, as specified in the last column of the following table.

| name | type | default | description | dynamic |
|:---:|:---:|:---:|:----|:----:|
| `listenaddr` | `ipV4 address, ascii format` | "0.0.0.0" | local address the server is listening on| N |
| `listenport` | `integer` | 9090 | port number the server is listening on | N |
| `listenstdin` | `integer` (bool) | 0 | enable input from stdin via additional thread | N |
| `policy` | `integer` | 0 | scheduling priority for telnet (0-99) | N |
| `loopcount` | `integer` | 10 | number of iterations for the loop command  | Y |
| `loopdelay` | `integer` | 5000 | delay (in ms) between 2 loop command iterations  | Y |
| `histfile` | `string` | "oaitelnet.history" | file used for command history persistency | Y |
| `histsize` | `integer` | 50 | maximum number of commands saved in the history | Y |
| `logfile`  | `string` | `oaisoftmodem.log` | output file to which to redirect
| `phypbsize`  | `integer` | 65000 | string buffer size to dump phy stats | Y |
| `staticmod`  | `string` | (empty) | additional internally defined telnet modules (`--telnetsrv.staticmod X`, comma-separated) to load on startup. The modules should define a function `add_X_cmds()` in which the module can register telnet commands | N |
| `shrmod`  | `string` | (empty) | additional shared object files `telnetsrv_X.so` (`--telnetsrv.shrmod X`, comma-separated) to load on startup. The shared object(s) should define a function `add_X_cmds()` in which the shared object can register telnet commands | N |

[oai telnet server home](telnetsrv.md)
