cd /tmp/oai_test_setup/oai
source oaienv
cd cmake_targets
rm -rf log
mkdir -p log
echo $BUILD_ARGUMENTS
./build_oai $BUILD_ARGUMENTS
echo $BUILD_OUTPUT
ls $BUILD_OUTPUT
