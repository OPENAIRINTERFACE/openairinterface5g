#!/bin/bash

ue_id="$1"      # UE id
duration="$2"   # Sim duration

shift
shift

function end
{
    exit 0
}
trap end INT TERM

echo "ip netns exec $ue_id $@ > /tmp/test_${ue_id}.log"
ip netns exec $ue_id $@ > /tmp/test_$ue_id.log


