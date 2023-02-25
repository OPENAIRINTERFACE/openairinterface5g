#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Jul  7 23:04:51 2020

@author: hardy
"""



import yaml
import sys
import subprocess

      

def main():
  f_yaml=sys.argv[1]
  f_sh=sys.argv[2]
  #filename='py_params_template.yaml'
  with open(f_yaml,'r') as file:
    # The FullLoader parameter handles the conversion from YAML
    # scalar values to Python the dictionary format
    print('Loading '+f_yaml)
    params = yaml.load(file,Loader=yaml.FullLoader)


  with open(f_sh,'w') as f:
    f.write('#!/bin/sh\n')
    for i in range (0, len(params['steps'])):
      step=params['steps'][i].split(',')
      mode=step[0]
      f_xml=step[1]
      line='python3 main.py ' + \
           '--mode='+ mode + ' ' + \
           '--ranRepository=' + params['ranRepository'] + ' ' + \
           '--ranBranch=' + params['ranBranch'] + ' ' + \
           '--ranCommitID=' + params['ranCommitID'] + ' ' + \
           '--ranAllowMerge=' + params['ranAllowMerge'] + ' ' + \
           '--ranTargetBranch=' + params['ranTargetBranch'] + ' ' + \
           \
           '--UEIPAddress=' + params['UE']['UEIPAddress'] + ' ' + \
           '--UEUserName=' + params['UE']['UEUserName'] + ' ' + \
           '--UEPassword=' + params['UE']['UEPassword'] + ' ' + \
           '--UESourceCodePath=' + params['UE']['UESourceCodePath'] + ' ' + \
           \
           '--EPCIPAddress=' + params['EPC']['EPCIPAddress'] + ' ' + \
           '--EPCUserName=' + params['EPC']['EPCUserName'] + ' ' + \
           '--EPCPassword=' + params['EPC']['EPCPassword'] + ' ' + \
           '--EPCSourceCodePath=' + params['EPC']['EPCSourceCodePath'] + ' ' + \
           '--EPCType=' + params['EPC']['EPCType'] + ' ' + \
           \
           '--eNBIPAddress=' + params['RAN'][0]['eNBIPAddress'] + ' ' + \
           '--eNBUserName=' + params['RAN'][0]['eNBUserName'] + ' ' + \
           '--eNBPassword=' + params['RAN'][0]['eNBPassword'] + ' ' + \
           '--eNBSourceCodePath=' + params['RAN'][0]['eNBSourceCodePath'] + ' ' + \
           \
           '--eNB1IPAddress=' + params['RAN'][1]['eNB1IPAddress'] + ' ' + \
           '--eNB1UserName=' + params['RAN'][1]['eNB1UserName'] + ' ' + \
           '--eNB1Password=' + params['RAN'][1]['eNB1Password'] + ' ' + \
           '--eNB1SourceCodePath=' + params['RAN'][1]['eNB1SourceCodePath'] + ' '
      if mode!="InitiateHtml":
         line+='--XMLTestFile=' + f_xml
      #if mode is InitiateHTML we have a special processing to mention all xml files from the list
      #loop starting at 1 to avoid the xml file mentioned with InitiateHtml in yaml file (file is none)
      else:
         for i in range (1, len(params['steps'])):
            step=params['steps'][i].split(',')
            f_xml=step[1]
            line+='--XMLTestFile=' + f_xml+' '
      line+='\n'
      print(line)
      f.write(line)
  subprocess.call(['chmod','777',f_sh])

if __name__ == "__main__":
    main()


