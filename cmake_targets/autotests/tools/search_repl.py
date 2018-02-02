#!/usr/bin/python
#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */

# \author Navid Nikaein, Rohit Gupta

import sys
import re
import os

#Arg 1 name of file
#Arg 2 keyword
#arg 3 replacement text
#Note that these should be seperated by spaces
if len(sys.argv) != 4:
  print "search_repl.py: Wrong number of arguments. This program needs 3 arguments. The number of arguments supplied : " + str(sys.argv)
  sys.exit()
filename = os.path.expandvars(sys.argv[1])
keyword = sys.argv[2]
replacement_text = sys.argv[3]


file = open(filename, 'r')
string = file.read()
file.close()


if keyword == 'mme_ip_address':
   replacement_text = keyword + ' =  ( { ' + replacement_text + ' } ) ; '
   string = re.sub(r"mme_ip_address\s*=\s*\(([^\$]+?)\)\s*;", replacement_text, string, re.M)
elif keyword == 'IPV4_LIST' or keyword=='GUMMEI_LIST' or keyword == 'TAI_LIST':
   replacement_text = keyword + ' =  ( ' + replacement_text + '  ) ; '
   string = re.sub(r"%s\s*=\s*\(([^\$]+?)\)\s*;" % keyword, replacement_text, string, re.M)
elif keyword == 'rrh_gw_config':
   replacement_text = keyword + ' =  ( { ' + replacement_text + ' } ) ; '
   string = re.sub(r"rrh_gw_config\s*=\s*\(([^\$]+?)\)\s*;", replacement_text, string, re.M)
else :
   replacement_text = keyword + ' =  ' + replacement_text + ' ; '
   string = re.sub(r"%s\s*=\s*([^\$]+?)\s*;" % keyword , replacement_text, string, re.M)   
#else : 
#   replacement_text = keyword + ' =\"' + replacement_text + '\" ; '
#   string = re.sub(r"%s\s*=\s*\"([^\$]+?)\"\s*;" % keyword , replacement_text, string, re.M)

file = open(filename, 'w')
file.write(string)
file.close()

