#!/bin/bash

set -euo pipefail

# First see if all interfaces are up
ifconfig

# S10 might be on loopback --> needs bring-up
if [[ "$MME_INTERFACE_NAME_FOR_S10" == *"lo:"* ]]
then
    ifconfig ${MME_INTERFACE_NAME_FOR_S10} ${MME_IPV4_ADDRESS_FOR_S10} up
fi

LIST_OF_NETWORKS=`ifconfig -s | egrep -v "^Iface|^lo" | cut -d' ' -f1`

for if_name in $LIST_OF_NETWORKS
do
    IF_IP_ADDR=`ifconfig $if_name | grep inet | sed -e "s# *inet#inet#" | cut -d' ' -f2`
    if [[ "${IF_IP_ADDR}" == "${MME_IPV4_ADDRESS_FOR_S1_MME}" ]]; then
        echo "S1C is on $if_name"
	MME_INTERFACE_NAME_FOR_S1_MME=$if_name
    fi
    if [[ "${IF_IP_ADDR}" == "${MME_IPV4_ADDRESS_FOR_S10}" ]]; then
        echo "S10 is on $if_name"
	MME_INTERFACE_NAME_FOR_S10=$if_name
    fi
    if [[ "${IF_IP_ADDR}" == "${MME_IPV4_ADDRESS_FOR_S11}" ]]; then
        echo "S11 is on $if_name"
	MME_INTERFACE_NAME_FOR_S11=$if_name
    fi
done

CONFIG_DIR="/openair-mme/etc"

for c in ${CONFIG_DIR}/mme_fd.conf; do
    #echo "entrypoint.sh process config file $c"
    sed -i -e "s#@TAC-LB#@TAC_LB#" -e "s#TAC-HB_#TAC_HB_#" ${c}
    # grep variable names (format: ${VAR}) from template to be rendered
    VARS=$(grep -oP '@[a-zA-Z0-9_]+@' ${c} | sort | uniq | xargs)
    #echo "entrypoint.sh process vars $VARS"

    # create sed expressions for substituting each occurrence of ${VAR}
    # with the value of the environment variable "VAR"
    EXPRESSIONS=""
    for v in ${VARS}; do
        #echo "var is $v"
        NEW_VAR=`echo $v | sed -e "s#@##g"`
        #echo "NEW_VAR is $NEW_VAR"
        if [[ "${!NEW_VAR}x" == "x" ]]; then
            echo "Error: Environment variable '${NEW_VAR}' is not set." \
                "Config file '$(basename $c)' requires all of $VARS."
            exit 1
        fi
        # Some fields require CIDR format
        if [[ "${NEW_VAR}" == "MME_IPV4_ADDRESS_FOR_S1_MME" ]] || \
           [[ "${NEW_VAR}" == "MME_IPV4_ADDRESS_FOR_S11" ]] || \
           [[ "${NEW_VAR}" == "MME_IPV4_ADDRESS_FOR_S10" ]]; then
            EXPRESSIONS="${EXPRESSIONS};s|${v}|${!NEW_VAR}/24|g"
        else
            EXPRESSIONS="${EXPRESSIONS};s|${v}|${!NEW_VAR}|g"
        fi
    done
    EXPRESSIONS="${EXPRESSIONS#';'}"

    # render template and inline replace config file
    sed -i "${EXPRESSIONS}" ${c}
done

pushd /openair-mme/scripts
./check_mme_s6a_certificate ${PREFIX} ${MME_FQDN}
popd

exec "$@"
