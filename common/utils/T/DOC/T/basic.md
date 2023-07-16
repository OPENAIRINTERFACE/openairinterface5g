# Basic usage of the T

## Compilation

### softmodem side

Simply call `build_oai` the usual way, for example `./build_oai --eNB -w USRP`.
The T tracer is compiled in by default.

### tracer side

Go to the directory `common/utils/T/tracer` and do `make`. This will locally
compile all tracer executables, and place them in `common/utils/T/tracer`. If
you wish to keep source and build separate, go to your existing build directory
(typically `cmake_targets/ran_build/build`), and do `make T_tools` (or `ninja
T_tools`). After that, the executables will be in
`<build-dir>/common/utils/T/tracer`.

In case of failure with one of the following errors:

```shell
/usr/bin/ld: cannot find -lXft
```
or
```shell
-- Checking for module 'xft'
--   No package 'xft' found
CMake Error at /usr/share/cmake-3.16/Modules/FindPkgConfig.cmake:463 (message):
  A required package was not found
```
Run:

```shell
sudo apt-get install libxft-dev
```

## Run the softmodem

Run the softmodem with the option `--T_stdout 2` and it will wait for a
tracer to connect to it before processing. (This option is confusing
and may change in the future.)

```shell
cd cmake_targets/ran_build/build
sudo ./lte-softmodem -O [configuration file] --T_stdout 2
```

Additional options can be passed to the softmodem.

The option `--T_nowait` lets the processing start immediately without
first waiting for a tracer.

The option `--T_port [port]` changes the default port used by the
softmodem to wait for a tracer. The default port is 2021.

The option `--T_dont_fork` allows one to use gdb to debug problems
with the softmodem. Note that you then may have some "zombie"
processes after crashes, in which case you can run
`sudo killall -9 lte-softmodem` to get rid of them (`lte-softmodem`
to be replaced by the program you trace, like `lte-softmodem-nos1`
or `oaisim`).

The option `--T_stdout` also accepts values 0 (to disable output
on the terminal and only use the T tracer) and 1 (to disable
the T tracer and only output on the terminal). The default is
1.

## Run a tracer

Go into the directory `common/utils/T/tracer` (or
`cmake_targets/ran_build/build/common/utils/T/tracer`) and run for example the
`enb` tracer:

```shell
./enb -d ../T_messages.txt
```

To trace a remote program, use the `-ip` option. For example,
if you want to trace a program running on `192.168.12.148` do:

```shell
./enb -d ../T_messages.txt -ip 192.168.12.148
```

A graphical user interface will appear with which you can interact to
control the monitoring. See [there](./enb.md) for more documentation
about the `enb` tracer.

It is possible to run several tracers at the same time. See
[here](./multi.md).
