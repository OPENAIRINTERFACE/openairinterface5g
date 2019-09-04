# telnet server principles

The oai telnet server is implemented in a shared library to be loaded by the [oai shared library loader](loader). The implementation includes a `telnetsrv_autoinit` function which is automatically called at load time, starts the telnet server and registers a first set of commands, which are delivered with the server (telnet, softmodem, loader).

Currently the telnet server only supports one user connection. The same dedicated thread is used to wait for a user connection and process the input received from this connection.

The telnet server provides an API which can be used by any oai component to add new CLI commands to the server. A pre-defined  command can be used to get or set a list of variables.



# telnet server source files

telnet server source files are located in [common/utils/telnetsrv](https://gitlab.eurecom.fr/oai/openairinterface5g/tree/develop/common/utils/telnetsrv)

1. [telnetsrv.c](https://gitlab.eurecom.fr/oai/openairinterface5g/tree/develop/common/utils/telnetsrv/telnetsrv.c) contains the telnet server implementation, including the implementation of the telnet CLI command.
1.  [telnetsrv.h](https://gitlab.eurecom.fr/oai/openairinterface5g/tree/develop/common/utils/telnetsrv/telnetsrv.h) is the telnet server include file containing both private and public data type definitions. It also contains API prototypes for functions that are used to register a new command in the server.
1.  `telnetsrv\_\<XXX\>.c`: implementation of \<XXX\> CLI command which are delivered with the telnet server.
1.  `telnetsrv\_\<XXX\>.h`: include file for the implementation of XXX CLI command. Usually included only in the corresponding `.c`file
1.  [telnetsrv_CMakeLists.txt](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/common/utils/telnetsrv/telnetsrv_CMakeLists.txt): CMakelists file containing the cmake instructions to build the telnet server. this file is included in the [global oai CMakelists](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/cmake_targets/CMakeLists.txt).

[oai telnet server home](telnetsrv.md)
