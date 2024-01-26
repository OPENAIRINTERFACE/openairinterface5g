<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OpenAirInterface documentation overview</font></b>
    </td>
  </tr>
</table>

[[_TOC_]]

# General

- [FEATURE_SET.md](./FEATURE_SET.md): lists supported features
- [GET_SOURCES.md](./GET_SOURCES.md): how to download the sources
- [BUILD.md](./BUILD.md): how to build the sources
- [cross-compile.md](./cross-compile.md): how to cross-compile OAI for ARM
- [clang-format.md](./clang-format.md): how to format the code
- [sanitizers.md](./dev_tools/sanitizers.md): how to run with ASan/UBSan/MemSAN/TSan
- [environment-variables.md](./environment-variables.md): the environment variables used by OAI

There is some general information in the [OpenAirInterface Gitlab Wiki](https://gitlab.eurecom.fr/oai/openairinterface5g/-/wikis/home)

# Tutorials

- Step-by-step tutorials to set up 5G:
  * [OAI 5GC](./NR_SA_Tutorial_OAI_CN5G.md)
  * [OAI gNB with COTS UE](./NR_SA_Tutorial_COTS_UE.md)
  * [OAI NR-UE](./NR_SA_Tutorial_OAI_nrUE.md)
- [RUNMODEM.md](./RUNMODEM.md): Generic information on how to
  * Run simulators
  * Run with hardware
  * Specific OAI modes (phy-test, do-ra, noS1)
  * (5G) Using SDAP and custom DRBs
  * IF setups and arbitrary frequencies
  * MIMO
- [How to run OAI with O-RAN 7.2 FHI](./ORAN_FHI7.2_Tutorial.md)
- [How to run a 5G-NSA setup](./TESTING_GNB_W_COTS_UE.md)
- [How to run a 4G setup using L1 simulator](./L1SIM.md) _Note: we recommend the RFsimulator_
- [How to use the L2 simulator](./L2NFAPI.md)
- [How to use multiple BWPs](./RUN_NR_multiple_BWPs.md)
- [How to run OAI-VNF and OAI-PNF](./RUN_NR_NFAPI.md) _Note: does not work currently_
- [How to use the positioning reference signal (PRS)](./RUN_NR_PRS.md)
- [How to use device-to-device communication (D2D, 4G)](./d2d_emulator_setup.txt)
- [How to run with E2 agent](../openair2/E2AP/README.md)

Legacy unmaintained files:
- `L2NFAPI_NOS1.md`, `L2NFAPI_S1.md`: old L2simulator, not valid anymore
- `SystemX-tutorial-design.md`
- `UL_MIMO.txt`

# Designs

- General software architecture notes: [SW_archi.md](./SW_archi.md)
- [Information on E1](./E1-design.md)
- [Information on F1](./F1-design.md)
- [Information on how NR nFAPI works](./NR_NFAPI_archi.md)
- [Flow graph of the L1 in gNB](SW-archi-graph.md)
- [L1 threads in NR-UE](./nr-ue-design.md)

Legacy unmaintained files:
- 5Gnas.md

# Building and running from images

- [How to build images](../docker/README.md)
- [How to run 5G with the RFsimulator from images](../ci-scripts/yaml_files/5g_rfsimulator/README.md)
- [How to run 4G with the RFsimulator from images](../ci-scripts/yaml_files/4g_rfsimulator_fdd_05MHz/README.md)
- [How to run 5G with the L2simulator from images](../ci-scripts/yaml_files/5g_l2sim_tdd/README.md)
- [How to run images in OpenShift](../openshift/README.md)

# Libraries

## General

- The [T tracer](../common/utils/T/DOC/T.md): a generic tracing tool (VCD, Wireshark, GUI, to save for later, ...)
- [OPT](../openair2/UTIL/OPT/README.txt): how to trace to wireshark
- The [configuration module](../common/config/DOC/config.md)
- The [logging module](../common/utils/LOG/DOC/log.md)
- The [shared object loader](../common/utils/DOC/loader.md)
- The [threadpool](../common/utils/threadPool/thread-pool.md) used in L1
- The [LDPC implementation](../openair1/PHY/CODING/DOC/LDPCImplementation.md) is a shared library

## SDRs

Some directories under `radio` contain READMEs:

- [RFsimulator](../radio/rfsimulator/README.md)
- [USRP](../radio/USRP/README.md)
- [BladeRF](../radio/BLADERF/README)
- [IQPlayer](../radio/iqplayer/DOC/iqrecordplayer_usage.md), and [general documentation](./iqrecordplayer_usage.md)

The other SDRs (AW2S, LimeSDR, ...) have no READMEs.

## Special-purpose libraries

- OAI has two scopes, one based on Xforms, one on Qt5, described in [this README](../openair1/PHY/TOOLS/readme.md)
- OAI comes with an integrated [telnet server](../common/utils/telnetsrv/DOC/telnethelp.md) to monitor and control
- OAI comes with an integrated [web server](../common/utils/websrv/DOC/websrv.md)

# Testing

- [UnitTests.md](./UnitTests.md) explains the unit testing setup
- [TESTBenches.md](./TESTBenches.md) lists the CI setup and links to pipelines
