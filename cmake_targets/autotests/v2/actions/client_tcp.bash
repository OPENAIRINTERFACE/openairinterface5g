echo $SERVER_IP
timeout -s 9 20s iperf -c $SERVER_IP -i1
