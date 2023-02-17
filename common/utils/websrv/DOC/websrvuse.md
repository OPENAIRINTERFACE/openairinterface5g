

When all dependencies are met, you can build the all the web server interface components using the build_oai script with the `--build-lib all` option . As the web interface is an optional component, if it's dependencies are not found it won't stop the build. Web interface components (back-end or front-end) which cannot be built are just skipped.

###### build example when missing back-end dependencies 

```
./build_oai --build-lib all --nrUE
  websrv optional build not included in build-lib option  as dependencies requirements not met 
Enabling build of all optional shared libraries (telnetsrv enbscope uescope nrscope)
Will compile NR UE
RF HW set to None
2. Setting the OAI PATHS ...
OPENAIR_DIR    = /usr/local/oai/oai-develop/openairinterface5g
FreeDiameter prefix not found, install freeDiameter if EPC, HSS
3. building the compilation directives ...
running cmake ../../..
cuda include /usr/include
cuda library 
-- CMAKE_BUILD_TYPE is RelWithDebInfo
-- CPUARCH x86_64
-- AVX512 intrinsics are OFF
-- AVX2 intrinsics are ON
-- No T1 Offload support detected
calling protoc_call=/usr/local/oai/oai-develop/openairinterface5g/cmake_targets/tools/generate_protobuf FSPT_C_DIR=/usr/local/oai/oai-develop/openairinterface5g/cmake_targets/ran_build/build/CMakeFiles/FSPT_V2 FSPT_MSG_DIR=/usr/local/oai/oai-develop/openairinterface5g/targets/COMMON/MESSAGES/V2 FSPT_MSG_FILES=/usr/local/oai/oai-develop/openairinterface5g/targets/COMMON/MESSAGES/V2/flexsplit.proto
[libprotobuf WARNING google/protobuf/compiler/parser.cc:546] No syntax specified for the proto file: flexsplit.proto. Please use 'syntax = "proto2";' or 'syntax = "proto3";' to specify a syntax version. (Defaulted to proto2 syntax.)
fspt c dir is : /usr/local/oai/oai-develop/openairinterface5g/cmake_targets/ran_build/build/CMakeFiles/FSPT_V2
gcc -Wall -I. -I.. -I../itti -I../../../openair2/COMMON -Itracer -o _check_vcd check_vcd.c tracer/database.c tracer/utils.c -lm -pthread
./_check_vcd || (rm -f ./_check_vcd ./T_IDs.h ./T_messages.txt.h && false)
rm -f ./_check_vcd
Add enb specific telnet functions in libtelnetsrv_enb.so
No specific telnet functions for gnb
No specific telnet functions for 4Gue
Add 5Gue specific telnet functions in libtelnetsrv_5Gue.so
CMake Warning at common/utils/websrv/websrv_CMakeLists.txt:12 (message):
  ulfius library (https://github.com/babelouest/ulfius) not found, install
  libulfius-dev (ubuntu) if you need to build websrv back-end
Call Stack (most recent call first):
  CMakeLists.txt:3189 (include)


-- websrv backend build skipped, dependencies not found
..........
.............

```

######  build example when missing back-end dependencies 

```
  ./build_oai --build-lib all --nrUE
  Enabling build of all optional shared libraries (telnetsrv enbscope uescope nrscope websrv)
  Will compile NR UE
  RF HW set to None
  
  2. Setting the OAI PATHS ...
     OPENAIR_DIR    = /usr/local/oai/oai-develop/openairinterface5g
     FreeDiameter prefix not found, install freeDiameter if EPC, HSS
  3. building the compilation directives ...
     running cmake ../../..
     cuda include /usr/include
     cuda library 
     -- CMAKE_BUILD_TYPE is RelWithDebInfo
     -- CPUARCH x86_64
     -- AVX512 intrinsics are OFF
     -- AVX2 intrinsics are ON
     -- No T1 Offload support detected
     calling protoc_call=/usr/local/oai/oai-develop/openairinterface5g/cmake_targets/tools/generate_protobuf FSPT_C_DIR=/usr/local/oai/oai-develop/openairinterface5g/cmake_targets/ran_build/build/CMakeFiles/FSPT_V2 FSPT_MSG_DIR=/usr/local/oai/oai-develop/openairinterface5g/targets/COMMON/MESSAGES/V2 FSPT_MSG_FILES=/usr/local/oai/oai-develop/openairinterface5g/targets/COMMON/MESSAGES/V2/flexsplit.proto
     [libprotobuf WARNING google/protobuf/compiler/parser.cc:546] No syntax specified for the proto file: flexsplit.proto. Please use 'syntax = "proto2";' or 'syntax = "proto3";' to specify a syntax version. (Defaulted to proto2 syntax.)
     fspt c dir is : /usr/local/oai/oai-develop/openairinterface5g/cmake_targets/ran_build/build/CMakeFiles/FSPT_V2
     gcc -Wall -I. -I.. -I../itti -I../../../openair2/COMMON -Itracer -o _check_vcd check_vcd.c tracer/database.c tracer/utils.c -lm -pthread
     ./_check_vcd || (rm -f ./_check_vcd ./T_IDs.h ./T_messages.txt.h && false)
     rm -f ./_check_vcd
     Add enb specific telnet functions in libtelnetsrv_enb.so
     No specific telnet functions for gnb
     No specific telnet functions for 4Gue
     Add 5Gue specific telnet functions in libtelnetsrv_5Gue.so
     -- websrv backend can be built
     -- websrv frontend can be built
     -- Configuring done
     -- Generating done
     -- Build files have been written to: /usr/local/oai/oai-develop/openairinterface5g/cmake_targets/ran_build/build
............
.............
Log file for compilation is being written to: /usr/local/oai/oai-develop/openairinterface5g/cmake_targets/log/websrv.txt
websrv compiled
Build of websrv frontend enabled
Log file for compilation is being written to: /usr/local/oai/oai-develop/openairinterface5g/cmake_targets/log/websrvfront.txt
websrvfront compiled
Compiling rfsimulator
Log file for compilation is being written to: /usr/local/oai/oai-develop/openairinterface5g/cmake_targets/log/rfsimulator.txt  
.............
..............
```

  

# building and installing the front-end

Before building the front-end you need to install the npm node.js installer:

`apt-get install npm` for ubuntu or `dnf install npm`for fedora

then to build and install the frontend:

``` bash
cd \<oai repository\>/openairinterface5g/cmake_targets/ran_build/build
make websrvfront
up to date, audited 1099 packages in 3s

142 packages are looking for funding
  run `npm fund` for details

3 moderate severity vulnerabilities

Some issues need review, and may require choosing
a different dependency.

Run `npm audit` for details.
Built target websrvfront_installjsdep

> softmodemngx@2.0.0 build
> ng build

✔ Browser application bundle generation complete.
✔ Copying assets complete.
✔ Index html generation complete.

Initial Chunk Files           | Names         |  Raw Size | Estimated Transfer Size
main.1917944d43411293.js      | main          | 996.88 kB |               231.03 kB
styles.ca83009174c6585f.css   | styles        |  74.33 kB |                 7.70 kB
polyfills.36a0bc8cdfe2cb81.js | polyfills     |  45.10 kB |                13.82 kB
runtime.bf8117e4a18c7212.js   | runtime       |   1.06 kB |               606 bytes

                              | Initial Total |   1.09 MB |               253.14 kB

Build at: 2022-10-17T16:52:42.517Z - Hash: 3b6b647cfcb0dac9 - Time: 8651ms
Moving frontend files to:
    /usr/local/oai/oai-develop/openairinterface5g/cmake_targets/ran_build/build/websrv
    /usr/local/oai/oai-develop/openairinterface5g/targets/bin/websrv
Built target websrvfront
```



# Building and installing  the web server back-end
The back-end has two dependencies:

1. the [ulfius library](https://github.com/babelouest/ulfius) and the corresponding include files which are provided by the ubuntu libulfius-dev package: `sudo apt-get install -y libulfius-dev`
2. the [jansson](https://github.com/akheron/jansson-debian) library and the corresponding include files which are provided by the ubuntu libjansson-dev package: `sudo apt-get install -y libjansson-dev`

Dependencies can also be installed on fedora distribution, the jansson package is `jansson-devel`, ulfius has to be installed as explained [here](https://github.com/babelouest/ulfius/blob/master/INSTALL.md#pre-compiled-packages). 

By default the embedded web server back-end , which is implemented in a shared library, is not built. It can be built after compiling the softmodem executable using the `build_oai` script:

```bash
 cd \<oai repository\>/openairinterface5g
 source oaienv
 cd cmake_targets
 ./build_oai  --build-lib websrv
```

This will create the `libwebsrv.so`  file in the `targets/bin` and `cmake_targets/ran_build/build` sub directories of the oai repository.

When starting the softmodem, you must specify the **_\-\-websrv_** option to load and start the web server. The web server is loaded via the [oai shared library loader](loader).

# Testing the web server interface


## web server parameters
The web server back-end is using the [oai configuration module](Config/Rtusage). web server parameters must be specified in the websrv section. 

| name | type | default | description |
|:---:|:---:|:---:|:----|
| `listenaddr` | `ipV4 address, ascii format` | "0.0.0.0" | local address the back-end  is listening on |
| `listenport` | `integer` | 8090 | port number the server is listening on |
| debug | `integer` | 0 | When not 0, http  requests headers and json objects dump are added to back-end traces |
| fpath | character string | websrv | The path to on-disk http server resources . The default value matches the front-end installation when running  the softmodem from the executables repository. |
| cert, key, rootca | `character string` | null | certificates and key used to trigger https protocol (not tested) |
|                   |                              |                   |                                                              |

## running the back-end
To trigger the back-end use the `--websrv` option, possibly modifying the parameters as explained in the previous chapter.  The two following commands allow starting the oai gNB and the oai 5G UE on the same computer, starting the telnet server and the web interface on both executables.

`./nr-softmodem -O  /usr/local/oai/conf/gnb.band78.sa.fr1.106PRB.usrpb210.conf --rfsim --rfsimulator.serveraddr server --telnetsrv  --telnetsrv.listenstdin --websrv   --rfsimulator.options chanmod` 

.`/nr-uesoftmodem -O /usr/local/oai/conf/nrue_sim.conf --sa --numerology 1 -r 106 -C 3649440000 --rfsim --rfsimulator.serveraddr 127.0.0.1 --websrv --telnetsrv --websrv.listenport 8092 --nokrnmod --telnetsrv.listenport 8091`



## Connecting from a browser to the oai softmodem's

Assuming that the previous commands run successfully and that you also run your browser on the same host, you should be able to connect to the gNB and UE web interface using respectively the following url's:

http://127.0.0.1:8090/websrv/index.html
http://127.0.0.1:8092/websrv/index.html

The interface should be intuitive enough, keeping in mind the following restrictions:

- The command tab is not available if the telnet server is not enabled
- The softscope tab is not available if the xforms scope is started `(-d` option)
- Only one connection is supported to a back-end, especially for the scope interface

Some front-end  objects, which usage are less intuitive  provide a tooltip to help interface usage.

[oai web serverinterface  home](websrv.md)
