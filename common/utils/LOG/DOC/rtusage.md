## configuring the logging facility
The logging facility is fully configurable and it uses the [config module](../../../config/config.md) to get its parameters at init time. The [telnet server](../../telnetsrv/DOC/telnetsrv.md) includes a set of commands which can be used to dynamically modify the logging facility behavior

All logging facility parameters are defined in the log_config section. Some parameters are global to the logging facility, they modify the way messages are printed to stdout. Conversely, some parameters are specific to a component and  only modify the behavior for messages issued by a given component. A third type of parameters can be used to activate conditional debug code or dump messages or buffers.

### global parameters

| name | type | default | description |
|:---:|:---:|:---:|:----|
| `global_log_level` | `pre-defined string of char` | `info` |  Allows printing of messages up to the specified  level. Available levels, from lower to higher are `error`,`warn`,`info`,`debug`,`trace` |
| `global_log_online` | `boolean` | 1 (=true) | If false, all console messages are discarded, whatever their level |
| `global_log_options` | `list of pre-defined string of char` |  |  3 options can be specified to trigger the information added in the header of the message:  `nocolor`, disable color usage in log messages, usefull when redirecting logs to a file, where escape sequences used for color selection can be annoying, `level`, add a one letter level id in the message header (T,D,I,W,E for trace, debug, info, warning, error),`thread`, add the thread name in the message header|

### Component specific parameters
| name | type | default | description |
|:---:|:---:|:---:|:----|
| `<component>_log_level` | `boolean` | global log level, as defined by the  `global_log_level ` parameter) |
| `<component>_log_infile` | `boolean` | 0 = false| Triggers the redirection of log messages printed by the specified component in a file. The file path and name is /tmp/<componemt>.[extension] the extension is optional and component dependant, it can be `log `,  `dat `,  `txt `|

The list of components defined within oai can be retrieved from the  [config module](../../../config/config.md) traces, when asking for config module debugging info on the command line:

```bash
./lte-softmodem -O libconfig:<path to config file>:dbgl5

[LIBCONFIG] log_config.global_log_level: "info"
[CONFIG] global_log_online: 1
[CONFIG] log_config.global_log_online set to default value
[LIBCONFIG] log_config: 3/3 parameters successfully set, (1 to default value)
[LIBCONFIG] global_log_options[0]: nocolor
[LIBCONFIG] global_log_options[1]: level
[LIBCONFIG] global_log_options[2]: thread
[CONFIG] log_config 1 options set from command line
[LIBCONFIG] log_config.phy_log_level: "info"
[LIBCONFIG] log_config.mac_log_level: "info"
[CONFIG] log_config.sim_log_level set to default value "info"
[CONFIG] log_config.ocg_log_level set to default value "info"
[CONFIG] log_config.omg_log_level set to default value "info"
[CONFIG] log_config.opt_log_level set to default value "info"
[CONFIG] log_config.otg_log_level set to default value "info"
[CONFIG] log_config.otg_latency_log_level set to default value "info"
[CONFIG] log_config.otg_latency_bg_log_level set to default value "info"
[CONFIG] log_config.otg_gp_log_level set to default value "info"
[CONFIG] log_config.otg_gp_bg_log_level set to default value "info"
[CONFIG] log_config.otg_jitter_log_level set to default value "info"
[LIBCONFIG] log_config.rlc_log_level: "info"
[LIBCONFIG] log_config.pdcp_log_level: "info"
[LIBCONFIG] log_config.rrc_log_level: "info"
[CONFIG] log_config.nas_log_level set to default value "info"
[CONFIG] log_config.perf_log_level set to default value "info"
[CONFIG] log_config.oip_log_level set to default value "info"
[CONFIG] log_config.cli_log_level set to default value "info"
[CONFIG] log_config.msc_log_level set to default value "info"
[CONFIG] log_config.ocm_log_level set to default value "info"
[CONFIG] log_config.udp_log_level set to default value "info"
[CONFIG] log_config.gtpv1u_log_level set to default value "info"
[CONFIG] log_config.comp23?_log_level set to default value "info"
[CONFIG] log_config.s1ap_log_level set to default value "info"
[CONFIG] log_config.sctp_log_level set to default value "info"
[LIBCONFIG] log_config.hw_log_level: "info"
[CONFIG] log_config.osa_log_level set to default value "info"
[CONFIG] log_config.eral_log_level set to default value "info"
[CONFIG] log_config.mral_log_level set to default value "info"
[CONFIG] log_config.enb_app_log_level set to default value "info"
[CONFIG] log_config.flexran_agent_log_level set to default value "info"
[CONFIG] log_config.tmr_log_level set to default value "info"
[CONFIG] log_config.usim_log_level set to default value "info"
[CONFIG] log_config.localize_log_level set to default value "info"
[CONFIG] log_config.x2ap_log_level set to default value "info"
[CONFIG] log_config.loader_log_level set to default value "info"
[CONFIG] log_config.asn_log_level set to default value "info"
[LIBCONFIG] log_config: 38/38 parameters successfully set, (32 to default value)
[CONFIG] log_config 0 options set from command line
[CONFIG] phy_log_infile: 0
[CONFIG] log_config.phy_log_infile set to default value
[CONFIG] mac_log_infile: 0
[CONFIG] log_config.mac_log_infile set to default value
[CONFIG] sim_log_infile: 0
[CONFIG] log_config.sim_log_infile set to default value
[CONFIG] ocg_log_infile: 0
[CONFIG] log_config.ocg_log_infile set to default value
[CONFIG] omg_log_infile: 0
[CONFIG] log_config.omg_log_infile set to default value
[CONFIG] opt_log_infile: 0
[CONFIG] log_config.opt_log_infile set to default value
[CONFIG] otg_log_infile: 0
[CONFIG] log_config.otg_log_infile set to default value
[CONFIG] otg_latency_log_infile: 0
[CONFIG] log_config.otg_latency_log_infile set to default value
[CONFIG] otg_latency_bg_log_infile: 0
[CONFIG] log_config.otg_latency_bg_log_infile set to default value
[CONFIG] otg_gp_log_infile: 0
[CONFIG] log_config.otg_gp_log_infile set to default value
[CONFIG] otg_gp_bg_log_infile: 0
[CONFIG] log_config.otg_gp_bg_log_infile set to default value
[CONFIG] otg_jitter_log_infile: 0
[CONFIG] log_config.otg_jitter_log_infile set to default value
[CONFIG] rlc_log_infile: 0
[CONFIG] log_config.rlc_log_infile set to default value
[CONFIG] pdcp_log_infile: 0
[CONFIG] log_config.pdcp_log_infile set to default value
[CONFIG] rrc_log_infile: 0
[CONFIG] log_config.rrc_log_infile set to default value
[CONFIG] nas_log_infile: 0
[CONFIG] log_config.nas_log_infile set to default value
[CONFIG] perf_log_infile: 0
[CONFIG] log_config.perf_log_infile set to default value
[CONFIG] oip_log_infile: 0
[CONFIG] log_config.oip_log_infile set to default value
[CONFIG] cli_log_infile: 0
[CONFIG] log_config.cli_log_infile set to default value
[CONFIG] msc_log_infile: 0
[CONFIG] log_config.msc_log_infile set to default value
[CONFIG] ocm_log_infile: 0
[CONFIG] log_config.ocm_log_infile set to default value
[CONFIG] udp_log_infile: 0
[CONFIG] log_config.udp_log_infile set to default value
[CONFIG] gtpv1u_log_infile: 0
[CONFIG] log_config.gtpv1u_log_infile set to default value
[CONFIG] comp23?_log_infile: 0
[CONFIG] log_config.comp23?_log_infile set to default value
[CONFIG] s1ap_log_infile: 0
[CONFIG] log_config.s1ap_log_infile set to default value
[CONFIG] sctp_log_infile: 0
[CONFIG] log_config.sctp_log_infile set to default value
[CONFIG] hw_log_infile: 0
[CONFIG] log_config.hw_log_infile set to default value
[CONFIG] osa_log_infile: 0
[CONFIG] log_config.osa_log_infile set to default value
[CONFIG] eral_log_infile: 0
[CONFIG] log_config.eral_log_infile set to default value
[CONFIG] mral_log_infile: 0
[CONFIG] log_config.mral_log_infile set to default value
[CONFIG] enb_app_log_infile: 0
[CONFIG] log_config.enb_app_log_infile set to default value
[CONFIG] flexran_agent_log_infile: 0
[CONFIG] log_config.flexran_agent_log_infile set to default value
[CONFIG] tmr_log_infile: 0
[CONFIG] log_config.tmr_log_infile set to default value
[CONFIG] usim_log_infile: 0
[CONFIG] log_config.usim_log_infile set to default value
[CONFIG] localize_log_infile: 0
[CONFIG] log_config.localize_log_infile set to default value
[CONFIG] x2ap_log_infile: 0
[CONFIG] log_config.x2ap_log_infile set to default value
[CONFIG] loader_log_infile: 0
[CONFIG] log_config.loader_log_infile set to default value
[CONFIG] asn_log_infile: 0
[CONFIG] log_config.asn_log_infile set to default value
[LIBCONFIG] log_config: 38/38 parameters successfully set, (38 to default value)
[CONFIG] log_config 0 options set from command line
[CONFIG] PRACH_debug: 0
[CONFIG] log_config.PRACH_debug set to default value
[CONFIG] RU_debug: 0
[CONFIG] log_config.RU_debug set to default value
[CONFIG] UE_PHYPROC_debug: 0
[CONFIG] log_config.UE_PHYPROC_debug set to default value
[CONFIG] LTEESTIM_debug: 0
[CONFIG] log_config.LTEESTIM_debug set to default value
[CONFIG] DLCELLSPEC_debug: 0
[CONFIG] log_config.DLCELLSPEC_debug set to default value
[CONFIG] ULSCH_debug: 0
[CONFIG] log_config.ULSCH_debug set to default value
[CONFIG] RRC_debug: 0
[CONFIG] log_config.RRC_debug set to default value
[CONFIG] PDCP_debug: 0
[CONFIG] log_config.PDCP_debug set to default value
[CONFIG] DFT_debug: 0
[CONFIG] log_config.DFT_debug set to default value
[CONFIG] ASN1_debug: 0
[CONFIG] log_config.ASN1_debug set to default value
[CONFIG] CTRLSOCKET_debug: 0
[CONFIG] log_config.CTRLSOCKET_debug set to default value
[CONFIG] SECURITY_debug: 0
[CONFIG] log_config.SECURITY_debug set to default value
[CONFIG] NAS_debug: 0
[CONFIG] log_config.NAS_debug set to default value
[CONFIG] RLC_debug: 0
[CONFIG] log_config.RLC_debug set to default value
[CONFIG] UE_TIMING_debug: 0
[CONFIG] log_config.UE_TIMING_debug set to default value
[LIBCONFIG] log_config: 15/15 parameters successfully set, (15 to default value)
[CONFIG] log_config 0 options set from command line
[CONFIG] PRACH_dump: 0
[CONFIG] log_config.PRACH_dump set to default value
[CONFIG] RU_dump: 0
[CONFIG] log_config.RU_dump set to default value
[CONFIG] UE_PHYPROC_dump: 0
[CONFIG] log_config.UE_PHYPROC_dump set to default value
[CONFIG] LTEESTIM_dump: 0
[CONFIG] log_config.LTEESTIM_dump set to default value
[CONFIG] DLCELLSPEC_dump: 0
[CONFIG] log_config.DLCELLSPEC_dump set to default value
[CONFIG] ULSCH_dump: 0
[CONFIG] log_config.ULSCH_dump set to default value
[CONFIG] RRC_dump: 0
[CONFIG] log_config.RRC_dump set to default value
[CONFIG] PDCP_dump: 0
[CONFIG] log_config.PDCP_dump set to default value
[CONFIG] DFT_dump: 0
[CONFIG] log_config.DFT_dump set to default value
[CONFIG] ASN1_dump: 0
[CONFIG] log_config.ASN1_dump set to default value
[CONFIG] CTRLSOCKET_dump: 0
[CONFIG] log_config.CTRLSOCKET_dump set to default value
[CONFIG] SECURITY_dump: 0
[CONFIG] log_config.SECURITY_dump set to default value
[CONFIG] NAS_dump: 0
[CONFIG] log_config.NAS_dump set to default value
[CONFIG] RLC_dump: 0
[CONFIG] log_config.RLC_dump set to default value
[CONFIG] UE_TIMING_dump: 0
[CONFIG] log_config.UE_TIMING_dump set to default value
[LIBCONFIG] log_config: 15/15 parameters successfully set, (15 to default value)
[CONFIG] log_config 0 options set from command line
log init done

```
It can also be retrieved when using the telnet server, as explained  [below](### Using the telnet server to configure the logging facility)

### parameters to activate conditional code
| name | type | default | description |
|:---:|:---:|:---:|:----|
| `<flag>_debug` | `boolean` | 0 = false | Triggers the activation of conditional code identified by the specified flag.
| `<flag>_dump` | `boolean` | 0 = false| Triggers buffer dump, on the console in text form or in a file in matlab format, depending on the developper choice and forcasted usage|

### Using the configuration file to configure the logging facility
The following example sets all components log level to info, exept for hw,phy,mac,rlc,pdcp,rrc which log levels are set to error or warning.
```bash
    log_config :
    {
      global_log_level                      ="info";
      hw_log_level                          ="error";
      phy_log_level                         ="error";
      mac_log_level                         ="warn";
      rlc_log_level                         ="error"
      pdcp_log_level                        ="error";
      rrc_log_level                         ="error";
   };
```
### Using the command line to configure the logging facility
Command line parameter values supersedes values specified in the configuration file.
```bash
./lte-softmodem -O --log_config.global_log_options nocolor,level,thread  --log_config.prach_log_level debug --log_config.PRACH_debug
```
In this example to get all the debug PRACH messages it is necessary to also set the PRACH_debug flag. This is a choice from the developper.
The log messages will be printed whithout color and the header will include the lmessage evel and the thread name:
```bash
[PHY]I ru thread Time in secs now: 104652566
[PHY]I ru thread Time in secs last pps: 91827117
[PHY]I ru thread RU 0 rf device ready
[PHY]I ru thread RU 0 no asynch_south interface
[MAC]E rxtx processing SCHED_MODE=0
[PHY]I lte-softmodem PRACH (eNB) : running rx_prach for subframe 1, prach_FreqOffset 2, prach_ConfigIndex 0 , rootSequenceIndex 0
[PHY]I lte-softmodem prach_I0 = 0.0 dB
[PHY]I rxtx processing max_I0 21, min_I0 0
[PHY]I lte-softmodem PRACH (eNB) : running rx_prach for subframe 1, prach_FreqOffset 2, prach_ConfigIndex 0 , rootSequenceIndex 0
[PHY]I lte-softmodem PRACH (eNB) : running rx_prach for subframe 1, prach_FreqOffset 2, prach_ConfigIndex 0 , rootSequenceIndex 0
```

### Using the telnet server to configure the logging facility
The telnet server includes a `log` command which can be used to dymically modify the logging facility configuration parameters.
[telnet server ***softmodem log*** commands](../../telnetsrv/DOC/telnetlog.md)

[logging facility  main page](log.md)
[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
