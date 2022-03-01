#!/bin/bash
set -eo pipefail

STATUS=0
SMF_IP_SBI_INTERFACE=$(ifconfig $SMF_INTERFACE_NAME_FOR_SBI | grep inet | awk {'print $2'})
SMF_SBI_PORT_STATUS=$(netstat -tnpl | grep -o "$SMF_IP_SBI_INTERFACE:$SMF_INTERFACE_PORT_FOR_SBI")

#Check if entrypoint properly configured the conf file and no parameter is unset(optional)
#NB_UNREPLACED_AT=`cat /openair-smf/etc/*.conf | grep -v contact@openairinterface.org | grep -c @ || true`

#if [ $NB_UNREPLACED_AT -ne 0 ]; then
#	STATUS=-1
#	echo "Healthcheck error: UNHEALTHY configuration file is not configured properly"
#fi
#
if [[ -z $SMF_SBI_PORT_STATUS ]]; then
	STATUS=-1
	echo "Healthcheck error: UNHEALTHY SBI TCP/HTTP port $SMF_INTERFACE_PORT_FOR_SBI is not listening."
fi

exit $STATUS
