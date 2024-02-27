#!/bin/bash

# Modified by Karim Boutiba (karim.boutiba@eurecom.fr)
# mbim-set-ip script version 1.1
# Modified mbimcli IPv4 IPv6 parsing script based on parts of project https://github.com/grandcentrix/thinkpad-x260-modem-scripts
# Licensed under the Apache License, Version 2.0
# Modified by Jörgen Storvist, Techship http://www.techship.com
# Further details in the Techship FAQ sections on using MBIM and setting correct modes in cellular modules:
# https://techship.com/faq/how-to-set-up-a-simple-data-connection-over-the-mbim-interface-using-libmbim-and-driver-cdc-mbim-in-linux/
#
# Installation & Usage: 
#
# Verify that the cellular module is exposing the MBIM interface over the USB interface. 
# Use mbimcli to check status for the cellular connection and enter the PIN code if necessary. 
# Configure mbim-network.conf and place in /etc/ (example file included in the archive, check link bellow for details).
# https://www.freedesktop.org/software/libmbim/man/latest/mbim-network.1.html
# Start the mbim data connection with command:
# mbim-network <MBIM-INTERFACE> start
# Ensure execution of mbim-set-ip script with sufficient system priviledges
# ./mbim-set-ip <MBIM-INTERFACE> <NETWORK-INTERFACE>
#
# Examples:
# mbimcli -d /dev/cdc-wdm0 -p --query-device-caps
# mbimcli -d /dev/cdc-wdm0 -p --enter-pin=1234
# mbim-network /dev/cdc-wdm0 start
# ./mbim-set-ip /dev/cdc-wdm0 wwan0
#
# You should now be able to ping over the data connection.
# IP v4 ping: (only if IPv4 address was acquired from cellular module)
# ping -4 -I wwan0 8.8.8.8
# ping -4 -I wwan0 google.com

# IP v6 ping: (only if IPv6 address was acquired from cellular module)
# ping -6 -I wwan0 2001:4860:4860::8888
# ping -6 -I wwan0 google.com
#
# The script relies on the linux tools mbimcli, ip, systemd-resolve
# (Tested on libmbim 1.16.0 running on Ubuntu 18.10 cosmic (development) with kernel version 4.15.0)

# Details on mbim-network and mbimcli
# https://www.freedesktop.org/software/libmbim/man/latest/mbim-network.1.html
# https://www.freedesktop.org/software/libmbim/man/latest/mbimcli.1.html

ipv4_addresses=()
ipv4_gateway=""
ipv4_dns=()
ipv4_mtu=""
ipv6_addresses=()
ipv6_gateway=""
ipv6_dns=()
ipv6_mtu=""

#CONTROL-IFACE
CONTROLDEV="$1" 
#WWAN-IFACE
DEV="$2" 
SESSION="$3"
echo $SESSION
echo "Requesting IPv4 and IPv6 information through mbimcli proxy:"
mbimcli -d $CONTROLDEV -p --query-ip-configuration
IPDATA=$(mbimcli -d $CONTROLDEV -p --query-ip-configuration=$SESSION)

function parse_ip {
	#      IP [0]: '10.134.203.177/30'
	local line_re="IP \[([0-9]+)\]: '(.+)'"
	local input=$1
	if [[ $input =~ $line_re ]]; then
		local ip_cnt=${BASH_REMATCH[1]}
		local ip=${BASH_REMATCH[2]}
	fi
	echo "$ip"
}

function parse_gateway {
	#    Gateway: '10.134.203.178'
	local line_re="Gateway: '(.+)'"
	local input=$1
	if [[ $input =~ $line_re ]]; then
		local gw=${BASH_REMATCH[1]}
	fi
	echo "$gw"
}

function parse_dns {
	#      DNS [0]: '10.134.203.177/30'
	local line_re="DNS \[([0-9]+)\]: '(.+)'"
	local input=$1
	if [[ $input =~ $line_re ]]; then
		local dns_cnt=${BASH_REMATCH[1]}
		local dns=${BASH_REMATCH[2]}
	fi
	echo "$dns"
}

function parse_mtu {
	#        MTU: '1500'
	local line_re="MTU: '([0-9]+)'"
	local input=$1
	if [[ $input =~ $line_re ]]; then
		local mtu=${BASH_REMATCH[1]}
	fi
	echo "$mtu"
}
while read -r line || [[ -n "$line" ]] ; do
	[ -z "$line" ] && continue
	case "$line" in
		*"IPv4 configuration available: 'none'"*)
		       	state="start"
		        continue
			;;
		*"IPv4 configuration available"*)
			state="ipv4"
			continue
			;;
		*"IPv6 configuration available: 'none'"*)
		        state="start"
		        continue
			;;
		*"IPv6 configuration available"*)
			state="ipv6"
			continue
			;;
		*)
			;;
	esac
	case "$state" in
		"ipv4")

			case "$line" in
			*"IP"*)
				row=$(parse_ip "$line")
				ipv4_addresses+=("$row")
			        continue
				;;
			*"Gateway"*)
				row=$(parse_gateway "$line")
				ipv4_gateway="$row"
				continue
				;;
			*"DNS"*)
				row=$(parse_dns "$line")
				ipv4_dns+=("$row")
				continue
				;;
			*"MTU"*)
				row=$(parse_mtu "$line")
				ipv4_mtu="$row"
				continue
				;;
			*)
				;;
			esac
			;;

		"ipv6")
			case "$line" in
			*"IP"*)
				row=$(parse_ip "$line")
				ipv6_addresses+=("$row")
			        continue
				;;
			*"Gateway"*)
				row=$(parse_gateway "$line")
				ipv6_gateway="$row"
				continue
				;;
			*"DNS"*)
				row=$(parse_dns "$line")
				ipv6_dns+=("$row")
				continue
				;;
			*"MTU"*)
				row=$(parse_mtu "$line")
				ipv6_mtu="$row"
				continue
				;;
			*)
				continue
				;;
			esac
		;;
	*)
		continue
	;;
	esac
done <<< "$IPDATA"

execfile=$(mktemp)

printf "ip link set $DEV down\n" >> $execfile
printf "ip addr flush dev $DEV \n" >> $execfile
printf "ip -6 addr flush dev $DEV \n" >> $execfile
printf "ip link set $DEV up\n" >> $execfile

if [[ "${#ipv4_addresses[@]}" > 0 ]]; then
	printf "ip addr add %s dev $DEV broadcast +\n" "${ipv4_addresses[@]}" >> $execfile
	printf "ip route add 192.168.61.0/24 via $ipv4_gateway dev $DEV\n" >> $execfile

	if [ -n "$ipv4_mtu" ]; then
		printf "ip link set mtu $ipv4_mtu dev $DEV \n" >> $execfile
	fi
	if [[ "${#ipv4_dns[@]}" > 0 ]]; then
		printf "resolvectl -4 --interface=$DEV --set-dns=%s\n" "${ipv4_dns[@]}" >>$execfile
	fi
fi

if [[ "${#ipv6_addresses[@]}" > 0 ]]; then
	printf "ip -6 addr add %s dev $DEV\n" "${ipv6_addresses[@]}" >> $execfile
	printf "ip -6 route add default via $ipv6_gateway dev $DEV\n" >> $execfile
	if [ -n "$ipv6_mtu" ]; then
		printf "ip -6 link set mtu $ipv6_mtu dev $DEV\n" >> $execfile
	fi
	if [[ "${#ipv6_dns[@]}" > 0 ]]; then
		printf "systemd-resolve -6 --interface=$DEV --set-dns=%s\n" "${ipv6_dns[@]}" >>$execfile
	fi
fi
echo "Applying the following network interface configurations:"
cat $execfile
bash $execfile
rm $execfile
echo "Network interface configurations completed."
