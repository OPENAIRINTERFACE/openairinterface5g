sudo rmmod nasmesh || true
sudo rmmod ue_ip || true
sudo /opt/ltebox/tools/stop_ltebox || true
sudo killall -9 hss_sim || true
sudo /opt/hss_sim0609/starthss_real
