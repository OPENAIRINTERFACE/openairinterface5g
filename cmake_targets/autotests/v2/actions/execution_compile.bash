cd /tmp/oai_test_setup/oai
source oaienv
cd cmake_targets
rm -rf log
mkdir -p log
bash -c "$PRE_BUILD"
$BUILD_PROG $BUILD_ARGUMENTS
bash -c "$PRE_EXEC"
