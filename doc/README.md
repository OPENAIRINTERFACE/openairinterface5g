<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# OpenAirInterface documentation overview

This is the general overview page of the OpenAirInterface documentation.
This page groups links to general information, tutorials, design documents, radio integration, and special-purpose libraries.

**IMPORTANT NOTE:**
Before reading this documentation, we strongly advise you to keep your own repository rebased on `develop`
or at least to checkout the documentation on the version of the repository you are using.
Then the documentation will better reflect the features available in your repository so that you may avoid some errors.
Beware if you previously pulled the `develop` branch that your repository may be now behind `develop`.

[[_TOC_]]

## General

- [FEATURE_SET.md](./FEATURE_SET.md): lists supported features
- [GET_SOURCES.md](./GET_SOURCES.md): how to download the sources
- [BUILD.md](./BUILD.md): how to build the sources
- [environment-variables.md](./environment-variables.md): the environment variables used by OAI
- [tuning_and_security.md](./tuning_and_security.md): performance and security considerations
- [Supported_Hardware_Operating_System.md](./Supported_Hardware_Operating_System.md): List of supported hardware and operating system for OAI

There is some general information in the [OpenAirInterface Gitlab Wiki](https://gitlab.eurecom.fr/oai/openairinterface5g/-/wikis/home)

## Tutorials

- Step-by-step tutorials to set up 5G:
    * [OAI 5GC](./NR_SA_Tutorial_OAI_CN5G.md)
    * [OAI gNB with COTS UE](./NR_SA_Tutorial_COTS_UE.md)
    * [OAI NR-UE](./NR_SA_Tutorial_OAI_nrUE.md)
    * [Multiple OAI NR-UE with RFsimulator](./NR_SA_Tutorial_OAI_multi_UE.md)
- [RUNMODEM.md](./RUNMODEM.md): Information on how to run the gNB
    * Run simulators
    * Run with hardware
    * (5G) Using SDAP and custom DRBs
    * IF setups and arbitrary frequencies
    * MIMO
- [nrUE-specific configuration and modes](./runmodem-nrue.md)
    * Specific OAI modes (phy-test, do-ra, noS1)
- [How to run OAI with O-RAN 7.2 FHI](./ORAN_FHI7.2_Tutorial.md)
- [How to run a 5G-NSA setup](./TESTING_OAI_NSA_COTS_UE.md)
- [How to run a 4G setup using L1 simulator](./L1SIM.md) _Note: we recommend the RFsimulator_
- [How to use the L2 simulator](./L2NFAPI.md)
- [How to use the OAI channel simulator](../openair1/SIMULATION/TOOLS/DOC/channel_simulation.md)
- [How to run an NTN setup](./ntn-configuration.md)
- [How to use GPU-accelerated channel simulation](../openair1/SIMULATION/TOOLS/DOC/gpu_acceleration.md)
- [How to run OAI-VNF and OAI-PNF](./nfapi.md): how to run the FAPI/nFAPI split,
  including some general remarks on FAPI/nFAPI.
- [How to use the positioning reference signal (PRS)](./RUN_NR_PRS.md)
- [How to use device-to-device communication (D2D, 4G)](./d2d_emulator_setup.md)
- [How to run with E2 agent](../openair2/E2AP/README.md)
- [How to run the physical simulators](./physical-simulators.md)
- [How to setup OAI with Nvidia Aerial and Foxconn](./Aerial_FAPI_Split_Tutorial.md)
- [How to setup OAI with LDPC accelerators (Xilinx T2/Intel ACCs)](./LDPC_OFFLOAD_SETUP.md)
- [How to run the UE with LDPC CUDA accelerator](./NR_SA_Tutorial_OAI_nrUE_GPU_offload.md)
- [How to setup OAI with the XDMA FPGA LDPC accelerator](./LDPC_XDMA_offload_setup.md)
- [How to do a handover](./handover-tutorial.md)
- [How to setup gNB frequency](./gNB_frequency_setup.md)
- [How to use the RT data recording app](./data_recording.md)
- [How to use packages](./packages.md)

Legacy unmaintained files:
- [`L2NFAPI_NOS1.md`](./L2NFAPI_NOS1.md), [`L2NFAPI_S1.md`](./L2NFAPI_S1.md):
  old L2simulator, not valid anymore

## Designs

- General software architecture notes: [SW_archi.md](./SW_archi.md)
- [Information on E1](./E1AP/E1-design.md)
- [Information on F1](./F1AP/F1-design.md)
- [Information on GTP-U](./GTP/gtp-design.md)
- [Information on how NR nFAPI works](./NR_NFAPI_archi.md)
- [Flow graph of the L1 in gNB](SW-archi-graph.md)
- [L1 threads in NR-UE](./nr-ue-design.md)
- [Information on gNB MAC](./MAC/mac-usage.md)
- [Information on gNB RRC](./RRC/rrc-usage.md)
- [Information on analog beamforming implementation](./analog_beamforming.md)
- [Information on the UE 5G NAS implementation](./5Gnas.md)
- [Information on UL-MIMO](./UL_MIMO.md): UL-MIMO specific notes
- [Information on NRPPA](./NRPPA/NRPPA-call-flow.md)

## Building and running from images

- [How to build images](../docker/README.md)
- [How to run 5G with the RFsimulator from images](../ci-scripts/yaml_files/5g_rfsimulator/README.md)
- [How to run 4G with the RFsimulator from images](../ci-scripts/yaml_files/4g_rfsimulator_fdd_05MHz/README.md)
- [How to run physical simulators in OpenShift](../openshift/README.md)

## Libraries

### General

- The [T tracer](../common/utils/T/DOC/T.md): a generic tracing tool (VCD, Wireshark, GUI, to save for later, ...)
- [OPT](../openair2/UTIL/OPT/README.txt): how to trace to wireshark
- The [configuration module](../common/config/DOC/config.md)
- The [logging module](../common/utils/LOG/DOC/log.md)
- The [shared object loader](../common/utils/DOC/loader.md)
- The [threadpool](../common/utils/threadPool/thread-pool.md) used in L1
- The [LDPC implementation](../openair1/PHY/CODING/DOC/LDPCImplementation.md) is a shared library
- The [time management](time_management.md) module

### Radios

Some directories under `radio` contain READMEs:

- [RFsimulator](../radio/rfsimulator/README.md)
- [USRP](../radio/USRP/README.md)
- [BladeRF](../radio/BLADERF/README.md)
- [IQPlayer](../radio/iqplayer/DOC/iqrecordplayer_usage.md), and [general documentation](./iqrecordplayer_usage.md)
- [fhi_72](../radio/fhi_72/README.md)
- [vrtsim](../radio/vrtsim/README.md)
- [rf_emulator](../radio/emulator/README.md)
- [zmq](../radio/zmq/README.md)

The other SDRs (AW2S, LimeSDR, ...) have no READMEs.

### Special-purpose libraries

- OAI has two scopes: one based on Xforms and one based on imgui, described in [this README](../openair1/PHY/TOOLS/readme.md)
- OAI comes with an integrated [telnet server](../common/utils/telnetsrv/DOC/telnethelp.md) to monitor and control
- OAI comes with an integrated [web server](../common/utils/websrv/DOC/websrv.md)

## Testing

- [UnitTests.md](./UnitTests.md) explains the unit testing setup
- Component tests are under `tests/`. Currently, the following component tests are supported
  - A simple CU-UP tester, see the corresponding [README.md](../tests/nr-cuup/README.md).
  - A CU-NRPPA tester, see the corresponding [README.md](../tests/nr-cu-nrppa/README.md).
  - A UE NAS tester, see the corresponding [README.md](../tests/nr-ue-nas-simulator/README.md).
- [TESTBenches.md](./TESTBenches.md) lists the CI setup and links to pipelines
- The CI setup uses a [custom framework](../ci-scripts/README.md) to run
  end-to-end tests.

## Developer tools

- [code-style-contrib.md](./code-style-contrib.md): overall working practices, code style, and review process
- [git-guide.md](./git-guide.md): Git how-tos — commit signing setup, branch
  management, submodules, recovering from mistakes, reusing conflict
  resolutions (rerere)
- [cross-compile.md](./cross-compile.md): how to cross-compile OAI for ARM
- [clang-format.md](./clang-format.md): how to format the code. See also the
  next entry for an error detection tool.
- [formatting](../tools/formatting/README.md) is a clang-format error detection tool
- [cppcheck](../tools/cppcheck/README.md) for static code analysis
- [sanitizers.md](./dev_tools/sanitizers.md): how to run with ASan/UBSan/MemSAN/TSan
- [iwyu](../tools/iwyu/README.md) is a tool to detect `#include` errors
- [docker-dev-env](../tools/docker-dev-env/README.md) is a ubuntu24 docker development environment
- [performance analysis with tracy](./dev_tools/tracy.md)
- [doc_best_practices.md](./doc_best_practices.md): overall best practices for writing documentations
