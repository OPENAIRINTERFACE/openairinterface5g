# starting the softmodem with the telnet server
By default the embedded telnet server, which is implemented in a shared library, is not built. It can be built after compiling the softmodem executable using the `build_oai` script:

```bash
 cd \<oai repository\>/openairinterface5g
 source oaienv
 cd cmake_targets
 ./build_oai  --build-telnetsrv
```

This will create the `libtelnetsrv.so` file in the `targets/bin` and `cmake_targets/lte_build_oai/build` sub directories of the oai repository.

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

# telnet server parameters
The telnet server is using the [oai configuration module](Config/Rtusage). Telnet parameters must be specified in the `telnetsrv` section. Some parameters can be modified via the telnet telnet server command, as specified in the last column of the following table.

| name | type | default | description | dynamic |
|:---:|:---:|:---:|:----|:----:|
| `listenaddr` | `ipV4 address, ascii format` | "0.0.0.0" | local address the server is listening on| N |
| `listenport` | `integer` | 9090 | port number the server is listening on | N |
| `loopcount` | `integer` | 10 | number of iterations for the loop command  | Y |
| `loopdelay` | `integer` | 5000 | delay (in ms) between 2 loop command iterations  | Y |
| `histfile` | `character string` | "oaitelnet.history" | file used for command history persistency | Y |
| `histfsize` | `integer` | 50 | maximum number of commands saved in the history | Y |

[oai telnet server home](telnetsrv.md)
