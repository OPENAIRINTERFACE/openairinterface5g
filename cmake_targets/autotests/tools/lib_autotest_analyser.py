#******************************************************************************
#
# \file		lib_autotest_analyser.py
#
# \par     Informations
#            - \b Project  : UED Autotest Framework
#            - \b Software : 
#
# \date		16 september 2016
#
# \version	0.1
#
# \brief	library to extract metrics from autotest logs and assign a verdict
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


import re
from pylab import *
from matplotlib.font_manager import FontProperties

import os
from jinja2 import Environment, FileSystemLoader


PATH = os.path.dirname(os.path.abspath(__file__))
TEMPLATE_ENVIRONMENT = Environment(
    autoescape=False,
    loader=FileSystemLoader(os.path.join(PATH, '../templates')),
    trim_blocks=False)


def render_template(template_filename, context):
    return TEMPLATE_ENVIRONMENT.get_template(template_filename).render(context)



def init(args = None):
    return


#
#
#
def do_extract_metrics(args):

#	print ""
#	print "do_extract_metrics ... "

	fname 	= args['file']
	metric 	= args['metric']
#	print(fname)
#	print 'metric id = ' + metric['id']
#	print 'metric regex = ' + metric['regex']

	count 		= 0
	mmin 		= 0
	mmin_index 	= 0
	mmax  		= 0
	mmax_index 	= 0
	mean 		= 0
	median 		= 0
	

	output = np.fromregex(fname,metric['regex'], [('id', 'S20'), ('metric', np.float), ('frame', np.int)] )
#	print 'T0T0 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'
#	print output
	count =  output['metric'].size
#	print count
	if count > 0:
		mmin  		= np.amin(output['metric']);
		mmin_index 	= np.argmin(output['metric']);
		mmax  		= np.amax(output['metric']);
		mmax_index 	= np.argmax(output['metric']); 			
		mean 		= np.mean(output['metric']);
		median 		= np.median(output['metric']);

#		print ( ( (metric['min_limit'] > output['metric']).sum() / float(output['metric'].size) ) * 100 )

	ret = {	'metric_count'		: count,
			'metric_buf' 		: output,
		 	'metric_min' 		: mmin, 
		 	'metric_min_index' 	: mmin_index,
			'metric_max' 		: mmax,
		 	'metric_max_index' 	: mmax_index,
			'metric_mean' 		: mean,
			'metric_median' 	: median,
			}
	return(ret)


def do_extract_metrics_new(args):

#	print ""
#	print "do_extract_metrics ... "

	fname 	= args['file']
	metric 	= args['metric']
	print(fname)
	print 'metric id = ' + metric['id']
	print 'metric regex = ' + metric['regex']



	count 		= 0
	mmin 		= 0
	mmin_index 	= 0
	mmax  		= 0
	mmax_index 	= 0
	mean 		= 0
	median 		= 0
	
	toto = [('id', 'S20'), ('metric', np.float), ('frame', np.int)]
	print toto


	np_format = []

	for x in range(0, metric['nb_metric']):
	 	np_format.append( ('id'+str(x), 'S20') )
	 	np_format.append( ('metric'+str(x), np.float) )
	 	np_format.append( ('uom'+str(x), 'S20') )
	np_format.append( ('frame', np.int)) 

	print np_format

	output = np.fromregex(fname,metric['regex'], np_format)
	print output
	count =  output['frame'].size
	print count
	if count > 0:


		fontP = FontProperties()
		fontP.set_size('small')

		fig = plt.figure(1)
		plt.figure(figsize=(10,10))

		plot_xmax = np.amax(output['frame'])+np.amin(output['frame'])

		for x in range(0, metric['nb_metric']):	

			metric_name = output['id'+str(x)][0]
			metric_uom = output['uom'+str(x)][0]

			mmin 	= np.amin(output['metric'+str(x)])
			mmax 	= np.amax(output['metric'+str(x)])
			mmean 	= np.mean(output['metric'+str(x)])
			mmedian = np.median(output['metric'+str(x)])

			plot_loc = 100*metric['nb_metric']+10+x+1

			sbplt = plt.subplot(plot_loc)
			sbplt.plot(output['frame'], output['metric'+str(x)], color='b' )
			sbplt.set_title( metric_name+' ('+metric_uom+')')
			if mmin < 0:
				sbplot_ymin=mmin+mmin/10
			else:
				sbplot_ymin=0
			sbplt.set_ylim(ymin=sbplot_ymin)	
			if mmax > 0:
				sbplot_ymax=mmax+mmax/10
			else:
				sbplot_ymax=0
			sbplt.set_ylim(ymax=sbplot_ymax)

			sbplt.set_xlim(xmax=plot_xmax)
			sbplt.set_xlim(xmin=0)
			text='min: '+str(mmin)+'\nmax: '+str(mmax)+'\nmean: '+str(mmean)+'\nmedian: '+str(mmedian)
			sbplt.text( plot_xmax+10,sbplot_ymin,text)
			sbplt.set_xlabel('frame')
			sbplt.set_ylabel(metric_name)

		plt.tight_layout()

		fname = "toto.png"

#		lgd = plt.legend(prop=fontP, bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0.)
		mng = plt.get_current_fig_manager()
		plt.savefig(fname, bbox_inches='tight')
		plt.close()









		mmin  		= np.amin(output['metric']);
		mmin_index 	= np.argmin(output['metric']);
		mmax  		= np.amax(output['metric']);
		mmax_index 	= np.argmax(output['metric']); 			
		mean 		= np.mean(output['metric']);
		median 		= np.median(output['metric']);

#		print ( ( (metric['min_limit'] > output['metric']).sum() / float(output['metric'].size) ) * 100 )

	ret = {	'metric_count'		: count,
		'metric_buf' 		: output,
		

		'metric_min' 		: mmin, 
		'metric_min_index' 	: mmin_index,
		'metric_max' 		: mmax,
		'metric_max_index' 	: mmax_index,
		'metric_mean' 		: mean,
		'metric_median' 	: median,
		}
	return(ret)

#
#
#
def do_check_verdict(metric_def, metric_data):
	verdict = 'INCON'

	pass_fail_stat = metric_def['pass_fail_stat']

	if pass_fail_stat == 'max_value':
		metric_stat = metric_data['metric_max']
	elif pass_fail_stat == 'min_value':
		metric_stat = metric_data['metric_min']
	elif pass_fail_stat == 'mean_value':
		metric_stat = metric_data['metric_mean']
	elif pass_fail_stat == 'median_value':
		metric_stat = metric_data['metric_median']
	else :
		print "do_check_verdict -> undef metric stat (pass_fail_stat in xml file)"
		return verdict

	if 'max_limit' in metric_def:
		if  metric_stat > metric_def['max_limit']:
			verdict =  'FAIL'
		else:
			verdict = 'PASS'

	if 'min_limit' in metric_def:
		if metric_stat < metric_def['min_limit']:
			verdict = 'FAIL'
		else:
			verdict = 'PASS'
	return verdict


def do_print_metrics(metric_def, metric_data):


	if metric_data['metric_count'] > 0 :

#		output = np.array( metric_data['metric_buf'] , [('id', 'S20'), ('metric', np.float), ('frame', np.int)])
		output = metric_data['metric_buf']
#		print output

		fontP = FontProperties()
		fontP.set_size('small')

		plt.scatter(output['frame'], output['metric'], color='b', alpha=0.33, s = 1 )
		plt.plot([0, output['frame'][metric_data['metric_count']-1]],[ metric_def['min_limit'],metric_def['min_limit']], 'r-', lw=2) # Red straight line

		plt.title('Physical throughput ')
		plt.xlabel('frame')
		plt.ylabel(metric_def['id'])
		plt.legend(prop=fontP)
		mng = plt.get_current_fig_manager()
		plt.show()

def do_img_metrics(metric_def, metric_data, fname):


	if metric_data['metric_count'] > 0 :

#		output = np.array( metric_data['metric_buf'] , [('id', 'S20'), ('metric', np.float), ('frame', np.int)])
		output = metric_data['metric_buf']
#		print 'TITI !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'
#		print output

#		print metric_def['min_limit']
#		print metric_data['metric_count']
#		print output['frame'][metric_data['metric_count']-1]

		fontP = FontProperties()
		fontP.set_size('small')

		plt.figure()

#		print output['frame'].size
#		print output['metric'].size

		plt.scatter(output['frame'], output['metric'], color='b', alpha=0.33, s = 1 , label=metric_def['id'])
		
		if 'min_limit' in metric_def:
			plt.plot([0, output['frame'][metric_data['metric_count']-1]],[ metric_def['min_limit'],metric_def['min_limit']], 'r-', lw=2, label='min limit') # Red straight line

		plt.title(metric_def['id'] +' ('+metric_def['unit_of_meas']+')')
		plt.xlabel('frame')
		plt.ylabel(metric_def['id'])
		
		# Set graphic minimum Y axis
		# -------------------------
		if metric_data['metric_min'] < 0:
			plt.ylim(ymin=metric_data['metric_min']+metric_data['metric_min']/10)
		else :	
			plt.ylim(ymin=0)

		y_axis_max = 0
		if 'min_limit' in metric_def:
			if metric_data['metric_max'] >  metric_def['min_limit']:
				y_axis_max =metric_data['metric_max']+metric_data['metric_max']/10
			else:
				y_axis_max =metric_def['min_limit']+metric_def['min_limit']/10
		else:
			y_axis_max =metric_data['metric_max']+metric_data['metric_max']/10


		plt.ylim(ymax=y_axis_max)

		lgd = plt.legend(prop=fontP, bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0.)
		mng = plt.get_current_fig_manager()
		plt.savefig(fname, bbox_extra_artists=(lgd,), bbox_inches='tight')
		plt.close()

#	with open(fname, 'r') as f:
#		for line in f:
#			m = re.search(metric['regex'], line)
#			if m :
#				print m.group(1) + " -> "+m.group(2)




def do_extract_traffic_metrics(args):

	fname 	= args['file']

#	print(fname)
#	print 'metric id = ' + metric['id']

#[ ID] Interval       Transfer     Bandwidth        Jitter   Lost/Total Datagrams
#[  3]  0.0- 1.0 sec   238 KBytes  1.95 Mbits/sec   0.980 ms    2/  174 (1.1%)
# [  3]  0.0- 1.0 sec  63.2 KBytes   517 Kbits/sec   0.146 ms    1/   45 (2.2%)

#	iperf_regex = '[\s*(\d+)]\s+\d+\.\d+-\s*(\d+\.\d+)\s+sec\s+(\d+).+(\d+\.\d+).+(\d+\.\d+).+(\d+)\/\s*(\d+)\s+\((\d+\.\d+)\%\)'
	#                    ID          0.0            1.0                 63.2        KByte   517          Kbits/s   0.146
	iperf_regex = '\[\s*(\d+)\]\s+(\d+\.*\d*)-\s*(\d+\.*\d*)\s+sec\s+(\d+\.*\d*)\s+(\D+)\s+(\d+\.*\d*)\s+(\D+)\s+(\d+\.*\d*)\s+ms\s+(\d+)\/\s*(\d+)\s+\((\d+\.*\d*)\%\)'
#	print 'iperf regex = ' + iperf_regex

	count 		= 0
	bw_min 		= 0
	bw_max 		= 0
	bw_mean 	= 0
	bw_median	= 0
	jitter_min 		= 0
	jitter_max 		= 0
	jitter_mean 	= 0
	jitter_median	= 0

	rl_min 		= 0
	rl_max 		= 0
	rl_mean 	= 0
	rl_median	= 0

	interval_stop_max = 0
	
	output = np.fromregex(fname,iperf_regex, [('id', np.int) , ('interval_start', np.float), ('interval_stop', np.float), ('transfer', np.float), ('transfer_uom', 'S20') ,('bandwidth', np.float), ('bandwidth_uom', 'S20') ,('jitter', np.float), ('lost', np.int) , ('total', np.int), ('rate_lost', np.float) ] )

	count = output['id'].size -1
	# remove last line that is an iperf result resume

	if count > 0:

		output= np.delete(output, (count), axis=0 )
#		print output

		bw_min 		= np.amin(output['bandwidth']);
		bw_max 		= np.amax(output['bandwidth']);
		bw_mean 	= np.mean(output['bandwidth']);
		bw_median	= np.median(output['bandwidth']);

		jitter_min 		= np.amin(output['jitter']);
		jitter_max 		= np.amax(output['jitter']);
		jitter_mean 	= np.mean(output['jitter']);
		jitter_median	= np.median(output['jitter']);

		rl_min 		= np.amin(output['rate_lost']);
		rl_max 		= np.amax(output['rate_lost']);
		rl_mean 	= np.mean(output['rate_lost']);
		rl_median	= np.median(output['rate_lost']);

		interval_stop_max = np.amax(output['interval_stop']);

	else :
		count = 0


	ret = {	'traffic_count'	: count,
			'traffic_buf' 	: output,
			'bw_min' 		: bw_min,
			'bw_max' 		: bw_max,
			'bw_mean' 		: bw_mean,
			'bw_median'		: bw_median,
			'jitter_min' 	: jitter_min,
			'jitter_max' 	: jitter_max,
			'jitter_mean' 	: jitter_mean,
			'jitter_median'	: jitter_median,
			'rl_min' 		: rl_min,
			'rl_max' 		: rl_max,
			'rl_mean' 		: rl_mean,
			'rl_median'		: rl_median,
			'interval_stop_max' : interval_stop_max
			}
	return(ret)




def do_img_traffic(traffic_data, fname):


	if traffic_data['traffic_count'] > 0 :

		output = traffic_data['traffic_buf']


		fontP = FontProperties()
		fontP.set_size('small')

		fig = plt.figure(1)

		ax1= plt.subplot(311)
		plt.plot(output['interval_stop'], output['bandwidth'], color='b' )
		ax1.set_title('Bandwidth (Mbits/s)')
		ax1.set_ylim(ymin=-1)
		ax1.set_xlim(xmax=np.amax(output['interval_stop']))
		text='min: '+str(traffic_data['bw_min'])+'\nmax: '+str(traffic_data['bw_max'])+'\nmean: '+str(traffic_data['bw_mean'])+'\nmedian: '+str(traffic_data['bw_median'])
		ax1.text( np.amax(output['interval_stop'])+10,0,text)
		ax1.set_xlabel('time (s)')
		ax1.set_ylabel(' ')

		ax2=plt.subplot(312)
		plt.plot(output['interval_stop'], output['jitter'], color='b' )
		ax2.set_title('Jitter (ms)')
		ax2.set_xlim(xmax=np.amax(output['interval_stop']))
		ax2.set_ylim(ymin=-1)
		text='min: '+str(traffic_data['jitter_min'])+'\nmax: '+str(traffic_data['jitter_max'])+'\nmean: '+str(traffic_data['jitter_mean'])+'\nmedian: '+str(traffic_data['jitter_median'])
		ax2.text( np.amax(output['interval_stop'])+10,0,text)
		ax2.set_xlabel('time (s)')
		ax2.set_ylabel(' ')

		ax3=plt.subplot(313)
		plt.plot(output['interval_stop'], output['rate_lost'], color='b')
		ax3.set_title('Loss rate %')
		ax3.set_xlim(xmax=np.amax(output['interval_stop']))
		ax3.set_ylim(ymin=-1)
		text='min: '+str(traffic_data['rl_min'])+'\nmax: '+str(traffic_data['rl_max'])+'\nmean: '+str(traffic_data['rl_mean'])+'\nmedian: '+str(traffic_data['rl_median'])
		ax3.text( np.amax(output['interval_stop'])+10,0,text)
		ax3.set_xlabel('time (s)')
		ax3.set_ylabel(' ')

#		plt.title('Physical throughput ('+metric_def['unit_of_meas']+')')
#		plt.xlabel('time (s)')
#		plt.ylabel(metric_def['id'])
		
		# Set graphic minimum Y axis
		# -------------------------
#		if traffic_data['bw_min'] == 0 :
#			plt.ylim(ymin=-metric_def['min_limit']/10)
#		else :	
#			plt.ylim(ymin=0)

#		y_axis_max = 0
#		if traffic_data['metric_max'] >  metric_def['min_limit']:
#			y_axis_max =traffic_data['metric_max']+traffic_data['metric_max']/10
#		else:
#			y_axis_max =metric_def['min_limit']+metric_def['min_limit']/10
#
#		plt.ylim(ymax=y_axis_max)

		plt.tight_layout()

#		lgd = plt.legend(prop=fontP, bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0.)
		mng = plt.get_current_fig_manager()
		plt.savefig(fname, bbox_inches='tight')
		plt.close()




def create_report_html(context):
    fname = context['report_path']+"/index.html"
  
    #
    with open(fname, 'w') as f:
        html = render_template('index.html', context)
        f.write(html)


def create_test_report_detailed_html(context, fname ):
    with open(fname, 'w') as f:
        html = render_template('testcase_report.html', context)
        f.write(html)


def check_cell_synchro(fname):	

	with open(fname, 'r') as f:
		for line in f:

			m = re.search('AUTOTEST Cell Sync \:', line)
			if m :
				#print line
				return 'CELL_SYNCH'

	return 'CELL_NOT_SYNCH'


def check_exec_seg_fault(fname):	

	with open(fname, 'r') as f:
		for line in f:
			m = re.search('Segmentation fault', line)
			if m :
				#print line
				return 'SEG_FAULT'

	return 'NO_SEG_FAULT'

