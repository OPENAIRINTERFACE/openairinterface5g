#enable control+C reception (to be refined if it does not work)
stty isig intr ^C

cd /tmp/oai_test_setup/oai
source oaienv
cd cmake_targets/lte_build_oai/build
ulimit -c unlimited
sudo rm -f core
#sudo -E ./lte-softmodem -O $OPENAIR_DIR/cmake_targets/autotests/v2/config/enb.band7.tm1.usrpb210.conf
sudo -E ./lte-softmodem -O /tmp/enb.conf
