#!/usr/bin/python
#******************************************************************************

#    OpenAirInterface 
#    Copyright(c) 1999 - 2014 Eurecom

#    OpenAirInterface is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.


#    OpenAirInterface is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with OpenAirInterface.The full GNU General Public License is 
#   included in this distribution in the file called "COPYING". If not, 
#   see <http://www.gnu.org/licenses/>.

#  Contact Information
#  OpenAirInterface Admin: openair_admin@eurecom.fr
#  OpenAirInterface Tech : openair_tech@eurecom.fr
#  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr
  
#  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

# *******************************************************************************/
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

