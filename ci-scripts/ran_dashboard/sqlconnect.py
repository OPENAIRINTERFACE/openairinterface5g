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



    #close database connection
    def close_connection(self):
        self.connection.close()


