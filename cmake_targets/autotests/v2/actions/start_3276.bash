#enable control+C reception (to be refined if it does not work)
stty isig intr ^C

cd /tmp/oai_test_setup/oai
source oaienv
sudo rmmod nasmesh || true
sudo rmmod ue_ip || true
cd cmake_targets/autotests/v2/actions
sudo python start_3276.py

sudo wvdial -C wvdial.3276.conf || true

sudo python stop_3276.py
