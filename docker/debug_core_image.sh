#!/bin/bash

if [ $# -ne 3 ]; then
  echo "usage: $0 <image> <coredump> <path-to-sources>"
  exit 1
fi

die() {
  echo $1
  exit 1
}

IMAGE=$1
COREDUMP=$2
SOURCES=$3

set -x

# the image/build_oai builds in cmake_targets/ran_build/build, so source
# information is relative to this path. In case the user did not compile on
# their computer, this directory will not exist. still allow to find it by
# creating it
BUILD_DIR=$SOURCES/cmake_targets/ran_build/build
mkdir -p $BUILD_DIR || die "cannot create $BUILD_DIR: is $SOURCES valid?"

# check if coredump is valid file
[ -f $COREDUMP ] || die "no such file: $COREDUMP"

# check if image exists, and determine type (gnb, nr-ue) for correct invocation
# of gdb
docker image inspect $IMAGE > /dev/null || exit 1
if [ $(grep "oai-gnb:" <<< $IMAGE) ] || [ $(grep "oai-gnb-aerial:" <<< $IMAGE) ]; then
  EXEC=bin/nr-softmodem
  TYPEPATH=oai-gnb
elif [ $(grep "oai-nr-ue:" <<< $IMAGE) ]; then
  EXEC=bin/nr-uesoftmodem
  TYPEPATH=oai-nr-ue
elif [ $(grep "oai-enb:" <<< $IMAGE) ]; then
  EXEC=bin/lte-softmodem
  TYPEPATH=oai-enb
elif [ $(grep "oai-lte-ue:" <<< $IMAGE) ]; then
  EXEC=bin/lte-uesoftmodem
  TYPEPATH=oai-lte-ue
else
  die "cannot determine if image is gnb or nr-ue: must match \"oai-gnb:\" or \"oai-nr-ue:\""
fi

# run gdb inside a container. We mount the coredump and the sources inside the
# container, and run gdb with the core dump, the correct executable, and using
# the source directory to show correct line numbers
docker run --rm -it \
  -v $COREDUMP:/tmp/coredump \
  -v $SOURCES:/opt/$TYPEPATH/src \
  --entrypoint bash \
  $IMAGE \
  -c "gdb --dir=src/cmake_targets/ran_build/build $EXEC /tmp/coredump"
