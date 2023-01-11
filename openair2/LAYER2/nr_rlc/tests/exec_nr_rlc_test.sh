#!/bin/bash

# this script is to be executed in the current folder. It takes one argument,
# which is the directory in which the executables are.

if [ $# -ne 1 ]; then
  echo "usage: $0 <directory-of-execs>"
  exit 1
fi

EXEC_DIR=$1

for i in {1..17}; do
  diff \
      <($EXEC_DIR/nr_rlc_test_$i | \grep ^TEST) \
      <(gunzip -c test$i.txt.gz) \
    || exit 1
done
