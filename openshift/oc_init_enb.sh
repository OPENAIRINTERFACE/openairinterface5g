#!/bin/sh
oc apply -f openshift/oai-enb-image-stream.yml
oc apply -f openshift/oai-enb-build-config.yml
oc set env bc/oai-enb-build-config NEEDED_GIT_PROXY=http://proxy.eurecom.fr:8080
oc start-build oai-enb-build-config --follow
