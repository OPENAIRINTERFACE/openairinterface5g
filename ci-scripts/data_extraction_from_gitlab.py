#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Mar 31 21:57:37 2021

@author: hardy
"""

import subprocess
import shlex
import json
import datetime


cmd="""curl --silent "https://gitlab.eurecom.fr/api/v4/projects/oai%2Fopenairinterface5g/merge_requests?state=opened&per_page=100" """
process = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
output = process.stdout.readline()
tmp=output.decode("utf-8") 


d = json.loads(tmp)
f = open("/tmp/gitlab_dashboard.txt", "w")
f.write("MR;Created_at;Author;Title;CAN START;IN PROGRESS;COMPLETED;OK MERGE;Merge conflicts\n")
for x in range(len(d)):
    date_time_str = d[x]['created_at']
    date_time_obj = datetime.datetime.strptime(date_time_str, '%Y-%m-%dT%H:%M:%S.%fZ')
    
    milestone1=milestone2=milestone3=milestone4=""
    if d[x]['milestone']=="REVIEW_CAN_START":
        milestone1="X"
    elif d[x]['milestone']=="REVIEW_IN_PROGRESS":
        milestone2="X"
    elif d[x]['milestone']=="REVIEW_COMPLETED_AND_APPROVED":
        milestone3="X"
    elif d[x]['milestone']=="OK_TO_BE_MERGED": 
        milestone4="X"      
    else:
        pass


    if d[x]['has_conflicts']==True:
        conflicts = "YES"
    else:
        conflicts = ""

    f.write(str(d[x]['iid'])+';'+ str(date_time_obj.date())+';'+ str(d[x]['author']['name'])+';'+str(d[x]['title'])+";" \
            + milestone1 +";"+ milestone2 +";"+ milestone3 +";"+ milestone4 + ";" + conflicts +"\n")
f.close()

