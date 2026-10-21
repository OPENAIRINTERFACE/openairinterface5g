#!/bin/bash
# SPDX-License-Identifier: MIT

function die() {
  echo $@
  exit 1
}

[ $# -eq 3 ] || die "usage: $0 <directory> <repository> <branch>"

set -ex

dir=$1
repo=$2
branch=$3

rm -rf ${dir}
git init -q ${dir}
cd ${dir}
git remote add origin "${repo}"
git fetch --depth=1 origin "+refs/heads/${branch}:refs/remotes/origin/${branch}" \
                           "+refs/heads/develop:refs/remotes/origin/develop"
git checkout -q -b "${branch}" "origin/${branch}"
mkdir -p cmake_targets/log
exit 0
