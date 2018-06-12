#!/bin/bash

if [ $# -ne 4 ]
then
    echo "Syntax Error: $0 src-branch src-commit-id dest-branch dest-commit-id"
    exit 1
fi

SOURCE_BRANCH=$1
echo "Source Branch is    : $SOURCE_BRANCH"

SOURCE_COMMIT_ID=$2
echo "Source Commit ID is : $SOURCE_COMMIT_ID"

TARGET_BRANCH=$3
echo "Target Branch is    : $TARGET_BRANCH"

TARGET_COMMIT_ID=$4
echo "Target Commit ID is : $TARGET_COMMIT_ID"

git config user.email "jenkins@openairinterface.org"
git config user.name "OAI Jenkins"

git checkout -f $SOURCE_COMMIT_ID

git merge --ff $TARGET_COMMIT_ID -m "Temporary merge for CI"

