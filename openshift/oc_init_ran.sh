#!/bin/sh
oc apply -f openshift/oai-ran-image-stream.yml
oc apply -f openshift/oai-ran-build-config.yml
oc set env bc/oai-ran-build-config NEEDED_GIT_PROXY=http://proxy.eurecom.fr:8080
oc start-build oai-ran-build-config --follow
