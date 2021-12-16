#!/bin/bash

set -euo pipefail

PREFIX=/opt/oai-gnb
ENABLE_X2=${ENABLE_X2:-yes}
THREAD_PARALLEL_CONFIG=${THREAD_PARALLEL_CONFIG:-PARALLEL_SINGLE_THREAD}

# Based another env var, pick one template to use
if [[ -v USE_NSA_TDD_MONO ]]; then cp $PREFIX/etc/gnb.nsa.tdd.conf $PREFIX/etc/gnb.conf; fi
if [[ -v USE_SA_TDD_MONO ]]; then cp $PREFIX/etc/gnb.sa.tdd.conf $PREFIX/etc/gnb.conf; fi
# Sometimes, the templates are not enough. We mount a conf file on $PREFIX/etc. It can be a template itself.
if [[ -v USE_VOLUMED_CONF ]]; then cp $PREFIX/etc/mounted.conf $PREFIX/etc/gnb.conf; fi

# Only this template will be manipulated
CONFIG_FILES=`ls $PREFIX/etc/gnb.conf || true`

for c in ${CONFIG_FILES}; do
    # Sometimes templates have no pattern to be replaced.
    if ! grep -oP '@[a-zA-Z0-9_]+@' ${c}; then
        echo "Configuration is already set"
        break
    fi

    # grep variable names (format: ${VAR}) from template to be rendered
    VARS=$(grep -oP '@[a-zA-Z0-9_]+@' ${c} | sort | uniq | xargs)

    # create sed expressions for substituting each occurrence of ${VAR}
    # with the value of the environment variable "VAR"
    EXPRESSIONS=""
    for v in ${VARS}; do
        NEW_VAR=`echo $v | sed -e "s#@##g"`
        if [[ "${!NEW_VAR}x" == "x" ]]; then
            echo "Error: Environment variable '${NEW_VAR}' is not set." \
                "Config file '$(basename $c)' requires all of $VARS."
            exit 1
        fi
        EXPRESSIONS="${EXPRESSIONS};s|${v}|${!NEW_VAR}|g"
    done
    EXPRESSIONS="${EXPRESSIONS#';'}"

    # render template and inline replace config file
    sed -i "${EXPRESSIONS}" ${c}
done

# Load the USRP binaries
if [[ -v USE_B2XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t b2xx
elif [[ -v USE_X3XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t x3xx
elif [[ -v USE_N3XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t n3xx
fi

echo "=================================="
echo "== Starting gNB soft modem"
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
