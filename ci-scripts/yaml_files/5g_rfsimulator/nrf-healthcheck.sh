#!/bin/bash
set -eo pipefail

STATUS=0
NRF_IP_SBI_INTERFACE=$(ifconfig $NRF_INTERFACE_NAME_FOR_SBI | grep inet | awk {'print $2'})
NRF_SBI_PORT_STATUS=$(netstat -tnpl | grep -o "$NRF_IP_SBI_INTERFACE:$NRF_INTERFACE_PORT_FOR_SBI")
#Check if entrypoint properly configured the conf file and no parameter is unset(optional)
NB_UNREPLACED_AT=`cat /openair-nrf/etc/*.conf | grep -v contact@openairinterface.org | grep -c @ || true`

if [ $NB_UNREPLACED_AT -ne 0 ]; then
	STATUS=1
	echo "Healthcheck error: UNHEALTHY configuration file is not configured properly"
fi

if [[ -z $NRF_SBI_PORT_STATUS ]]; then
	STATUS=1
	echo "Healthcheck error: UNHEALTHY SBI TCP/HTTP port $NRF_INTERFACE_PORT_FOR_SBI is not listening."
fi

exit $STATUS