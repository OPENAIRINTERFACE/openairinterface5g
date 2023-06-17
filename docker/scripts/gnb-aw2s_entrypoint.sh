#!/bin/bash

set -uo pipefail

PREFIX=/opt/oai-gnb-aw2s
CONFIGFILE=$PREFIX/etc/gnb.conf

if [ ! -f $CONFIGFILE ]; then
  echo "No configuration file found: please mount at $CONFIGFILE"
  exit 255
fi

echo "=================================="
echo "== Configuration file:"
cat $CONFIGFILE

# enable printing of stack traces on assert
export OAI_GDBSTACKS=1

echo "=================================="
echo "== Starting gNB soft modem with AW2S"
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
