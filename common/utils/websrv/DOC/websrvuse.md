[[_TOC_]]

# Building the webserver


back-end (the http server) and front-end (the html and javascript code for the browsers) are both built when the build of the `websrv` optional library is requested.

You can include the web server interface components in your build using the `build_oai` script with the `--build-lib "telnetsrv websrv"` option.
The related cmake cache entry  in `ran_build/build/CMakeCache.txt` for building the web server
is `ENABLE_WEBSRV:BOOL=ON` 

When cmake configuration has been completed, with websrv enabled and all dependencies met, font-end and back-end 
can be built separately, using respectively `make websrvfront` or `make websrv` from the build repository (replace make by ninja if you
build using ninja). 

## Additional dependencies for the backend

To build the webserver frontend, you need additionally

1. the [ulfius library](https://github.com/babelouest/ulfius)
2. the [jansson library](https://github.com/akheron/jansson-debian)

These libraries are not handled by `build_oai -I`

### Ubuntu

```bash
$ sudo apt-get update
$ sudo apt-get install -y libulfius-dev libjansson-dev npm curl wget
```

### Fedora(-based OS)

```bash
$ sudo dnf update -y
$ sudo dnf install -y jansson-devel npm curl wget
```

ulfius has to be installed as explained [here](https://github.com/babelouest/ulfius/blob/master/INSTALL.md#pre-compiled-packages).

## Additional dependencies for the frontend

Currently the web server frontend can run with nodejs 18, you can check the
version (if any) installed on your system entering the `node -v` command.

To prevent difficult situations with nodejs or npm versions it is better to
specifically install the required nodejs version (after removing existing
versions of npm and nodejs), as explained
[here](https://www.stewright.me/2023/04/install-nodejs-18-on-ubuntu-22-04/) for
ubuntu. Similar instructions can be found for other distributions.  It is also
possible to make several nodejs versions co-habiting on your system, this is
beyond this doc.


For example, to install a specific nodeJS version, you can run the below
command, with the typical output printed below for your convenience:
```bash
$ curl -s https://deb.nodesource.com/setup_18.x | sudo bash

## Populating apt-get cache...

apt-get update

## Confirming "jammy" is supported...

+ curl -sLf -o /dev/null 'https://deb.nodesource.com/node_18.x/dists/jammy/Release'

## Adding the NodeSource signing key to your keyring...

+ curl -s https://deb.nodesource.com/gpgkey/nodesource.gpg.key | gpg --dearmor | tee /usr/share/keyrings/nodesource.gpg >/dev/null

## Creating apt sources list file for the NodeSource Node.js 18.x repo...

+ echo 'deb [signed-by=/usr/share/keyrings/nodesource.gpg] https://deb.nodesource.com/node_18.x jammy main' > /etc/apt/sources.list.d/nodesource.list
+ echo 'deb-src [signed-by=/usr/share/keyrings/nodesource.gpg] https://deb.nodesource.com/node_18.x jammy main' >> /etc/apt/sources.list.d/nodesource.list

## Running `apt-get update` for you...

+ apt-get update
## Run `sudo apt-get install -y nodejs` to install Node.js 18.x and npm
## You may also need development tools to build native addons:
     sudo apt-get install gcc g++ make
## To install the Yarn package manager, run:
     curl -sL https://dl.yarnpkg.com/debian/pubkey.gpg | gpg --dearmor | sudo tee /usr/share/keyrings/yarnkey.gpg >/dev/null
     echo "deb [signed-by=/usr/share/keyrings/yarnkey.gpg] https://dl.yarnpkg.com/debian stable main" | sudo tee /etc/apt/sources.list.d/yarn.list
     sudo apt-get update && sudo apt-get install yarn


sudo apt install nodejs -y
Reading package lists... Done
Building dependency tree... Done
Reading state information... Done
The following packages were automatically installed and are no longer required:
  diodon libdiodon0 libxdo3 xdotool
Use 'sudo apt autoremove' to remove them.
The following NEW packages will be installed:
  nodejs
0 upgraded, 1 newly installed, 0 to remove and 8 not upgraded.
Need to get 28.9 MB of archives.
After this operation, 188 MB of additional disk space will be used.
Get:1 https://deb.nodesource.com/node_18.x jammy/main amd64 nodejs amd64 18.17.1-deb-1nodesource1 [28.9 MB]
Fetched 28.9 MB in 3s (11.3 MB/s)  
Selecting previously unselected package nodejs.
(Reading database ... 399541 files and directories currently installed.)
Preparing to unpack .../nodejs_18.17.1-deb-1nodesource1_amd64.deb ...
Unpacking nodejs (18.17.1-deb-1nodesource1) ...
Setting up nodejs (18.17.1-deb-1nodesource1) ...
Processing triggers for man-db (2.10.2-1) ...
```

If you want to put a more recent version, refer to the steps below, which is
common for any OS:
```bash
$ sudo npm -g install @angular/cli@latest npm@latest
$ sudo npm install n -g
# Install the LTS version
$ n lts
# Or install the latest version
$ n latest
$ node --version
```

## Building the webserver

### Backend

The websrv targets won't be available till cmake has been successfully configured with the websrv option enabled

```bash
$ cd <oai repository>/openairinterface5g/cmake_targets
$ ./build_oai  --build-lib websrv
```
or, without the help of the  `build_oai` script:

```bash
$ cd \<oai repository\>/openairinterface5g/cmake_targets/ran_build/build
$ make websrv
```

This will create the `libwebsrv.so`  file in the `cmake_targets/ran_build/build` directory.

### Frontend

The frontend targets needs to be triggered explicitly:

```bash
$ cd \<oai repository\>/openairinterface5g/cmake_targets/ran_build/build
$ make websrvfront
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


## Examples

**build example when back-end dependency is not installed**
```bash
$ ./build_oai --gNB --nrUE -w USRP --build-lib "telnetsrv websrv nrscope"
[...]
CMake Error at common/utils/websrv/CMakeLists.txt:31 (find_library):
  Could not find ULFIUS using the following names: libulfius.so
-- Configuring incomplete, errors occurred!
See also "/usr/local/oai/develop_unmodified/openairinterface5g/cmake_targets/ran_build/build/CMakeFiles/CMakeOutput.log".
build have failed
```

**build example when front-end dependency is not installed**
```bash
$ ./build_oai --gNB --nrUE -w USRP --build-lib "telnetsrv websrv nrscope"
[...]
-- found libulfius for websrv
-- found libjansson for websrv
CMake Error at common/utils/websrv/CMakeLists.txt:45 (find_program):
  Could not find NPM using the following names: npm


-- Configuring incomplete, errors occurred!
See also "/usr/local/oai/develop_unmodified/openairinterface5g/cmake_targets/ran_build/build/CMakeFiles/CMakeOutput.log".
build have failed
```

**build example when dependencies are met**

```bash
$ ./build_oai --ninja -c -C --gNB --nrUE -w USRP --build-lib "telnetsrv websrv nrscope"
[...]
-- found libulfius for websrv
-- found libjansson for websrv
-- found npm for websrv
-- Configuring webserver backend
-- Configuring webserver frontend
[...]
-- Configuring done
-- Generating done
-- Build files have been written to: /usr/local/oai/develop_unmodified/openairinterface5g/cmake_targets/ran_build/build
cd /usr/local/oai/develop_unmodified/openairinterface5g/cmake_targets/ran_build/build
Running "cmake --build .  --target nr-softmodem nr-cuup nr-uesoftmodem oai_usrpdevif telnetsrv websrv nrscope params_libconfig coding rfsimulator dfts -- -j8" 
Log file for compilation is being written to: /usr/local/oai/develop_unmodified/openairinterface5g/cmake_targets/log/all.txt
nr-softmodem nr-cuup nr-uesoftmodem oai_usrpdevif telnetsrv websrv nrscope params_libconfig coding rfsimulator dfts compiled
BUILD SHOULD BE SUCCESSFUL
```


# Running the web server interface

When starting the softmodem, you must specify the `--websrv` option to load and start the web server. The web server is loaded via the [oai shared library loader](loader).

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
To trigger the back-end use the `--websrv` option, possibly modifying the parameters as explained in the [previous section](./websrvuse.md#web-server-parameters).  The two following commands allow starting the oai gNB and the oai 5G UE on the same computer, starting the telnet server and the web interface on both executables.

`./nr-softmodem -O  /usr/local/oai/conf/gnb.band78.sa.fr1.106PRB.usrpb210.conf --rfsim --rfsimulator.serveraddr server --telnetsrv  --telnetsrv.listenstdin --websrv   --rfsimulator.options chanmod`

.`/nr-uesoftmodem -O /usr/local/oai/conf/nrue_sim.conf --sa --numerology 1 -r 106 -C 3649440000 --rfsim --rfsimulator.serveraddr 127.0.0.1 --websrv --telnetsrv --websrv.listenport 8092 --telnetsrv.listenport 8091`



## Connecting from a browser to the oai softmodem's

Assuming that the previous commands run successfully and that you also run your browser on the same host, you should be able to connect to the gNB and UE web interface using respectively the following url's:

http://127.0.0.1:8090/websrv/index.html

http://127.0.0.1:8092/websrv/index.html

The interface should be intuitive enough, keeping in mind the following restrictions:

- The command tab is not available if the telnet server is not enabled
- The softscope tab is not available if the xforms scope is started (`-d` option)
- Only one connection is supported to a back-end, especially for the scope interface

Some front-end  objects, which usage are less intuitive  provide a tooltip to help interface usage.

## some webserver screenshots

- [main page](main.png)
- [Configuring logs](logscfg.png)
- [scope interface](scope.png)

[oai web serverinterface  home](websrv.md)
