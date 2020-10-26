#LDPC coder/decoder implementation
The LDPC coder and decoder are implemented in a shared library, dynamically loaded at run-time using the [oai shared library loader](file://../../../../common/utils/DOC/loader.md). The code loading the LDPC library is in [nrLDPC_load.c](file://../nrLDPC_load.c), in function `load_nrLDPClib`, which must be called at init time.

## Selecting the LDPC library at run time

By default the function `int load_nrLDPClib(void)` looks for `libldpc.so`, this default behavior can be changed using the oai loader configuration options in the configuration file or from the command line as shown below:

>loading `libldpc_optim8seg.so` instead of `libldpc.so`

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

Today, this mechanism is not available in the `ldpctest` phy simulator which doesn't initialize the [configuration module](file://../../../../common/config/DOC/config.md). loads `libldpc.so` and `libldpc_orig.so` to compare the performance of the two implementations.

###LDPC libraries
Libraries implementing the LDPC algorithms must be named `libldpc<_version>.so`, they must implement two functions: `nrLDPC_decod` and `nrLDPC_encod`. The prototypes for these functions is defined in [nrLDPC_defs.h](file://nrLDPC_defs.h).

[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
