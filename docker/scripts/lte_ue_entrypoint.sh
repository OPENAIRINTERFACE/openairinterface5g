#!/bin/bash

set -uo pipefail

PREFIX=/opt/oai-lte-ue
USIM_CONFIGFILE=$PREFIX/etc/ue_usim.conf

if [ ! -f $USIM_CONFIGFILE ]; then
  echo "No ue_usim.conf configuration file found: please mount at $USIM_CONFIGFILE"
  exit 255
fi

echo "=================================="
echo "== USIM Configuration file:"
cat $USIM_CONFIGFILE

#now generate USIM files
# At this point all operations will be run from $PREFIX!
cd $PREFIX
$PREFIX/bin/conf2uedata -c $USIM_CONFIGFILE -o $PREFIX

CONFIGFILE=$PREFIX/etc/ue.conf

if [ ! -f $CONFIGFILE ]; then
  echo "No ue.conf configuration file found"
else
  echo "=================================="
  echo "== UE Configuration file:"
  cat $CONFIGFILE
fi

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
if [[ -f "$CONFIGFILE"  ]]; then
  new_args+=("-O")
  new_args+=("$CONFIGFILE")
fi

# enable printing of stack traces on assert
export OAI_GDBSTACKS=1

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
