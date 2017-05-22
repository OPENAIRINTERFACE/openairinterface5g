#! /usr/bin/python
#******************************************************************************
#
# \file		autotest_analyser.py
#
# \par     Informations
#            - \b Project  : UED Autotest Framework
#            - \b Software : 
#
# \date		16 september 2016
#
# \version	0.1
#
# \brief	helper to test lib_autotest_analyser.py
#
# \author	Benoit ROBERT (benoit.robert@syrtem.com)
#
# \par		Statement of Ownership
#           COPYRIGHT (c) 2016  BY SYRTEM S.A.R.L
#           This software is furnished under license and may be used and copied 
#			only in accordance with the terms of such license and with the inclusion
#			of the above COPYRIGHT notice. This SOFTWARE or any other copies thereof
#			may not be provided or otherwise made available to any other person. 
#			No title to and ownership of the SOFTWARE is hereby transferred.
#
#			The information in this SOFTWARE is subject to change without notice 
#			and should not be constructed as a commitment by SYRTEM.
#           SYRTEM assumes no responsibility for the use or reliability of its 
#			SOFTWARE on equipment or platform not explicitly validated by SYRTEM.
#
# *******************************************************************************

import os
import getopt
import sys
from subprocess import call

import encoder

sys.path.append(os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/tools/'))


#test_cases = ('030001', '030901', '031001', '031601', '031701', '031801', '031901', '032001', '032101', '032201', '032301', '032501', '032601', '032801')
test_cases = ('032800' , '032730' ) 

nb_run = 2

def error_opt(msg):
    print("Option error: " + msg)


def main(args):
	try:
		analyser = __import__("lib_autotest_analyser")
	except ImportError as err:
		print('Import error: ' + str(err))
		exit(0)


	log_path = 'log_save_2016-08-14/log/'



	# metric = {}
	# metric['id'] 			= 'UE_DLSCH_BITRATE'
	# metric['description'] 	= 'UE downlink physical throughput'	
	# metric['regex'] 		= '(UE_DLSCH_BITRATE) =\s+(\d+\.\d+) kbps.+frame = (\d+)\)'
	# metric['unit_of_meas']	= 'kbps'
	# metric['min_limit']		= 14668.8


#AUTOTEST Metric : RRC Measurments RSRP[0]=-97.60 dBm/RE, RSSI=-72.83 dBm, RSRQ[0] 9.03 dB, N0 -125 dBm/RE, NF 7.2 dB (frame = 4490)

	metric = {}
	metric['id'] 		= 'UE_DL_RRC_MEAS'
	metric['description'] 	= 'UE downlink RRC Measurments'	
	metric['nb_metric']	= 5
#	metric['regex'] 	= 'AUTOTEST Metric : RRC Measurments (RSRP\[0\])=(-?\d+\.?\d*)\s+(.+),\s+(RSRQ\[0\])=(-?\d+\.?\d*)\s+(.+),,\s+(N0)=(-?\d+\.?\d*)\s+(.+),,\s+(NF)=(-?\d+\.?\d*)\s+(.+)\s+\(frame = (\d+)\) '
	metric['regex'] 	= 'AUTOTEST Metric : RRC Measurments (RSRP\[0\])=(-?\d+\.?\d*)\s+(.+)\,\s+(RSSI)=(-?\d+\.?\d*)\s+(.+)\,\s+(RSRQ\[0\])=(-?\d+\.?\d*)\s+(.+)\,\s+(N0)=(-?\d+\.?\d*)\s+(.+)\,\s+(NF)=(-?\d+\.?\d*)\s+(.+)\s+\(frame = (\d+)\)'
	metric['unit_of_meas']	= 'kbps'
	metric['min_limit']		= 14668.8



#report_path = log_path+'/report/'

#os.system(' mkdir -p ' + report_path)

#analyser.create_report_html(report_path)

#return(0)

	test_results = []

	for test_case in test_cases:

		for i in range(0, nb_run):
			fname = '..//log//'+test_case+'/run_'+str(i)+'/UE_exec_'+str(i)+'_.log'
			args = {'metric' : metric,
					'file' : fname }

			# cell_synch_status = analyser.check_cell_synchro(fname)
			# if cell_synch_status == 'CELL_SYNCH':
			#   print '!!!!!!!!!!!!!!  Cell synchronized !!!!!!!!!!!'
			#   metric_checks_flag = 0
			# else :
			#   print '!!!!!!!!!!!!!!  Cell NOT  NOT synchronized !!!!!!!!!!!'
	
#			metrics_extracted = analyser.do_extract_metrics_new(args)


			# de-xmlfy test report
			xml_file = '..//log//'+test_case+'/test.'+test_case+'_ng.xml'
			print xml_file

		# 	test_result =


  # 			test_results.append(test_result)

  # xmlFile = logdir_local_testcase + '/test.' + testcasename + '.xml'
  # xml="\n<testcase classname=\'"+ testcaseclass +  "\' name=\'" + testcasename + "."+tags +  "\' Run_result=\'" + test_result_string + "\' time=\'" + str(duration) + " s \' RESULT=\'" + testcase_verdict + "\'></testcase> \n"
  # write_file(xmlFile, xml, mode="w")

  # xmlFile_ng = logdir_local_testcase + '/test.' + testcasename + '_ng.xml'
  # xml_ng = xmlify(test_result, wrap=testcasename, indent="  ")
  # write_file(xmlFile_ng, xml_ng, mode="w")




#			print "min       = "+ str( metric_extracted['metric_min'] )
#			print "min_index = "+ str( metric_extracted['metric_min_index'] )
#			print "max       = "+ str( metric_extracted['metric_max'] )
#			print "max_index = "+ str( metric_extracted['metric_max_index'] )
#			print "mean      = "+ str( metric_extracted['metric_mean'] )
#			print "median    = "+ str( metric_extracted['metric_median'] )			

#			verdict = analyser.do_check_verdict(metric, metric_extracted)
#			print verdict

#			fname= 'report/2016-9-8_toto/'+test_case+'/UE_metric_UE_DLSCH_BITRATE_'+str(i)+'_.png'
#			fname= 'report/UE_metric_UE_DLSCH_BITRATE_'+test_case+'_'+str(i)+'.png'
			
#			print fname
#			analyser.do_img_metrics(metric, metric_extracted, fname)

			# fname = 'log//'+test_case+'/run_'+str(i)+'/UE_traffic_'+str(i)+'_.log'
			
			# args = {'file' : fname }

			# traffic_metrics = analyser.do_extract_traffic_metrics(args)

			# fname= 'report/iperf_'+test_case+'_'+str(i)+'.png'
			
			# print fname
			# analyser.do_img_traffic(traffic_metrics, fname)


	for test_result in test_results:
	  cmd = 'mkdir -p ' + report_dir + '/'+ test_result['testcase_name']
	  result = os.system(cmd)

	  report_file = report_dir + '/'+ test_result['testcase_name'] + '/'+ test_result['testcase_name']+ '_report.html'

	  analyser.create_test_report_detailed_html(test_result, report_file )

	  print test_result



if __name__ == "__main__":
    main(sys.argv)

