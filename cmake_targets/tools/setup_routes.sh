#!/bin/bash

for i in $(seq 1 $1);
do
    let table=1000+$i
    echo "sudo ip route add 10.0.1.0/24 dev oaitun_ue$i table $table"
    sudo ip route add 10.0.1.0/24 dev oaitun_ue$i table $table
    echo "sudo ip route add default via 10.0.1.1 dev oaitun_ue$i table $table"
    sudo ip route add default via 10.0.1.1 dev oaitun_ue$i table $table
    let octet=$i+1
    echo "sudo ip rule add from 10.0.1.$octet table $table"
    sudo ip rule add from 10.0.1.$octet table $table
done
