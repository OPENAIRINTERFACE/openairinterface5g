#!/bin/sh
oc apply -f openshift/oai-gnb-image-stream.yml
oc apply -f openshift/oai-gnb-build-config.yml
oc set env bc/oai-gnb-build-config NEEDED_GIT_PROXY=http://proxy.eurecom.fr:8080
oc start-build oai-gnb-build-config --follow
