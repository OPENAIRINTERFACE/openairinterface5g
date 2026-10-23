# SPDX-License-Identifier: LicenseRef-CSSL-1.0

import sys
import logging
logging.basicConfig(
	level=logging.DEBUG,
	stream=sys.stdout,
	format="[%(asctime)s] %(levelname)8s: %(message)s"
)
import unittest

sys.path.append('./') # to find OAI imports below
import cls_oaicitest

class TestIperfAnalysis(unittest.TestCase):
	def setUp(self):
		self.iperf_bitrate_threshold = "99"
		self.iperf_packetloss_threshold = "0"

	def test_iperf_analyzeV3UDP_ok(self):
		filename_ok = "tests/log/iperf_udp_test_ok.log"
		msg_filename_ok = "tests/log/iperf_udp_msg_ok.txt"
		target_bitrate = "1"
		with open(msg_filename_ok, 'r') as file:
			expected_msg_ok = file.read().strip()
		success, msg = cls_oaicitest.Iperf_analyzeV3UDP(filename_ok, self.iperf_bitrate_threshold, self.iperf_packetloss_threshold, target_bitrate)
		self.assertEqual(msg, expected_msg_ok)
		self.assertTrue(success)

	def test_iperf_analyzeV3UDP_ok2(self):
		filename_ok = "tests/log/iperf_udp_test_ok2.log"
		msg_filename_ok = "tests/log/iperf_udp_msg_ok2.txt"
		target_bitrate = "4.67"
		with open(msg_filename_ok, 'r') as file:
			expected_msg_ok = file.read().strip()
		success, msg = cls_oaicitest.Iperf_analyzeV3UDP(filename_ok, self.iperf_bitrate_threshold, self.iperf_packetloss_threshold, target_bitrate)
		self.assertEqual(msg, expected_msg_ok)
		self.assertTrue(success)

	def test_iperf_analyzeV3UDP_nok(self):
		filename_nok = "tests/log/iperf_udp_test_nok.log"
		msg_filename_nok = "tests/log/iperf_udp_msg_nok.txt"
		target_bitrate = "1"
		with open(msg_filename_nok, 'r') as file:
			expected_msg_nok = file.read().strip()
		success, msg = cls_oaicitest.Iperf_analyzeV3UDP(filename_nok, self.iperf_bitrate_threshold, self.iperf_packetloss_threshold, target_bitrate)
		self.assertEqual(msg, expected_msg_nok)
		self.assertFalse(success)

	def test_iperf_analyzeV3UDP_nok2(self):
		filename_nok = "tests/log/iperf_udp_test_nok2.log"
		msg_filename_nok = "tests/log/iperf_udp_msg_nok2.txt"
		target_bitrate = "80"
		with open(msg_filename_nok, 'r') as file:
			expected_msg_nok = file.read().strip()
		success, msg = cls_oaicitest.Iperf_analyzeV3UDP(filename_nok, self.iperf_bitrate_threshold, self.iperf_packetloss_threshold, target_bitrate)
		self.assertEqual(msg, expected_msg_nok)
		self.assertFalse(success)

	def test_iperf_analyzeV3UDP_nok3(self):
		filename_nok = "tests/log/iperf_udp_test_nok3.log"
		msg_filename_nok = "tests/log/iperf_udp_msg_nok3.txt"
		target_bitrate = "4.67"
		with open(msg_filename_nok, 'r') as file:
			expected_msg_nok = file.read().strip()
		success, msg = cls_oaicitest.Iperf_analyzeV3UDP(filename_nok, self.iperf_bitrate_threshold, self.iperf_packetloss_threshold, target_bitrate)
		self.assertEqual(msg, expected_msg_nok)
		self.assertFalse(success)

	def test_iperf_analyzeV3UDP_notfound(self):
		filename_notfound = "tests/log/iperf_udp_test_notfound.log"
		target_bitrate = "1"
		success, msg = cls_oaicitest.Iperf_analyzeV3UDP(filename_notfound, self.iperf_bitrate_threshold, self.iperf_packetloss_threshold, target_bitrate)
		self.assertEqual(msg, "Iperf3 UDP: Log file not present")
		self.assertFalse(success)

	def test_iperf_analyzeV2UDP(self):
		filename_ok = "tests/log/iperf_udp_v2_ok.log"
		msg_filename_ok = "tests/log/iperf_udp_v2_ok.txt"
		target_bitrate = "2.00"
		bitrate_thresh = 80
		packetloss_thresh = 25
		with open(msg_filename_ok, 'r') as file:
			expected_msg_ok = file.read().strip()
		success, msg = cls_oaicitest.Iperf_analyzeV2UDP(filename_ok, bitrate_thresh, packetloss_thresh, target_bitrate)
		self.assertEqual(msg, expected_msg_ok)
		self.assertTrue(success)

if __name__ == '__main__':
	unittest.main()
