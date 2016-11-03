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



#test_cases = ('030001', '030901', '031001', '031601', '031701', '031801', '031901', '032001', '032101', '032201', '032301', '032501', '032601', '032801')
test_cases = ('030030' , '030030' ) 

nb_run = 3

def error_opt(msg):
    print("Option error: " + msg)


def main(args):
	try:
		analyser = __import__("lib_autotest_analyser")
	except ImportError as err:
		print('Import error: ' + str(err))
		exit(0)


	log_path = 'log_save_2016-08-14/log/'



	metric = {}
	metric['id'] 			= 'UE_DLSCH_BITRATE'
	metric['description'] 	= 'UE downlink physical throughput'	
	metric['regex'] 		= '(UE_DLSCH_BITRATE) =\s+(\d+\.\d+) kbps.+frame = (\d+)\)'
	metric['unit_of_meas']	= 'kbps'
	metric['min_limit']		= 14668.8


#report_path = log_path+'/report/'

#os.system(' mkdir -p ' + report_path)

#analyser.create_report_html(report_path)

#return(0)

	for test_case in test_cases:

#		print test_case
		if test_case == '030001':
			metric['min_limit']		= 500.0
		if test_case == '030901':
			metric['min_limit']		= 640.0
		if test_case == '031001':
			metric['min_limit']		= 3200.0
		if test_case == '031601':
			metric['min_limit']		= 5920.0
		if test_case == '031701':
			metric['min_limit']		= 6000.0
		if test_case == '031801':
			metric['min_limit']		= 6200.0
		if test_case == '031901':
			metric['min_limit']		= 7000.0
		if test_case == '032001':
			metric['min_limit']		= 7800.0
		if test_case == '032101':
			metric['min_limit']		= 8000.0
		if test_case == '032201':
			metric['min_limit']		= 9000.0
		if test_case == '032301':
			metric['min_limit']		= 10000.0
		if test_case == '032501':
			metric['min_limit']		= 11000.0
		if test_case == '032601':
			metric['min_limit']		= 12000.0
		if test_case == '032801':
			metric['min_limit']		= 12500.0

		if test_case == '035201':
			metric['min_limit']		= 14668.8
		if test_case == '036001':
			metric['min_limit']		= 25363.2



		for i in range(0, nb_run):
			fname = 'log//'+test_case+'/run_'+str(i)+'/UE_exec_'+str(i)+'_.log'
			args = {'metric' : metric,
					'file' : fname }

			cell_synch_status = analyser.check_cell_synchro(fname)
			if cell_synch_status == 'CELL_SYNCH':
			  print '!!!!!!!!!!!!!!  Cell synchronized !!!!!!!!!!!'
			  metric_checks_flag = 0
			else :
			  print '!!!!!!!!!!!!!!  Cell NOT  NOT synchronized !!!!!!!!!!!'
	
#			metric_extracted = analyser.do_extract_metrics(args)

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

#			fname = 'log//'+test_case+'/run_'+str(i)+'/UE_traffic_'+str(i)+'_.log'
			
#			args = {'file' : fname }

#			traffic_metrics = analyser.do_extract_traffic_metrics(args)

#			fname= 'report/iperf_'+test_case+'_'+str(i)+'.png'
			
#			print fname
#			analyser.do_img_traffic(traffic_metrics, fname)



if __name__ == "__main__":
    main(sys.argv)

