#!/bin/bash

set -euo pipefail

# Based another env var
if [[ -v USE_FDD_CU ]]; then ln -s /opt/oai-enb/etc/cu.fdd.conf /opt/oai-enb/etc/enb.conf; fi
if [[ -v USE_FDD_DU ]]; then ln -s /opt/oai-enb/etc/du.fdd.conf /opt/oai-enb/etc/enb.conf; fi
if [[ -v USE_FDD_MONO ]]; then ln -s /opt/oai-enb/etc/enb.fdd.conf /opt/oai-enb/etc/enb.conf; fi
if [[ -v USE_TDD_MONO ]]; then ln -s /opt/oai-enb/etc/enb.tdd.conf /opt/oai-enb/etc/enb.conf; fi
if [[ -v USE_FDD_RCC ]]; then ln -s /opt/oai-enb/etc/rcc.if4p5.enb.fdd.conf /opt/oai-enb/etc/enb.conf; fi
if [[ -v USE_FDD_RRU ]]; then ln -s /opt/oai-enb/etc/rru.fdd.conf /opt/oai-enb/etc/enb.conf; fi
if [[ -v USE_TDD_RRU ]]; then ln -s /opt/oai-enb/etc/rru.tdd.conf /opt/oai-enb/etc/enb.conf; fi

CONFIG_FILES=`ls /opt/oai-enb/etc/enb.conf`

for c in ${CONFIG_FILES}; do
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

# Later-on, how to add command lines options?

# Load the USRP binaries
# --> later conditional (when doing simulators)
/usr/lib/uhd/utils/uhd_images_downloader.py

exec "$@"
