#!/bin/bash

set -uo pipefail

PREFIX=/opt/oai-gnb-aw2s

if [[ -v USE_VOLUMED_CONF ]]; then
  cp $PREFIX/etc/mounted.conf $PREFIX/etc/gnb.conf
else
  echo "ERROR: No configuration file provided."
  echo "Please set USE_VOLUMED_CONF and mount a configuration file at $PREFIX/etc/mounted.conf"
  exit 1
fi

echo "=================================="
echo "== Configuration file:"
cat $PREFIX/etc/enb.conf

# enable printing of stack traces on assert
export gdbStacks=1

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
