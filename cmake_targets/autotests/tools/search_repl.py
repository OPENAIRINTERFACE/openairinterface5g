#!/usr/bin/python
import sys
import re

#Arg 1 name of file
#Arg 2 keyword
#arg 3 replacement text
#Note that these should be seperated by spaces
if len(sys.argv) != 4:
  print "search_repl.py: Wrong number of arguments. This program needs 3 arguments"
  sys.exit()
filename = sys.argv[1]
keyword = sys.argv[2]
replacement_text = sys.argv[3]

file = open(filename, 'r')
string = file.read()
file.close()


if keyword == 'mme_ip_address':
   #string =  (re.sub(r"mme_ip_address\s*=\s*\([^\$]+)\)\s*;\s*", r"<% tex \1 %>", t, re.M)
   replacement_text = keyword + ' =  ' + replacement_text + ' ; '
   string = re.sub(r"mme_ip_address\s*=\s*\(([^\$]+?)\)\s*;", replacement_text, string, re.M)
elif keyword == 'N_RB_DL':
   replacement_text = keyword + ' =  ' + replacement_text + ' ; '
   string = re.sub(r"%s\s*=\s*([^\$]+?)\s*;" % keyword , replacement_text, string, re.M)   
else : 
   replacement_text = keyword + ' = \" ' + replacement_text + '\" ; '
   string = re.sub(r"%s\s*=\s*\"([^\$]+?)\"\s*;" % keyword , replacement_text, string, re.M)

file = open(filename, 'w')
file.write(string)
file.close()

