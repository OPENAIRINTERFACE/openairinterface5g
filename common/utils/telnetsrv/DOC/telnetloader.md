loader command can be used to check loader configuration parameters and the list of loaded shared libraries and for each library the list of available functions.

```bash
softmodem> loader show params
loader parameters:
   Main executable build version: "Branch: develop-telnet-loader-fixes Abrev. Hash: e56ae69 Date: Fri Mar 9 16:47:08 2018 +0100"
   Default shared lib path: ""
   Max number of shared lib : 10
softmodem> loader show modules
4 shared lib have been dynamicaly loaded by the oai loader
   Module 0: telnetsrv
       Shared library build version: "Branch: develop-telnet-loader-fixes Abrev. Hash: e56ae69 Date: Fri Mar 9 16:47:08 2018 +0100"
       Shared library path: "libtelnetsrv.so"
       1 function pointers registered:
          function 0 add_telnetcmd at 0x7ff8b772a2b0
   Module 1: NB_IoT
       Shared library build version: ""
       Shared library path: "libNB_IoT.so"
       1 function pointers registered:
          function 0 RCConfig_NbIoT at 0x7ff8b6b1b390
   Module 2: coding
       Shared library build version: "Branch: develop-telnet-loader-fixes Abrev. Hash: e56ae69 Date: Fri Mar 9 16:47:08 2018 +0100"
       Shared library path: "libcoding.so"
       13 function pointers registered:
          function 0 init_td8 at 0x7ff8adde63a0
          function 1 init_td16 at 0x7ff8adde9760
          function 2 init_td16avx2 at 0x7ff8addec050
          function 3 phy_threegpplte_turbo_decoder8 at 0x7ff8adde6780
          function 4 phy_threegpplte_turbo_decoder16 at 0x7ff8adde9a90
          function 5 phy_threegpplte_turbo_decoder_scalar at 0x7ff8addef4a0
          function 6 phy_threegpplte_turbo_decoder16avx2 at 0x7ff8addec530
          function 7 free_td8 at 0x7ff8adde61d0
          function 8 free_td16 at 0x7ff8adde9590
          function 9 free_td16avx2 at 0x7ff8addebe80
          function 10 threegpplte_turbo_encoder_sse at 0x7ff8adde45b0
          function 11 threegpplte_turbo_encoder at 0x7ff8adde4a30
          function 12 init_encoder_sse at 0x7ff8adde49d0
   Module 3: oai_device
       Shared library build version: ""
       Shared library path: "liboai_device_usrp.so"
       1 function pointers registered:
          function 0 device_init at 0x7ff8ac16a7a0
softmodem>
```

[oai telnetserver home](telnetsrv.md)
[oai telnetserver usage home](telnetusage.md)
