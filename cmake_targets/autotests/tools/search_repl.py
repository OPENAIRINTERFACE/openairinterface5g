#!/usr/bin/python
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
elif keyword == 'IPV4_LIST':
   replacement_text = keyword + ' =  ( ' + replacement_text + '  ) ; '
   string = re.sub(r"IPV4_LIST\s*=\s*\(([^\$]+?)\)\s*;", replacement_text, string, re.M)
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

