The log command can be used to get the status of the log parameters and to dynamically modify these parameters. The log command has its own [help](telnethelp.md#oai-telnet-server-specific-commands-help)
```bash
softmodem>  softmodem log disable 0-35
log level/verbosity  comp 0 PHY set to info / medium (disabled)
log level/verbosity  comp 1 MAC set to info / medium (disabled)
log level/verbosity  comp 2 EMU set to info / medium (disabled)
log level/verbosity  comp 3 OCG set to info / medium (disabled)
log level/verbosity  comp 4 OMG set to info / medium (disabled)
log level/verbosity  comp 5 OPT set to info / medium (disabled)
log level/verbosity  comp 6 OTG set to info / medium (disabled)
log level/verbosity  comp 7 OTG_LATENCY set to info / medium (disabled)
log level/verbosity  comp 8 OTG_LATENCY_BG set to info / medium (disabled)
log level/verbosity  comp 9 OTG_GP set to info / medium (disabled)
log level/verbosity  comp 10 OTG_GP_BG set to info / medium (disabled)
log level/verbosity  comp 11 OTG_JITTER set to info / medium (disabled)
log level/verbosity  comp 12 RLC set to info / medium (disabled)
log level/verbosity  comp 13 PDCP set to info / medium (disabled)
log level/verbosity  comp 14 RRC set to info / medium (disabled)
log level/verbosity  comp 15 NAS set to info / medium (disabled)
log level/verbosity  comp 16 PERF set to info / medium (disabled)
log level/verbosity  comp 17 OIP set to info / medium (disabled)
log level/verbosity  comp 18 CLI set to info / medium (disabled)
log level/verbosity  comp 19 MSC set to info / medium (disabled)
log level/verbosity  comp 20 OCM set to info / medium (disabled)
log level/verbosity  comp 21 UDP set to info / medium (disabled)
log level/verbosity  comp 22 GTPV1U set to info / medium (disabled)
log level/verbosity  comp 23 comp23? set to info / medium (disabled)
log level/verbosity  comp 24 S1AP set to info / medium (disabled)
log level/verbosity  comp 25 SCTP set to info / medium (disabled)
log level/verbosity  comp 26 HW set to info / medium (disabled)
log level/verbosity  comp 27 OSA set to info / medium (disabled)
log level/verbosity  comp 28 eRAL set to info / medium (disabled)
log level/verbosity  comp 29 mRAL set to info / medium (disabled)
log level/verbosity  comp 30 ENB_APP set to info / medium (disabled)
log level/verbosity  comp 31 TMR set to info / medium (disabled)
log level/verbosity  comp 32 USIM set to info / medium (disabled)
log level/verbosity  comp 33 LOCALIZE set to info / medium (disabled)
log level/verbosity  comp 34 RRH set to info / medium (disabled)
softmodem> softmodem log show
Available log levels:
   emerg alert crit error warn notice info debug file trace
Available verbosity:
   none low medium high full
component                 verbosity  level  enabled
00               PHY:    medium      info  N
01               MAC:    medium      info  N
02               EMU:    medium      info  N
03               OCG:    medium      info  N
04               OMG:    medium      info  N
05               OPT:    medium      info  N
06               OTG:    medium      info  N
07       OTG_LATENCY:    medium      info  N
08    OTG_LATENCY_BG:    medium      info  N
09            OTG_GP:    medium      info  N
10         OTG_GP_BG:    medium      info  N
11        OTG_JITTER:    medium      info  N
12               RLC:    medium      info  N
13              PDCP:    medium      info  N
14               RRC:    medium      info  N
15               NAS:    medium      info  N
16              PERF:    medium      info  N
17               OIP:    medium      info  N
18               CLI:    medium      info  N
20               OCM:    medium      info  N
21               UDP:    medium      info  N
22            GTPV1U:    medium      info  N
23           comp23?:    medium      info  N
24              S1AP:    medium      info  N
25              SCTP:    medium      info  N
26                HW:    medium      info  N
27               OSA:    medium      info  N
28              eRAL:    medium      info  N
29              mRAL:    medium      info  N
30           ENB_APP:    medium      info  N
31               TMR:    medium      info  N
32              USIM:    medium      info  N
33          LOCALIZE:    medium      info  N
34               RRH:    medium      info  N
35           comp36?:    medium      info  Y
36            LOADER:    medium     alert  Y
softmodem> softmodem log level_error 0-4
log level/verbosity  comp 0 PHY set to error / medium (enabled)
log level/verbosity  comp 1 MAC set to error / medium (enabled)
log level/verbosity  comp 2 EMU set to error / medium (enabled)
log level/verbosity  comp 3 OCG set to error / medium (enabled)
log level/verbosity  comp 4 OMG set to error / medium (enabled)
softmodem> exit
Connection closed by foreign host.
```

[oai telnetserver home](telnetsrv.md)
[oai telnetserver usage home](telnetusage.md)
[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
