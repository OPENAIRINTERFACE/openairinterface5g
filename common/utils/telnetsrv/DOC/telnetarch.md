# telnet server principles

The oai telnet server is implemented in shared libraries to be loaded by the [oai shared library loader](loader). `libtelnetsrv.so ` contains the code common to all oai softmodem executables, where `libtelnetsrv_<app>.so` contains the code specific to the executable identified by `app`. 

| `<app>` |    executable     |
| :-----: | :---------------: |
|   enb   |  `lte-softmodem`  |
|   gnb   |  `nr-softmodem`   |
|  4Gue   | `lte-uesoftmodem` |
|  5Gue   | `nr-uesoftmodem`  |

The implementation includes a `telnetsrv_autoinit` function which is automatically called at load time, starts the telnet server and registers a first set of commands, which are delivered with the server (telnet, softmodem, loader). it also uses the `get_softmodem_function` call to retrieve the running executable id and possibly loads the corresponding `libtelnetsrv_<app>.so`library.

Currently the telnet server only supports one user connection. The same dedicated thread is used to wait for a user connection and process the input received from this connection.

The telnet server provides an API which can be used by any oai component to add new CLI commands to the server. A pre-defined  command can be used to get or set a list of variables.



# telnet server source files

telnet server source files are located in [common/utils/telnetsrv](https://gitlab.eurecom.fr/oai/openairinterface5g/tree/develop/common/utils/telnetsrv)

1. [telnetsrv.c](../telnetsrv.c) contains the telnet server implementation, including the implementation of the `telnet` CLI command. This implementation is compatible with all softmodem executables and is in charge of loading any additional `libtelnetsrv_<app> .so` containing code specific to the running executables.
1.  [telnetsrv.h](../telnetsrv.h) is the telnet server include file containing both private and public data type definitions. It also contains API prototypes for functions that are used to register a new command in the server.
1.  `telnetsrv_<XXX\>.c`: implementation of \<XXX\> CLI command which are delivered with the telnet server and are common to all softmodem executables. 
1. `telnetsrv_<XXX\>.h`: include file for the implementation of XXX CLI command. Usually included only in the corresponding `.c`file
1.  `telnetsrv_<app>_<XXX>.c`: implementation of \<XXX\> CLI command specific to the executable identified by \<app\>.These sources are used to create `libtelnetsrv_<app>.so` at build time.
1.  [telnetsrv_CMakeLists.txt](../CMakeLists.txt): CMakelists file containing the cmake instructions to build the telnet server. this file is included in the [global oai CMakelists](../../../../cmake_targets/CMakeLists.txt).

[oai telnet server home](telnetsrv.md)
