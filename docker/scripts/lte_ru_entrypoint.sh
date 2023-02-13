#!/bin/bash

set -uo pipefail

PREFIX=/opt/oai-lte-ru
CONFIGFILE=$PREFIX/etc/rru.conf

# Based another env var, pick one template to use
if [[ -v USE_VOLUMED_CONF ]]; then
  cp $PREFIX/etc/rru.fdd.conf $PREFIX/etc/rru.conf
else
  echo "ERROR: No configuration file provided."
  echo "Please set USE_VOLUMED_CONF and mount a configuration file at $PREFIX/etc/mounted.conf"
  exit 1
fi

if [ ! -f $CONFIGFILE ]; then
  echo "No configuration file found: please mount at $CONFIGFILE"
  exit 255
fi

echo "=================================="
echo "== Configuration file:"
cat $CONFIGFILE

# Load the USRP binaries
echo "=================================="
echo "== Load USRP binaries"
if [[ -v USE_B2XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t b2xx
elif [[ -v USE_X3XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t x3xx
elif [[ -v USE_N3XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t n3xx
fi

# enable printing of stack traces on assert
export gdbStacks=1

echo "=================================="
echo "== Starting eNB soft modem"
if [[ -v USE_ADDITIONAL_OPTIONS ]]; then
    echo "Additional option(s): ${USE_ADDITIONAL_OPTIONS}"
    new_args=()
    while [[ $# -gt 0 ]]; do
        new_args+=("$1")
        shift
    done
    for word in ${USE_ADDITIONAL_OPTIONS}; do
        new_args+=("$word")
    done
    echo "${new_args[@]}"
    exec "${new_args[@]}"
else
    echo "$@"
    exec "$@"
fi
