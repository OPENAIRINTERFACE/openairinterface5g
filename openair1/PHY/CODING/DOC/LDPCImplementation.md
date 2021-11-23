# LDPC coder/decoder implementation
The LDPC coder and decoder are implemented in a shared library, dynamically loaded at run-time using the [oai shared library loader](file://../../../../common/utils/DOC/loader.md). The code loading the LDPC library is in [nrLDPC_load.c](file://../nrLDPC_load.c), in function `load_nrLDPClib`, which must be called at init time.

## Selecting the LDPC library at run time

By default the function `int load_nrLDPClib(void)` looks for `libldpc.so`, this default behavior can be changed using the oai loader configuration options in the configuration file or from the command line as shown below:

#### Examples of ldpc shared lib selection when running nr softmodem's:

loading `libldpc_optim8seg.so` instead of `libldpc.so`:

```
./nr-softmodem -O libconfig:gnb.band78.tm1.106PRB.usrpx300.conf:dbgl5  --loader.ldpc.shlibversion _optim8seg
.......................
[CONFIG] loader.ldpc.shlibversion set to default value ""
[LIBCONFIG] loader.ldpc: 2/2 parameters successfully set, (1 to default value)
[CONFIG] shlibversion set to  _optim8seg from command line
[CONFIG] loader.ldpc 1 options set from command line
[LOADER] library libldpc_optim8seg.so successfully loaded
........................
```

loading `libldpc_cl.so` instead of `libldpc.so`:

`make ldpc_cl`

This command creates the `libldpc_cl.so` shared library. To perform this build successfully, only the OpenCL header `(/usr/include/CL/opencl.h)` and library `(/usr/lib/x86_64-linux-gnu/libOpenCL.so)`are required, they implement OpenCL API support which is not hardware dependent.

```
Scanning dependencies of target nrLDPC_decoder_kernels_CL
Built target nrLDPC_decoder_kernels_CL
Scanning dependencies of target ldpc_cl
Building C object CMakeFiles/ldpc_cl.dir/usr/local/oai/oai-develop/openairinterface5g/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder_CL.c.o
In file included from /usr/include/CL/cl.h:32,
                 from /usr/include/CL/opencl.h:38,
                 from /usr/local/oai/oai-develop/openairinterface5g/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder_CL.c:49:
/usr/include/CL/cl_version.h:34:9: note: #pragma message: cl_version.h: CL_TARGET_OPENCL_VERSION is not defined. Defaulting to 220 (OpenCL 2.2)
 #pragma message("cl_version.h: CL_TARGET_OPENCL_VERSION is not defined. Defaulting to 220 (OpenCL 2.2)")
         ^~~~~~~

Building C object CMakeFiles/ldpc_cl.dir/usr/local/oai/oai-develop/openairinterface5g/openair1/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim8segmulti.c.o
Linking C shared module libldpc_cl.so
Built target ldpc_cl

```

At runtime, to successfully use hardware acceleration via OpenCL, you need to install vendor specific packages which deliver the required drivers and tools to make use of their GPU (Nvidia, Intel...) , fpga (Xilinx, Intel) or CPU (Intel, AMD, ARM...) through OpenCL. 

`./nr-softmodem -O  libconfig:gnb.band78.sa.fr1.106PRB.usrpb210.conf:dbgl5 --rfsim --rfsimulator.serveraddr server  --sa --log_config.gtpu_log_level info  --loader.ldpc.shlibversion _cl`

``` 
------------------------------------------------
[LOADER] library libldpc_cl.so successfully loaded
[HW]   Platform 0, OpenCL profile FULL_PROFILE
[HW]   Platform 0, OpenCL version OpenCL 2.1 LINUX
[HW]   Device 0 is  available
[HW]   Device 0, type 2 = 0x00000002: cpu 
[HW]   Device 0, number of Compute Units: 8
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 8192
[HW]   Device 0, max Work Items size for dimension: 1 8192
[HW]   Device 0, max Work Items size for dimension: 2 8192
[New Thread 0x7fffcc258700 (LWP 3945123)]
[New Thread 0x7fffc3e57700 (LWP 3945124)]
[New Thread 0x7fffcbe57700 (LWP 3945125)]
[New Thread 0x7fffcba56700 (LWP 3945126)]
[New Thread 0x7fffcb254700 (LWP 3945128)]
[New Thread 0x7fffcb655700 (LWP 3945127)]
[New Thread 0x7fffcae53700 (LWP 3945129)]
[HW]   Platform 1, OpenCL profile FULL_PROFILE
[HW]   Platform 1, OpenCL version OpenCL 2.0 beignet 1.3
[New Thread 0x7fffc965a700 (LWP 3945130)]
[Thread 0x7fffc965a700 (LWP 3945130) exited]
[HW]   Device 0 is  available
[HW]   Device 0, type 4 = 0x00000004: gpu 
[HW]   Device 0, number of Compute Units: 20
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 512
[HW]   Device 0, max Work Items size for dimension: 1 512
[HW]   Device 0, max Work Items size for dimension: 2 512
-----------------------------------------------------------------
```

`./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --rfsim --sa -O libconfig:/usr/local/oai/conf/nrue_sim.conf:dbgl5  --nokrnmod --loader.ldpc.shlibversion _cl --log_config.hw_log_level info`

```
............................................................
[CONFIG] shlibversion set to  _cl from command line
[CONFIG] loader.ldpc 1 options set from command line
[LOADER] library libldpc_cl.so successfully loaded
[HW]   Platform 0, OpenCL profile FULL_PROFILE
[HW]   Platform 0, OpenCL version OpenCL 2.1 LINUX
[HW]   Device 0 is  available
[HW]   Device 0, type 2 = 0x00000002: cpu 
[HW]   Device 0, number of Compute Units: 8
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 8192
[HW]   Device 0, max Work Items size for dimension: 1 8192
[HW]   Device 0, max Work Items size for dimension: 2 8192
[New Thread 0x7fffecccc700 (LWP 3945413)]
[New Thread 0x7fffec8cb700 (LWP 3945415)]
[New Thread 0x7fffec4ca700 (LWP 3945414)]
[New Thread 0x7fffdf7fd700 (LWP 3945417)]
[New Thread 0x7fffdfbfe700 (LWP 3945418)]
[New Thread 0x7fffdffff700 (LWP 3945416)]
[New Thread 0x7fffd73fc700 (LWP 3945419)]
[HW]   Platform 1, OpenCL profile FULL_PROFILE
[HW]   Platform 1, OpenCL version OpenCL 2.0 beignet 1.3
[New Thread 0x7fffde105700 (LWP 3945420)]
[Thread 0x7fffde105700 (LWP 3945420) exited]
[HW]   Device 0 is  available
[HW]   Device 0, type 4 = 0x00000004: gpu 
[HW]   Device 0, number of Compute Units: 20
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 512
[HW]   Device 0, max Work Items size for dimension: 1 512
[HW]   Device 0, max Work Items size for dimension: 2 512 
------------------------------------------------------------
```

A mechanism to select ldpc implementation is also available in the `ldpctest` phy simulator via the `-v`option, which can be used to specify the version of the ldpc shared library to be used.

#### Examples of ldpc shared lib selection when running ldpctest:

Loading libldpc_cuda.so, the cuda implementation of the ldpc decoder:

```
$ ./ldpctest -v _cuda
ldpctest -v _cuda
Initializing random number generator, seed 0
block length 8448: 
n_trials 1: 
SNR0 -2.000000: 
[CONFIG] get parameters from cmdline , debug flags: 0x00400000
[CONFIG] log_config: 2/3 parameters successfully set 
[CONFIG] log_config: 53/53 parameters successfully set 
[CONFIG] log_config: 53/53 parameters successfully set 
[CONFIG] log_config: 16/16 parameters successfully set 
[CONFIG] log_config: 16/16 parameters successfully set 
log init done
[CONFIG] loader: 2/2 parameters successfully set 
[CONFIG] loader.ldpc: 1/2 parameters successfully set 
[LOADER] library libldpc_cuda.so successfully loaded
...................................
```


Loading libldpc_cl.so, the opencl implementation of the ldpc decoder:

`make ldpc_cl`


```
$ ./ldpctest -v _cl
Initializing random number generator, seed 0
block length 8448: 
n_trials 1: 
SNR0 -2.000000: 
[CONFIG] get parameters from cmdline , debug flags: 0x00400000
[CONFIG] log_config: 2/3 parameters successfully set 
[CONFIG] log_config: 53/53 parameters successfully set 
[CONFIG] log_config: 53/53 parameters successfully set 
[CONFIG] log_config: 16/16 parameters successfully set 
[CONFIG] log_config: 16/16 parameters successfully set 
log init done
[CONFIG] loader: 2/2 parameters successfully set 
[CONFIG] loader.ldpc: 1/2 parameters successfully set 
[LOADER] library libldpc_cl.so successfully loaded
[HW]   Platform 0, OpenCL profile FULL_PROFILE
[HW]   Platform 0, OpenCL version OpenCL 2.1 LINUX
[HW]   Device 0 is  available
[HW]   Device 0, type 2 = 0x00000002: cpu 
[HW]   Device 0, number of Compute Units: 8
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 8192
[HW]   Device 0, max Work Items size for dimension: 1 8192
[HW]   Device 0, max Work Items size for dimension: 2 8192
[HW]   Platform 1, OpenCL profile FULL_PROFILE
[HW]   Platform 1, OpenCL version OpenCL 2.0 beignet 1.3
[HW]   Device 0 is  available
[HW]   Device 0, type 4 = 0x00000004: gpu 
[HW]   Device 0, number of Compute Units: 20
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 512
[HW]   Device 0, max Work Items size for dimension: 1 512
[HW]   Device 0, max Work Items size for dimension: 2 512
................................
```



### LDPC libraries
Libraries implementing the LDPC algorithms must be named `libldpc<_version>.so`, they must implement three functions: `nrLDPC_initcall` `nrLDPC_decod` and `nrLDPC_encod`. The prototypes for these functions is defined in [nrLDPC_defs.h](file://nrLDPC_defs.h).

`libldpc_cuda.so`has been tested with the `ldpctest` executable, usage from the softmodem's has to be tested.

`libldpc_cl.so`is under development.

[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
