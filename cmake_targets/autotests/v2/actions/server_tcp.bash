stty isig intr ^C

#timeout -s 9 20s iperf -s -i1
iperf -s -i1
