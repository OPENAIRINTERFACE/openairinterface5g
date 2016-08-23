#!/bin/bash

#Simple script to retry git clone in case of failure

REALGIT=/usr/bin/git

RETRIES=10
DELAY=10
COUNT=1
while [ $COUNT -lt $RETRIES ]; do
  $REALGIT $* >> git-clone-`hostname`-log.txt 2>&1
  if [ $? -eq 0 ]; then
    RETRIES=0
    break
  fi
  let COUNT=$COUNT+1
  sleep $DELAY
done
