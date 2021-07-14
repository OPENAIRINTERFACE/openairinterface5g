#enable control+C reception (to be refined if it does not work)
stty isig intr ^C

#If /dev/bandrich is not present when we start the test, what to do?
#The following commented lines are an attempt at solving this.
#This is not satisfying, so we rather do nothing.

##the UE got stuck once, I had to run usb_modeswitch by hand.
##So at this point, if /dev/bandrich does not exist, maybe the UE is
##stuck for whatever reason. Let's forcefully run usb_modeswitch.
#if [ ! -e /dev/bandrich ]; then sudo usb_modeswitch -v 1a8d -p 1000 -I -W -K; true; fi
#
##wait for /dev/bandrich (TODO: use inotify?)
##may fail if the bandrich is in a bad state
#while [ ! -e /dev/bandrich ]; do sleep 1; done

cd /tmp/oai_test_setup/oai
source oaienv
sudo rmmod nasmesh || true
sudo rmmod ue_ip || true
cd cmake_targets/autotests/v2/actions
python start_bandrich.py

sudo wvdial -C wvdial.bandrich.conf || true

python stop_bandrich.py
