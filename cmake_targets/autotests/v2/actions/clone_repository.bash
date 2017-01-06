sudo rm -rf /tmp/oai_test_setup
mkdir /tmp/oai_test_setup
cd /tmp/oai_test_setup
git clone $REPOSITORY_URL oai
cd oai
git checkout $COMMIT_ID
