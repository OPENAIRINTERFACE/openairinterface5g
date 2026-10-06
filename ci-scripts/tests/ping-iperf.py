# SPDX-License-Identifier: LicenseRef-CSSL-1.0

import sys
import logging
logging.basicConfig(
	level=logging.DEBUG,
	stream=sys.stdout,
	format="[%(asctime)s] %(levelname)8s: %(message)s"
)
import os
os.system(f'rm -rf cmake_targets')
os.system(f'mkdir -p cmake_targets/log')
import unittest
import tempfile

sys.path.append('./') # to find OAI imports below
import cls_oai_html
from cls_ci_helper import TestCaseCtx
import cls_oaicitest
import cls_cmd

class TestPingIperf(unittest.TestCase):
	def setUp(self):
		self.html = cls_oai_html.HTMLManagement()
		self.html.testCaseId = "000000"
		self.node = "localhost"
		self.ctx = TestCaseCtx.Default(tempfile.mkdtemp())
	def tearDown(self):
		with cls_cmd.LocalCmd() as c:
			c.run(f'rm -rf {self.ctx.logPath}')

	def test_ping(self):
		opt = cls_oaicitest.PingOpt("-c3", 0.0, float('nan'))
		ep = cls_oaicitest.TrafficEndpoints.create("test", self.node, "test")
		infra_file = "tests/config/infra_ping_iperf.yaml"
		# TODO Should need nothing but options and UE(s) to use
		success = cls_oaicitest.OaiCiTest.Ping(self.ctx, self.html, opt, ep, infra_file=infra_file)
		self.assertTrue(success)

	def test_iperf(self):
		# note: needs to be five seconds because Iperf() adds -O 3, so if it is
		# too short, the server is terminated before the client loaded
		# everything
		opt = cls_oaicitest.IperfOpt("-u -t 5 -b 1M -R", 0.0, 0.0, "balanced", float('nan'))
		ep = cls_oaicitest.TrafficEndpoints.create("test", self.node, "test", "localhost")
		infra_file = "tests/config/infra_ping_iperf.yaml"
		# TODO Should need nothing but options and UE(s) to use
		success = cls_oaicitest.OaiCiTest.Iperf(self.ctx, self.html, opt, ep, infra_file=infra_file)
		self.assertTrue(success)

	def test_iperf2_unidir(self):
		opt = cls_oaicitest.IperfOpt("-u -t 5 -b 1M", 0.0, 0.0, "balanced", float('nan'))
		ep = cls_oaicitest.TrafficEndpoints.create("test", self.node, "test", "localhost")
		infra_file = "tests/config/infra_ping_iperf.yaml"
		# TODO Should need nothing but options and UE(s) to use
		success = cls_oaicitest.OaiCiTest.Iperf2_Unidir(self.ctx, self.html, opt, ep, infra_file=infra_file)
		self.assertTrue(success)

	def test_iperf_highrate(self):
		# note: needs to be five seconds because Iperf() adds -O 3, so if it is
		# too short, the server is terminated before the client loaded
		# everything
		opt = cls_oaicitest.IperfOpt("-u -t 5 -b 1000M -R -O 0", 0.0, 0.0, "balanced", float('nan'))
		ep = cls_oaicitest.TrafficEndpoints.create("test", self.node, "test", "localhost")
		infra_file = "tests/config/infra_ping_iperf.yaml"
		# TODO Should need nothing but options and UE(s) to use
		success = cls_oaicitest.OaiCiTest.Iperf(self.ctx, self.html, opt, ep, infra_file=infra_file)
		self.assertTrue(success)

	def test_iperf_new_bindport(self):
		opt = cls_oaicitest.IperfOpt("-u -t 5 -b 22M -O 0 -p 10000 -B 127.0.0.3", 0.0, 0.0, "balanced", float('nan'))
		ep = cls_oaicitest.TrafficEndpoints.create("test", self.node, "test", "localhost")
		infra_file = "tests/config/infra_ping_iperf.yaml"
		# TODO Should need nothing but options and UE(s) to use
		success = cls_oaicitest.OaiCiTest.Iperf(self.ctx, self.html, opt, ep, infra_file=infra_file)
		self.assertTrue(success)

class TestTrafficEndpoints(unittest.TestCase):
	def test_valid(self):
		self.assertTrue(cls_oaicitest.TrafficEndpoints(["ue1", "ue2"], "node", "cn", None).valid())
		self.assertFalse(cls_oaicitest.TrafficEndpoints([], "node", "cn", None).valid())
		self.assertFalse(cls_oaicitest.TrafficEndpoints([""], "node", "cn", None).valid())
		self.assertFalse(cls_oaicitest.TrafficEndpoints(["ue1"], "node", None, None).valid())
		self.assertFalse(cls_oaicitest.TrafficEndpoints(["ue1"], "node", "", None).valid())

	def test_create(self):
		ep = cls_oaicitest.TrafficEndpoints.create("ue1 ue2", "node", "cn", "srv")
		self.assertEqual(ep, cls_oaicitest.TrafficEndpoints(["ue1", "ue2"], "node", "cn", "srv"))
		self.assertTrue(ep.valid())
		ep = cls_oaicitest.TrafficEndpoints.create("ue1", "node", "cn")
		self.assertEqual(ep, cls_oaicitest.TrafficEndpoints(["ue1"], "node", "cn", None))
		self.assertTrue(ep.valid())

	def test_create_missing(self):
		ep = cls_oaicitest.TrafficEndpoints.create("ue1", "node", None)
		self.assertEqual(ep, cls_oaicitest.TrafficEndpoints(["ue1"], "node", None, None))
		self.assertFalse(ep.valid())
		ep = cls_oaicitest.TrafficEndpoints.create(None, "node", "cn")
		self.assertEqual(ep.ue_ids, [])
		self.assertFalse(ep.valid())
		ep = cls_oaicitest.TrafficEndpoints.create("", "node", "", "")
		self.assertEqual(ep, cls_oaicitest.TrafficEndpoints([], "node", None, None))
		self.assertFalse(ep.valid())

if __name__ == '__main__':
	unittest.main()
