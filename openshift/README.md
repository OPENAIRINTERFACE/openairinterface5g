<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OpenShift Build and Usage Procedures</font></b>
    </td>
  </tr>
</table>

-----

**CAUTION: this is experimental. Still a lot to be done.**

-----

# 1. Build pre-requisites #

To build our RAN images, we SHALL use the `codeready-builder-for-rhel-8-x86_64-rpms` repository with all the proper development libraries.

This repository is not directly accessible from the UBI RHEL8 image (`registry.access.redhat.com/ubi8/ubi:latest`).

So we need to copy from a register RHEL8 machine certificates and subsccription manager configuration files.

SO on a `RHEL8` physical machine (or a virtual machine) connected to the OpenShift Cluster, recover the entitlement and the RH subscription manager configs:

```bash
oc create configmap rhsm-conf --from-file /etc/rhsm/rhsm.conf
oc create configmap rhsm-ca --from-file /etc/rhsm/ca/redhat-uep.pem

oc create secret generic etc-pki-entitlement --from-file /etc/pki/entitlement/{NUMBER_ON_YOUR_COMPUTER}.pem --from-file /etc/pki/entitlement/{NUMBER_ON_YOUR_COMPUTER}-key.pem
```

These configmaps and secret will be shared by all the build configs in your OC project. No need to do it each time.

**CAUTION: these files expire every month or so. If you have done a build on your OC project and try again a few weeks later, you may need to re-copy them**.

```bash
oc delete secret etc-pki-entitlement
oc delete cm rhsm-conf
oc delete cm rhsm-ca
```

**LAST POINT: your OC project SHALL be `oai`.**

# 2. Build the Builder shared image #

In our Eurecom/OSA environment we need to pass a GIT proxy.

2 things are impacted by this situation:

*  In `openshift/oai-ran-rh8-build-config.yml` file
   * `httpProxy: http://proxy.eurecom.fr:8080` 
   * `httpsProxy: https://proxy.eurecom.fr:8080`
*  Add a environment variable to the build config

```bash
oc apply -f openshift/oai-ran-rh8-image-stream.yml
oc apply -f openshift/oai-ran-rh8-build-config.yml
oc set env bc/oai-ran-rhel8-build-config NEEDED_GIT_PROXY=http://proxy.eurecom.fr:8080
oc start-build oai-ran-rhel8-build-config --follow
```

In case you do NOT require a GIT proxy: **you SHALL remove the 2 lines in `openshift/oai-ran-rh8-build-config.yml` file.**

And no need to add a `NEEDED_GIT_PROXY` variable to the build config.

```bash
oc apply -f openshift/oai-ran-rh8-image-stream.yml
oc apply -f openshift/modified-oai-ran-rh8-build-config.yml
oc start-build oai-ran-rhel8-build-config --follow
```

After a while it should be successful.

# 3. Build an OAI target image #

For the example the eNB:

```bash
oc apply -f openshift/oai-enb-rh8-image-stream.yml
oc apply -f openshift/oai-enb-rh8-build-config.yml
oc start-build oai-enb-rh8-build-config --follow
```

**CAUTION: if you are pushing modifications to the branch you are using, it won't be taken into account besides the Dockerfile.**

**Because the source files are copied during the shared image creation.**

**Only way to regenerate images w/ modified source code is to re-start from step #2.**

# 4. Deployment using HELM charts #

**CAUTION: even more experimental.**

Helm charts are located in another repository:

```bash
git clone https://github.com/OPENAIRINTERFACE/openair-k8s.git
cd openair-k8s
git checkout helm-deployment-S6a-S1C-S1U-in-network-18-with-enb
helm install mme /path-to-your-cloned/openair-k8s/charts/oai-mme/
```
