<!-- SPDX-License-Identifier: CC-BY-4.0 -->

**Table of Contents**

[[_TOC_]]

## CI resources

The available resources, and their current usage, is indicated here: [Lockable
resources of jenkins-oai](https://jenkins-oai.eurecom.fr/lockable-resources/)

### Finding the resources used by a CI pipeline

To view the resource configuration for a specific pipeline run:
1. Open the Jenkins job, for example [RAN-SA-OAIUE-CN5G](https://jenkins-oai.eurecom.fr/job/RAN-SA-OAIUE-CN5G/).
2. In Builds, click a build number of a completed run.
3. Select `Parameters` in the run's sidebar.
4. Look for the `LockResources` parameter, which lists the resources configured for that run.
   For example, [RAN-SA-OAIUE-CN5G](https://jenkins-oai.eurecom.fr/job/RAN-SA-OAIUE-CN5G/10982/parameters)

## Pipelines

CI pipelines fall into two groups:

- [Event-triggered pipelines](#event-triggered-pipelines): triggered
  automatically when a pull request is opened or updated, or on a push event.
  Which pipelines run depends on the labels of the pull request.
- [Scheduled pipelines](#scheduled-pipelines): Runs on a nightly schedule
  against the latest `develop` or integration branch, independently of any pull
  request. They cover long-running or resource-intensive tests.

### Event-triggered pipelines

All pipelines in this group are started by the parent pipeline
[RAN-GitHub-Container-Parent](https://jenkins-oai.eurecom.fr/job/RAN-GitHub-Container-Parent/).

- Purpose: Runs the main CI workflow and downstream jobs based on labels set.
- Available labels:
  https://github.com/duranta-project/openairinterface5g/labels/documentation
  https://github.com/duranta-project/openairinterface5g/labels/BUILD-ONLY
  https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
  https://github.com/duranta-project/openairinterface5g/labels/5G-NR
  https://github.com/duranta-project/openairinterface5g/labels/nrUE
  https://github.com/duranta-project/openairinterface5g/labels/O-RU

This pipeline has two main stages: image build and image test. For the image build,
please also refer to the [dedicated documentation](../docker/README.md).

#### Image Build pipelines

- [RAN-ARM-Cross-Compile-Builder](https://jenkins-oai.eurecom.fr/job/RAN-ARM-Cross-Compile-Builder/)
  - Purpose: cross-compilation from Intel to ARM
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/BUILD-ONLY
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
    https://github.com/duranta-project/openairinterface5g/labels/O-RU
  - Images:
    - base image from [`Dockerfile.base.ubuntu.cross-arm64`](../docker/Dockerfile.base.ubuntu.cross-arm64)
    - build image from [`Dockerfile.build.ubuntu.cross-arm64`](../docker/Dockerfile.build.ubuntu.cross-arm64) (no target images)
- [RAN-RHEL-Cluster-Image-Builder](https://jenkins-oai.eurecom.fr/job/RAN-RHEL-Cluster-Image-Builder/)
  - Purpose: RHEL image build using the OpenShift cluster (using gcc/clang)
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/BUILD-ONLY
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
    https://github.com/duranta-project/openairinterface5g/labels/O-RU
  - Images:
    - base image from [`Dockerfile.base.rhel9`](../docker/Dockerfile.base.rhel9)
    - build image from [`Dockerfile.build.rhel9`](../docker/Dockerfile.build.rhel9), followed by
      - target image from [`Dockerfile.eNB.rhel9`](../docker/Dockerfile.eNB.rhel9)
      - target image from [`Dockerfile.gNB.rhel9`](../docker/Dockerfile.gNB.rhel9),
      - target image from [`Dockerfile.gNB.aw2s.rhel9`](../docker/Dockerfile.gNB.aw2s.rhel9)
      - target image from [`Dockerfile.nr-cuup.rhel9`](../docker/Dockerfile.nr-cuup.rhel9)
      - target image from [`Dockerfile.lteUE.rhel9`](../docker/Dockerfile.lteUE.rhel9)
      - target image from [`Dockerfile.nrUE.rhel9`](../docker/Dockerfile.nrUE.rhel9)
    - build image from [`Dockerfile.build.fhi72.rhel9`](../docker/Dockerfile.build.fhi72.rhel9), followed by
      - target image from [`Dockerfile.gNB.fhi72.rhel9`](../docker/Dockerfile.gNB.fhi72.rhel9)
    - build image from [`Dockerfile.phySim.rhel9`](../docker/Dockerfile.phySim.rhel9) (creates as direct target physical simulator
      image)
    - build image from [`Dockerfile.clang.rhel9`](../docker/Dockerfile.clang.rhel9) (compilation only, artifacts not used currently)
- [RAN-Ubuntu-Image-Builder](https://jenkins-oai.eurecom.fr/job/RAN-Ubuntu-Image-Builder/)
  - Purpose: Ubuntu image build using Docker
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/BUILD-ONLY
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
    https://github.com/duranta-project/openairinterface5g/labels/O-RU
  - Images:
    - base image from [`Dockerfile.base.ubuntu`](../docker/Dockerfile.base.ubuntu)
    - build image from [`Dockerfile.build.ubuntu`](../docker/Dockerfile.build.ubuntu), followed by
      - target image from [`Dockerfile.eNB.ubuntu`](../docker/Dockerfile.eNB.ubuntu)
      - target image from [`Dockerfile.gNB.ubuntu`](../docker/Dockerfile.gNB.ubuntu)
      - target image from [`Dockerfile.nr-cuup.ubuntu`](../docker/Dockerfile.nr-cuup.ubuntu)
      - target image from [`Dockerfile.nrUE.ubuntu`](../docker/Dockerfile.nrUE.ubuntu)
      - target image from [`Dockerfile.lteUE.ubuntu`](../docker/Dockerfile.lteUE.ubuntu)
      - target image from [`Dockerfile.lteRU.ubuntu`](../docker/Dockerfile.lteRU.ubuntu)
      - target image from [`Dockerfile.gNB.aerial.ubuntu`](../docker/Dockerfile.gNB.aerial.ubuntu)
    - build image from [`Dockerfile.build.fhi72.ubuntu`](../docker/Dockerfile.build.fhi72.ubuntu), followed by
      - target image from [`Dockerfile.gNB.fhi72.ubuntu`](../docker/Dockerfile.gNB.fhi72.ubuntu)
    - build unit tests from [`Dockerfile.unittest.ubuntu`](../ci-scripts/docker/Dockerfile.unittest.ubuntu), and run them
- [RAN-Ubuntu-ARM-Image-Builder](https://jenkins-oai.eurecom.fr/job/RAN-Ubuntu-ARM-Image-Builder/)
  - Purpose: ARM Ubuntu image build using Docker
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/BUILD-ONLY
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
    https://github.com/duranta-project/openairinterface5g/labels/O-RU
  - Images:
    - base image from [`Dockerfile.base.ubuntu`](../docker/Dockerfile.base.ubuntu)
    - build image from [`Dockerfile.build.ubuntu`](../docker/Dockerfile.build.ubuntu), followed by
      - target image from [`Dockerfile.gNB.ubuntu`](../docker/Dockerfile.gNB.ubuntu)
      - target image from [`Dockerfile.nr-cuup.ubuntu`](../docker/Dockerfile.nr-cuup.ubuntu)
      - target image from [`Dockerfile.nrUE.ubuntu`](../docker/Dockerfile.nrUE.ubuntu)
      - target image from [`Dockerfile.gNB.aerial.ubuntu`](../docker/Dockerfile.gNB.aerial.ubuntu)
- [RAN-Ubuntu-Jetson-Image-Builder](https://jenkins-oai.eurecom.fr/job/RAN-Ubuntu-Jetson-Image-Builder/)
  - Purpose: ARMv8 Ubuntu image build using Docker
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/BUILD-ONLY
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
    https://github.com/duranta-project/openairinterface5g/labels/O-RU
  - Images:
    - base image from [`Dockerfile.base.ubuntu`](../docker/Dockerfile.base.ubuntu)
    - build image from [`Dockerfile.build.ubuntu`](../docker/Dockerfile.build.ubuntu), followed by
      - target image from [`Dockerfile.gNB.ubuntu`](../docker/Dockerfile.gNB.ubuntu)
      - target image from [`Dockerfile.nr-cuup.ubuntu`](../docker/Dockerfile.nr-cuup.ubuntu)
      - target image from [`Dockerfile.nrUE.ubuntu`](../docker/Dockerfile.nrUE.ubuntu)

#### Image Test pipelines

- [OAI-FLEXRIC-RAN-Integration-Test](https://jenkins-oai.eurecom.fr/job/OAI-FLEXRIC-RAN-Integration-Test/)
  - Purpose: uses RFsimulator, tests FlexRIC/E2 interface and xApps
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
- [RAN-gNB-N300-Timing-Phytest-LDPC](https://jenkins-oai.eurecom.fr/view/RAN/job/RAN-gNB-N300-Timing-Phytest-LDPC/)
  - Purpose: performance test through phy-test mode, AMD T2 Telco Card and ORS Aurora LDPC offload tests with physims (`nr_dlsim` and `nr_ulsim`)
  - Hardware: USRP N310
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
- [RAN-L2-Sim-Test-4G](https://jenkins-oai.eurecom.fr/job/RAN-L2-Sim-Test-4G/)
  - Purpose: L2 simulator: skips physical layer and uses proxy between eNB and UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
- [RAN-LTE-FDD-LTEBOX-Container](https://jenkins-oai.eurecom.fr/job/RAN-LTE-FDD-LTEBOX-Container/)
  - Purpose: tests RRC inactivity timers, different bandwidths, IF4p5 fronthaul
  - Hardware: USRP B210, 2x COTS UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
- [RAN-LTE-FDD-OAIUE-OAICN4G-Container](https://jenkins-oai.eurecom.fr/job/RAN-LTE-FDD-OAIUE-OAICN4G-Container/)
  - Purpose: tests OAI 4G for 10 MHz/TM1; known to be unstable
  - Hardware: USRP B210 (eNB + lteUE)
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
- [RAN-LTE-TDD-2x2-Container](https://jenkins-oai.eurecom.fr/view/RAN/job/RAN-LTE-TDD-2x2-Container/)
  - Purpose: TM1 and TM2 test, IF4p5 fronthaul
  - Hardware: USRP N310, Quectel UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
- [RAN-LTE-TDD-LTEBOX-Container](https://jenkins-oai.eurecom.fr/job/RAN-LTE-TDD-LTEBOX-Container/)
  - Purpose: TM1 over bandwidths 5, 10, 20 MHz in Band 40, default scheduler for 20 MHz
  - Hardware: USRP B210, 2x COTS UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
- [RAN-NSA-B200-Module-LTEBOX-Container](https://jenkins-oai.eurecom.fr/job/RAN-NSA-B200-Module-LTEBOX-Container/)
  - Purpose: basic NSA test
  - Hardware: USRP B200 (eNB + gNB), Quectel UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
- [RAN-PhySim-Cluster-4G](https://jenkins-oai.eurecom.fr/job/RAN-PhySim-Cluster-4G/)
  - Purpose: tests 4G physical simulators (`dlsim`,`ulsim`, etc.)
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
  - Details: see [`./physical-simulators.md`](./physical-simulators.md) for an overview
- [RAN-PhySim-Cluster-5G](https://jenkins-oai.eurecom.fr/job/RAN-PhySim-Cluster-5G/)
  - Purpose: tests 5G physical simulators (`nr_dlsim`,`nr_ulsim`, etc.)
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
  - Details: see [`./physical-simulators.md`](./physical-simulators.md) for an overview
- [RAN-PhySim-GraceHopper-5G](https://jenkins-oai.eurecom.fr/job/RAN-PhySim-GraceHopper-5G/)
  - Purpose: tests 5G physical simulators (`nr_dlsim`,`nr_ulsim`, etc.)
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
  - Details: see [`./physical-simulators.md`](./physical-simulators.md) for an overview
- [RAN-RF-Sim-Test-4G](https://jenkins-oai.eurecom.fr/job/RAN-RF-Sim-Test-4G/)
  - Purpose: uses RFsimulator, for FDD 5, 10, 20 MHz with core, 5 MHz noS1
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/4G-LTE
- [RAN-RF-Sim-Test-5G](https://jenkins-oai.eurecom.fr/job/RAN-RF-Sim-Test-5G/)
  - Purpose: uses RFsimulator to evaluate performance and functionality across a variety of test scenarios
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
- [RAN-SA-AW2S-CN5G](https://jenkins-oai.eurecom.fr/job/RAN-SA-AW2S-CN5G/)
  - Purpose: 5G-NR SA test; multi UE testing using Amarisoft UE simulator
  - Hardware: AW2S Jaguar, Amarisoft UE simulator
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
  - Details: OpenShift cluster for CN deployment and container images for gNB deployment
- [RAN-SA-B200-Module-SABOX-Container](https://jenkins-oai.eurecom.fr/job/RAN-SA-B200-Module-SABOX-Container/)
  - Purpose: basic SA test (20 MHz TDD), F1, reestablishment, ...
  - Hardware: USRP B200, Quectel UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
- [RAN-SA-OAIUE-CN5G](https://jenkins-oai.eurecom.fr/job/RAN-SA-OAIUE-CN5G/)
  - Purpose: 5G-NR SA test setup with OAI nrUE
  - Hardware: USRP N310 (gNB + nrUE)
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
  - Details: OpenShift cluster for CN deployment and container images for gNB and UE deployment
- [RAN-SA-AERIAL-CN5G](https://jenkins-oai.eurecom.fr/job/RAN-SA-AERIAL-CN5G/)
  - Purpose: 5G-NR SA test setup with NVIDIA Aerial
  - Hardware: WNC RU + NVIDIA Aerial cuBB, Quectel UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
  - Details: container images for gNB deployment
- [RAN-SA-Multi-Antenna-CN5G](https://jenkins-oai.eurecom.fr/view/RAN/job/RAN-SA-Multi-Antenna-CN5G/)
  - Purpose: 5G-NR performance tests: 2x2 and 4x4 configuration, 60 MHz and 100 MHz bandwidth
  - Hardware: USRP N310, Quectel UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
- [RAN-SA-FHI72-CN5G](https://jenkins-oai.eurecom.fr/view/RAN/job/RAN-SA-FHI72-CN5G/)
  - Purpose: FHI 7.2 testing with 100 MHz bandwidth, 2 layers in DL
  - Hardware: VVDN LPRU, Quectel UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
  - Details: OpenShift cluster for CN deployment
- [RAN-SA-Handover-CN5G](https://jenkins-oai.eurecom.fr/job/RAN-SA-Handover-CN5G/)
  - Purpose: 5G-NR SA handover testing
  - Hardware: USRP B210 (cell0 + cell1), Quectel UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
  - Details:
    - OpenShift cluster for CN deployment
    - Attenuator (Mini-Circuits RC4DAT-6G-60), controlled from rocket
- [RAN-Channel-Simulation](https://jenkins-oai.eurecom.fr/job/RAN-Channel-Simulation/)
  - Purpose: PHY simulators using CUDA channel simulation, along with several unit tests
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
- [RAN-SA-AERIAL-OAIUE-CN5G](https://jenkins-oai.eurecom.fr/job/RAN-SA-AERIAL-OAIUE-CN5G/)
  - Purpose: 5G-NR SA test setup with NVIDIA Aerial and OAI UE
  - Hardware: WNC RU + NVIDIA Aerial cuBB, USRP B210 (nrUE)
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE
  - Details: OpenShift cluster for CN deployment and container images for gNB and UE deployment
- [RAN-SA-FHI72-MPLANE-CN5G](https://jenkins-oai.eurecom.fr/view/RAN/job/RAN-SA-FHI72-MPLANE-CN5G/)
  - Purpose: FHI 7.2 testing with 40 MHz (4x4 MIMO) and 100 MHz (2x2 MIMO) configuration
  - Hardware: Benetel 550 O-RU, Amarisoft UE simulator
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
  - Details:
    - OpenShift cluster for CN deployment
    - FHI 7.2 Configuration and Performance Management via NETCONF session of an O-RU
- [RAN-SA-ORU-CN5G](https://jenkins-oai.eurecom.fr/job/RAN-SA-ORU-CN5G/)
  - Purpose: FHI 7.2 testing with 40 MHz bandwidth, OAI O-RU
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/O-RU
- [RAN-SA-FHI72-FR2-CN5G](https://jenkins-oai.eurecom.fr/view/RAN/job/RAN-SA-FHI72-FR2-CN5G/)
  - Purpose: FHI 7.2 testing with 100 MHz bandwidth, 2 layers in DL
  - Hardware: Microamp FR2 O-RU, Quectel UE
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
  - Details: OpenShift cluster for CN deployment
- [RAN-VRT-Sim-Test-5G](https://jenkins-oai.eurecom.fr/job/RAN-VRT-Sim-Test-5G/)
  - Purpose: uses VRTsim to evaluate performance and functionality across a variety of test scenarios
  - Labels:
    https://github.com/duranta-project/openairinterface5g/labels/5G-NR
    https://github.com/duranta-project/openairinterface5g/labels/nrUE

### Scheduled pipelines

Runs on a nightly schedule against the latest `develop` or integration branch,
independently of any pull request. They cover long-running or
resource-intensive tests.

- [RAN-SA-FHI72-4x4-CN5G](https://jenkins-oai.eurecom.fr/view/RAN/job/RAN-SA-FHI72-4x4-CN5G/)
  - Purpose: FHI 7.2 testing with 100 MHz bandwidth, 4 layers in DL, 2 layers in UL
  - Hardware: VVDN LPRU, Benetel 550/650, LiteON FlexFi, Metanoia Cobra, AMD T2 Telco Card
  - Details: OpenShift cluster for CN deployment
- [RAN-Nightly-Channel-Simulation](https://jenkins-oai.eurecom.fr/job/RAN-Nightly-Channel-Simulation/)
  - Purpose: `test_channel_scalability` to test GPU channel simulation across different channel configurations
- [RAN-SA-OAIUE-CN5G-Longrun](https://jenkins-oai.eurecom.fr/job/RAN-SA-OAIUE-CN5G-Longrun/)
  - Purpose: 5G-NR SA test setup with OAI nrUE; iperf3 traffic test runs for 1 hour
  - Hardware: USRP N310 (gNB + nrUE)
  - Details: OpenShift cluster for CN deployment and container images for gNB and UE deployment

## How to reproduce CI results

The CI builds docker images at the beginning of every test run. To see the
exact command line steps, please refer to the `docker/Dockerfile.build.*`
files. Note that the console log of each pipeline run also lists the used
docker files.

The CI uses these images for *most* of the pipelines. It uses docker-compose to
orchestrate the images. To identify the docker-compose file, follow these
steps:

1. Each CI test run HTML lists the XML file used for a particular test run.
   Open the corresponding XML file under `ci-scripts/xml_files/`.
2. The XML file has a "test case" that refers to deployment of the image; it
   will reference a directory containing a YAML file (the docker-compose file)
   using option `yaml_path`, which will be under `ci-scripts/yaml_files/`. Go
   to this directory and open the docker-compose file.
3. The docker-compose file can be used to run images locally, or you can infer
   the used configuration file and possible additional options that are to be
   passed to the executable to run from source.

For instance, to see how the CI runs the multi-UE 5G RFsim test case, the above
steps look like this:

1. The first tab in the 5G RFsim test mentions
   `xml_files/container_5g_rfsim.xml`, so open
   `ci-scripts/xml_files/container_5g_rfsim.xml`.
2. This XML file has a `DeployGenObject` test case, referencing the directory
   `yaml_files/5g_rfsimulator`. The corresponding docker-compose file path is
   `ci-scripts/yaml_files/5g_rfsimulator/docker-compose.yaml`.
3. To know how to run the gNB, realize that there is a section `oai-gnb`. It
   mounts the configuration
   `ci-scripts/conf_files/gnb.sa.band78.106prb.rfsim.conf` (note that the path
   is relative to the directory in which the docker-compose file is located).
   Further, an environment variable `USE_ADDITIONAL_OPTIONS` is declared,
   referencing the relevant options `-E --rfsim` (you can ignore logging
   options). You would therefore run the gNB from source like this:
   ```
   sudo ./cmake_targets/ran_build/build/nr-softmodem -O ci-scripts/conf_files/gnb.sa.band78.106prb.rfsim.conf -E --rfsim
   ```
   To run this on your local machine, assuming you have a 5GC installed, you
   might need to change IP information in the config to match your core.

If you wish, you can rebuild CI images locally following [these
steps](../docker/README.md) and then use the docker-compose file directly.

Some tests are run from source (e.g.
`ci-scripts/xml_files/gnb_phytest_usrp_run.xml`), which directly give the
options they are run with.

## How to debug CI failures

It is possible to debug CI failures using the generated core dump and the image
used for the run. A script is provided (see developer instructions below) that,
provided the core dump file, container image, and the source tree, executes
`gdb` inside the container; using the core dump information, a developer can
investigate the cause of failure.

### Developer instructions

The CI team will send you a docker image and a core dump file, and the commit
as of which the pipeline failed. Let's assume the coredump is stored at
`/tmp/coredump.tar.xz`, and the image is in `/tmp/oai-nr-ue.tar.gz`. First, you
should check out the corresponding branch (or directly the commit), let's say
in `~/oai-branch-fail`. Now, unpack the core dump, load the image into docker,
and use the script [`docker/debug_core_image.sh`](../docker/debug_core_image.sh)
to open gdb, as follows:

```
cd /tmp
tar -xJf /tmp/coredump.tar.xz
docker load < /tmp/oai-nr-ue.tar.gz
~/oai-branch-fail/docker/debug_core_image.sh <image> /tmp/coredump ~/oai-branch-fail
```

where you replace `<image>` with the image loaded in `docker load`. The script
will start the container and open gdb; you should see information about where
the failure (e.g., segmentation fault) happened. If you just see `??`, the core
dump and container image don't match. Be also on the lookout for the
corresponding message from gdb:
```
warning: core file may not match specified executable file.
```

Once you quit `gdb`, the container image will be removed automatically.


### CI team instructions

The entrypoint scripts of all containers print the core pattern that is used on
the running machine. Search for `core_pattern` at the start of the container
logs to retrieve the possible location. Possible locations might be:

- a path: the corresponding directory must be mounted in the container to be
  writable
- systemd-coredumpd: see [documentation](https://systemd.io/COREDUMP/)
- abrt: see [documentation](https://abrt.readthedocs.io/en/latest/usage.html)
- apport: see [documentation](https://wiki.ubuntu.com/Apport)

See below for instructions on how to retrieve the core dump. Further, download
the image and store it to a file using `docker save`. Make sure to pick the
right image (Ubuntu or RHEL)!

#### Core dump in a file

> **This is not recommended, as files could pile up and fill the system disk
completely!** Prefer another method further down.

If the core pattern is a path: it should at least include the time in the
pattern name (suggested pattern: `/tmp/core.%e.%p.%t`) to correlate the time
the segfault occurred with the CI logs. If you identified the core dump,
copy the core dump from that machine; if identification is difficult, consider
rerunning the pipeline.

#### Core dump via systemd

Use the first command to list all core dumps. Scroll down to the core dump of
interest (it lists the executables in the last column; use the time to
correlate the segfault and the CI run).  Take the PID of the executable (first
column after the time). Dump the core dump to a location of your choice.

```
sudo coredumpctl list
sudo coredumpctl dump <PID> > /tmp/coredump
```

#### Core dump via abrt (automatic bug reporting tool)

> TBD: use the documentation page for the moment.

#### Core dump via apport

I did not find an easy way to use apport. Anyway, the systemd approach works
fine. So remove apport, install systemd-coredump, and verify it is the new
coredump handler:
```
sudo systemctl stop apport
sudo systemctl mask --now apport
sudo apt install systemd-coredump
# Verify this changed the core pattern to a pipe to systemd-coredump
sysctl kernel.core_pattern
```
