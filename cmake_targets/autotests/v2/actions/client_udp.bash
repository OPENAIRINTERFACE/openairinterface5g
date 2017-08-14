echo $SERVER_IP
echo $UDP_BANDWIDTH
timeout -s 9 20s iperf -c $SERVER_IP -i1 -u -b $UDP_BANDWIDTH
