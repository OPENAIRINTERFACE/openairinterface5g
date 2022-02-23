#!/bin/bash
set -eo pipefail

STATUS=0
AMF_PORT_FOR_NGAP=38412
AMF_PORT_FOR_N11_HTTP=80
AMF_IP_NGAP_INTERFACE=$(ifconfig $AMF_INTERFACE_NAME_FOR_NGAP | grep inet | awk {'print $2'})
AMF_IP_N11_INTERFACE=$(ifconfig $AMF_INTERFACE_NAME_FOR_N11 | grep inet | awk {'print $2'})
N2_PORT_STATUS=$(netstat -Snpl | grep -o "$AMF_IP_NGAP_INTERFACE:$AMF_PORT_FOR_NGAP")
N11_PORT_STATUS=$(netstat -tnpl | grep -o "$AMF_IP_N11_INTERFACE:$AMF_PORT_FOR_N11_HTTP")
#Check if entrypoint properly configured the conf file and no parameter is unset (optional)
NB_UNREPLACED_AT=`cat /openair-amf/etc/*.conf | grep -v contact@openairinterface.org | grep -c @ || true`

if [ $NB_UNREPLACED_AT -ne 0 ]; then
	STATUS=1
	echo "Healthcheck error: configuration file is not configured properly"
fi

if [[ -z $N2_PORT_STATUS ]]; then
	STATUS=1
	echo "Healthcheck error: N2 SCTP port $AMF_PORT_FOR_NGAP is not listening"
fi

if [[ -z $N11_PORT_STATUS ]]; then
	STATUS=1
	echo "Healthcheck error: N11/SBI TCP/HTTP port $AMF_PORT_FOR_N11_HTTP is not listening"
fi

#host="${MYSQL_SERVER}"
#user="${MYSQL_USER:-root}"
#export MYSQL_PWD="${MYSQL_PASS}"

#args=(
#	-h"$host"
#	-u"$user"
#	--silent
#)

#if ! command -v mysql &> /dev/null; then
#	echo "Installing mysql command"
#	apt update
#	apt-get -y install mysql-client
#else
#	if select="$(echo 'SELECT 1' | mysql "${args[@]}")" && [ "$select" = '1' ]; then
#		database_check=$(mysql -h$host -u$user -D oai_db --silent -e "SELECT * FROM users;")
#		if [[ -z $database_check ]]; then
#			echo "Healthcheck error: oai_db not populated"
#			STATUS=1
#		fi
#		STATUS=0
#	else
#		echo "Healthcheck error: Mysql port inactive"
#		STATUS=1
#	fi
#fi

exit $STATUS
