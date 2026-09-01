#!/bin/bash
# SPDX-License-Identifier: MIT

SHORT_COMMIT_SHA=$(git rev-parse --short=8 HEAD)
COMMIT_SHA=$(git rev-parse HEAD)
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
REPO_PATH=$(dirname $(realpath $0))/../

if [ $# -eq 0 ]
  then
    echo "Provide the path to a testcase XML (relative to ci-scripts/, e.g. xml_files/container_5g_rfsim.xml) as an argument"
    exit 1
fi

set -x

# The script assumes you've build the following images:
#
# docker build . -f docker/Dockerfile.gNB.ubuntu -t oai-gnb
# docker build . -f docker/Dockerfile.nr-cuup.ubuntu -t oai-nr-cuup
# docker build . -f docker/Dockerfile.nrUE.ubuntu -t oai-nr-ue
#
# The images above depend on the following images:
#
# docker build . -f docker/Dockerfile.build.ubuntu -t ran-build
# docker build . -f docker/Dockerfile.base.ubuntu -t ran-base

docker tag oai-nr-ue oai-ci/oai-nr-ue:${CURRENT_BRANCH}
docker tag oai-gnb oai-ci/oai-gnb:${CURRENT_BRANCH}
docker tag oai-nr-cuup oai-ci/oai-nr-cuup:${CURRENT_BRANCH}

TESTCASES="$@"

python3 main.py --repository=NONE --branch=${CURRENT_BRANCH} \
    --ranAllowMerge=false \
    --targetBranch=NONE \
    --workspace=${REPO_PATH} \
	--local --datefmt="%H:%M:%S" \
    ${TESTCASES}

exit $?
