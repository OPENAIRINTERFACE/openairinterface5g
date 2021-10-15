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
#---------------------------------------------------------------------
# Merge Requests Dashboard for RAN on googleSheet 
#
#   Required Python Version
#     Python 3.x
#
#---------------------------------------------------------------------

#author Remi


import pymysql
import sys
from datetime import datetime
import pickle

#This is the script/package used by the dashboard to retrieve the MR test results from the database

class SQLConnect:
    def __init__(self):
        self.connection = pymysql.connect(
                host='172.22.0.2',
                user='root', 
                password = 'ucZBc2XRYdvEm59F',
                db='oaicicd_tests',
                port=3306
                )
        self.data={} 


    #retrieve data from mysql database and organize it in a dictionary (per MR passed as argument)
    def get(self,MR):
        self.data[MR]={}
        cur=self.connection.cursor()

        #get counters per test
        sql = "select TEST,STATUS, count(*) AS COUNT from test_results where MR=(%s)  group by TEST, STATUS;"
        cur.execute(sql,MR)
        response=cur.fetchall()
        if len(response)==0:#no test results yet
            self.data[MR]['PASS']=''
            self.data[MR]['FAIL']=''
        else:
            for i in range(0,len(response)):
                test=response[i][0]
                status=response[i][1]
                count=response[i][2]
                if test in self.data[MR]:
                    self.data[MR][test][status]=count
                else:
                    self.data[MR][test]={}
                    self.data[MR][test][status]=count
      
        #get last failing build and link
        sql = "select TEST,BUILD, BUILD_LINK from test_results where MR=(%s) and STATUS='FAIL' order by DATE DESC;"
        cur.execute(sql,MR)
        response=cur.fetchall()
        if len(response)!=0:
            for i in range(0,len(response)):
                test=response[i][0]
                build=response[i][1]
                link=response[i][2]
                if 'last_fail' not in self.data[MR][test]:
                    self.data[MR][test]['last_fail']=[]
                    self.data[MR][test]['last_fail'].append(build)
                    self.data[MR][test]['last_fail'].append(link)

        #get last passing build and link
        sql = "select TEST,BUILD, BUILD_LINK from test_results where MR=(%s) and STATUS='PASS' order by DATE DESC;"
        cur.execute(sql,MR)
        response=cur.fetchall()
        if len(response)!=0:
            for i in range(0,len(response)):
                test=response[i][0]
                build=response[i][1]
                link=response[i][2]
                if 'last_pass' not in self.data[MR][test]:
                    self.data[MR][test]['last_pass']=[]
                    self.data[MR][test]['last_pass'].append(build)
                    self.data[MR][test]['last_pass'].append(link)


    #close database connection
    def close_connection(self):
        self.connection.close()


