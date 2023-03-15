#!/bin/bash

set -uo pipefail

PREFIX=/opt/oai-lte-ue
CONFIGFILE=$PREFIX/etc/ue_usim.conf

if [ ! -f $CONFIGFILE ]; then
  echo "No configuration file found: please mount at $CONFIGFILE"
  exit 255
fi

echo "=================================="
echo "== Configuration file:"
cat $CONFIGFILE

#now generate USIM files
# At this point all operations will be run from $PREFIX!
cd $PREFIX
$PREFIX/bin/conf2uedata -c $CONFIGFILE -o $PREFIX

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

# in case we have conf file, append
new_args=()
while [[ $# -gt 0 ]]; do
  new_args+=("$1")
  shift
done
FILE=$PREFIX/etc/ue.conf
if [[ -f "$FILE"  ]]; then
  new_args+=("-O")
  new_args+=("$PREFIX/etc/ue.conf")
fi

# enable printing of stack traces on assert
export gdbStacks=1

echo "=================================="
echo "== Starting LTE UE soft modem"
if [[ -v USE_ADDITIONAL_OPTIONS ]]; then
    echo "Additional option(s): ${USE_ADDITIONAL_OPTIONS}"
    for word in ${USE_ADDITIONAL_OPTIONS}; do
        new_args+=("$word")
    done
    echo "${new_args[@]}"
    exec "${new_args[@]}"
else
    echo "${new_args[@]}"
    exec "${new_args[@]}"
fi
