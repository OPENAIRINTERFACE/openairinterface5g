cd /tmp/oai_test_setup/oai
source oaienv
sudo rmmod nasmesh || true
sudo rmmod ue_ip || true
cd cmake_targets
rm -rf log
mkdir -p log
echo $PRE_BUILD
bash -c "$PRE_BUILD"
echo $BUILD_PROG $BUILD_ARGUMENTS
$BUILD_PROG $BUILD_ARGUMENTS
echo $PRE_EXEC
bash -c "$PRE_EXEC"
