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

# an existing oai-ci/<image>:<branch> is used as is, and not overwritten
for image in oai-nr-ue oai-gnb oai-nr-cuup; do
  if ! docker image inspect oai-ci/${image}:${CURRENT_BRANCH} > /dev/null 2>&1; then
    docker tag ${image} oai-ci/${image}:${CURRENT_BRANCH}
  fi
done

TESTCASES="$@"

python3 main.py --repository=NONE --branch=${CURRENT_BRANCH} \
    --targetBranch=NONE \
    --workspace=${REPO_PATH} \
	--local --datefmt="%H:%M:%S" \
    ${TESTCASES}

exit $?
