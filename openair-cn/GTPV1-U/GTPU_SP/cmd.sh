# Clear the iptables mangle table
iptables -t mangle -F

# Remove GTPU KLM
rmmod xt_GTPUSP

# Insert the GTPUSP KLM
insmod ./Bin/xt_GTPUSP.ko

# Copy the userland iptables extenstion library
if [ -d /lib/xtables ]; then
    /bin/cp -f ./Bin/libxt_GTPUSP.so /lib/xtables/
fi

if [ -d /lib/iptables ]; then
    /bin/cp -f ./Bin/libxt_GTPUSP.so /lib/iptables/libipt_GTPUSP.so
fi

