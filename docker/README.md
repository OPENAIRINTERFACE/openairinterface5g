<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI Docker/Podman Build and Usage Procedures</font></b>
    </td>
  </tr>
</table>

---

**Table of Contents**

[[_TOC_]]

---

# 1. Build Strategy #

For all platforms, the strategy for building docker/podman images is the same:

*  First we create a common shared image `ran-base` that contains:
   -  the latest source files (by using the `COPY` function)
   -  all the means to build an OAI RAN executable
      *  all packages, compilers, ...
      *  especially UHD is installed 
*  Then, from the `ran-base` shared image, we create a shared image `ran-build`
   into which all targets are compiled.
*  Then from the `ran-build` shared image, we can build target images for:
   -  eNB
   -  gNB (with UHD)
   -  gNB (with AW2S), only on RHEL9
   -  lte-UE
   -  nr-UE

   These target images will only contain:
   -  the generated executable (for example `lte-softmodem`)
   -  the generated shared libraries (for example `liboai_usrpdevif.so`)
   -  the needed libraries and packages to run these generated binaries
   -  Some configuration file templates
   -  Some tools (such as `ping`, `ifconfig`)

Note that on every push to develop (i.e., typically after integrating merge
requests), we build all images and push them to [Docker Hub](https://hub.docker.com/u/oaisoftwarealliance). To pull them, do

```
docker pull oaisoftwarealliance/oai-gnb:develop
docker pull oaisoftwarealliance/oai-nr-ue:develop
docker pull oaisoftwarealliance/oai-enb:develop
docker pull oaisoftwarealliance/oai-lte-ue:develop
```
Have a look at [this README](../ci-scripts/yaml_files/5g_rfsimulator/README.md) to get some
information on how to use the images.

# 2. File organization #

Dockerfiles are named with the following naming convention: `Dockerfile.${target}.${OS-version}`

Targets can be:

-  `base` for an image named `ran-base` (shared image)
-  `build` for an image named `ran-build` (shared image)
-  `eNB` for an image named `oai-enb`
-  `gNB` for an image named `oai-gnb`
-  `nr-cuup` for an image named `oai-nr-cuup`
-  `gNB.aw2s` for an image named `oai-gnb-aw2s`
-  `lteUE` for an image named `oai-lte-ue`
-  `nrUE` for an image named `oai-nr-ue`

The currently-supported OS are:

- `rhel9` for Red Hat Entreprise Linux (including images for an OpenShift cluster)
- `ubuntu20` for Ubuntu 20.04 LTS
- `rocky` for Rocky-Linux 8.7

For more details regarding the build on an Openshift Cluster, see [OpenShift README](../openshift/README.md).

# 3. Building using `docker` under Ubuntu 20.04 #

## 3.1. Pre-requisites ##

* `git` installed
* `docker-ce` installed
* Pulling `ubuntu:focal` from DockerHub

## 3.2. Building the shared images ##

There are two shared images: one that has all dependencies, and a second that compiles all targets (eNB, gNB, [nr]UE).

```bash
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git
cd openairinterface5g
git checkout develop
docker build --target ran-base --tag ran-base:latest --file docker/Dockerfile.base.ubuntu20 .
docker build --target ran-build --tag ran-build:latest --file docker/Dockerfile.build.ubuntu20 .
```

After building both:

```bash
docker image ls
REPOSITORY          TAG                 IMAGE ID            CREATED             SIZE
ran-build           latest              f2633a7f5102        1 minute ago        6.81GB
ran-base            latest              5c9c02a5b4a8        1 minute ago        2.4GB

...
```

Note that the steps are identical for `rocky-linux`.

### 3.2.1. Additional build otions

This is only available for the Ubuntu-20 version.

You can, for example, create a `sanitizer` version of the ran-build image.

```bash
docker build --target ran-build --tag ran-build:latest --file docker/Dockerfile.build.ubuntu20 --build-arg "BUILD_OPTION=--sanitize" .
```

Currently the `--sanitize` option for `build_oai` enables:

* [Address Sanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)
* [Undefined Behavior Sanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)

After building:

```bash
docker image ls
REPOSITORY          TAG                 IMAGE ID            CREATED             SIZE
ran-build           latest              f2633a7f5102        1 minute ago        8.78GB
...
```

Note that the image is much bigger.

You can also use this docker build arguments to pass any available option(s) on the `build-oai` script.

## 3.3. Building any target image ##

For example, the eNB:

```bash
docker build --target oai-enb --tag oai-enb:latest --file docker/Dockerfile.eNB.ubuntu20 .
```

After a while:

```
docker image ls
REPOSITORY          TAG                 IMAGE ID            CREATED             SIZE
oai-enb             latest              25ddbd8b7187        1 minute ago        516MB
<none>              <none>              875ea3b05b60        8 minutes ago       8.18GB
ran-build           latest              f2633a7f5102        1 hour ago          6.81GB
ran-base            latest              5c9c02a5b4a8        1 hour ago          2.4GB
```

Do not forget to remove the temporary image:

```
docker image prune --force
```

Note that the steps are identical for `rocky-linux`.

If you have used the sanitizer option, then you should also pass it when building the target image:

```bash
docker build --target oai-gnb --tag oai-gnb:latest --file docker/Dockerfile.gNB.ubuntu20 --build-arg "BUILD_OPTION=--sanitize" .
```

Normally the target image will be around 200 Mbytes bigger.

# 4. Building using `podman` under Red Hat Entreprise Linux 8.2 #

Analogous to the above steps:
```
sudo podman build --target ran-base --tag ran-base:latest --file docker/Dockerfile.base.rhel9 .
sudo podman build --target ran-build --tag ran-build:latest --file docker/Dockerfile.build.rhel9 .
sudo podman build --target oai-enb --tag oai-enb:latest --file docker/Dockerfile.eNB.rhel9 .
```

# 5. Running modems using `docker` under Ubuntu 18.04 #

The easiest is to run them from a `docker-compose` file, which is used by the
CI to test OAI. Some folders under `ci-scripts/yaml_files` have a README that
you can follow. For 5G, the easiest is to start with the RFsimulator, as
described in [this README](../ci-scripts/yaml_files/5g_rfsimulator/README.md)
(you would of course use your own images instead of downloading them from
Docker hub).

For an example using a B210, please refer to [this `docker-compose`
file](../ci-scripts/yaml_files/sa_b200_gnb/docker-compose.yml).

It is also possible to mount your own configuration file. The following
docker-compose file can be used to start a gNB using a B210 and your own
config, located at `/tmp/gnb.conf`:
```
version: '3.8'

services:
    gnb_mono_tdd:
        image: oai-gnb:latest
        privileged: true
        container_name: sa-b200-gnb
        environment:
            USE_B2XX: 'yes'
            USE_ADDITIONAL_OPTIONS: --sa -E --continuous-tx
        volumes:
            - /dev:/dev
            - /tmp/gnb.conf:/opt/oai-gnb/etc/gnb.conf
        networks:
            public_net:
                ipv4_address: 192.168.68.194
        healthcheck:
            # pgrep does NOT work
            test: /bin/bash -c "ps aux | grep -v grep | grep -c softmodem"
            interval: 10s
            timeout: 5s
            retries: 5

networks:
    public_net:
        name: sa-b200-gnb-net
        ipam:
            config:
                - subnet: 192.168.68.192/26
        driver_opts:
            com.docker.network.bridge.name: "sa-gnb-net"
```

You should also change the `image` to the right image name and tag of the gNB
you are using. Start like this:
```
docker-compose up    # gNB in foreground
docker-compose up -d # gNB in background
```
Stop it like this (in both cases):
```
docker-compose down
```

Note that in the above case, ALL devices are passed into the container (by
mounting `/dev`), which allows the container to access all devices connected to
the host!

# 6. Running modems using `podman` under Red Hat Entreprise Linux 8.2 #

TODO.
