# SPDX-License-Identifier: LicenseRef-CSSL-1.0

import sys
import logging
logging.basicConfig(
	level=logging.DEBUG,
	stream=sys.stdout,
	format="[%(asctime)s] %(levelname)8s: %(message)s"
)
import unittest
import yaml

sys.path.append('./') # to find OAI imports below
import cls_analysis
import cls_oai_html

class TestAnalysis(unittest.TestCase):
	def setUp(self):
		self.html = cls_oai_html.HTMLManagement()
		self.html.testCaseId = "000000"
		self.maxDiff = 10000 # to compare long files below

	def test_gnb_phytest(self):
		rtsf = "datalog_rt_stats.100.2x2.yaml"
		l1f = "tests/analysis/gnb_phytest.success.nrL1.log"
		macf = "tests/analysis/gnb_phytest.success.nrMAC.log"
		status, s = cls_analysis.Analysis.analyze_rt_stats(rtsf, [l1f, macf])
		self.assertTrue(status)
		self.assertEqual(s['ColNames'], ["Metric", "Average; Max; Count", "Average vs Reference Deviation (Reference Value; Acceptability Deviation Threshold)"])
		with open(rtsf, 'r') as f:
			rt_stats = yaml.load(f, Loader=yaml.FullLoader)
		# check that analyze_rt_stats() picked up all Ref/DeviationThreshold values from yaml file, e.g.
		# s['Ref']['L1 Tx processing'] == corresponding value in yaml
		# s['DeviationThreshold']['L1 Tx processing'] == corresponding value in yaml
		for k, v in rt_stats['Ref'].items():
			self.assertEqual(s['Ref'][k], v)
		for k, v in rt_stats['DeviationThreshold'].items():
			self.assertEqual(s['DeviationThreshold'][k], v)
		for k, v in s['Ref'].items():
			self.assertEqual(v, rt_stats['Ref'][k])
		for k, v in s['DeviationThreshold'].items():
			self.assertEqual(v, rt_stats['DeviationThreshold'][k])
		self.assertEqual(s['Data']['L1 Tx processing'], ['476', '872', '102933', '1.00'])
		self.assertEqual(s['Data']['DLSCH encoding'], ['216', '544', '77201', '1.08'])
		self.assertEqual(s['Data']['L1 Rx processing'], ['488', '868', '38598', '0.92'])
		self.assertEqual(s['Data']['Slot Indication'], ['18', '94', '128667', '1.38'])
		self.assertEqual(s['Data']['PUSCH inner-receiver'], ['307', '530', '25730', '0.85'])
		self.assertEqual(s['Data']['feprx'], ['169', '310', '38600', '1.12'])
		self.assertEqual(s['Data']['feptx_ofdm (per port, half_slot)'], ['56', '88', '102933', '0.98'])
		self.assertEqual(s['Data']['feptx_total'], ['148', '299', '102933', '1.00'])
		# TODO this one is not found, the CI does not recover this??
		#self.assertEqual(s['Data']['DL & UL scheduling timing'], ['17', '97', '159371', '1.14'])

	def test_analze_services_no_service(self):
		service_desc = {} # nothing to analyze
		to_analyze = ["testserv"] # some service requested to be analyze that does not exist
		status = cls_analysis.AnalyzeServices(self.html, service_desc, to_analyze)
		self.assertFalse(status)

	# TODO this does not work: we don't evaluate return codes yet
	#def test_analze_service_exit_nonnull(self):
	#	service_desc = {}
	#	service_desc["testserv"] = {'returncode': 129, 'logfile': 'tests/log-analysis/empty.log'}
	#	to_analyze = ["testserv"] # some service requested to be analyze that does not exist
	#	status = cls_analysis.AnalyzeServices(self.html, service_desc, to_analyze)
	#	self.assertFalse(status)

if __name__ == '__main__':
	unittest.main()
