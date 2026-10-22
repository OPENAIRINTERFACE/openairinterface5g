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
import cls_loganalysis

class TestLogAnalysis(unittest.TestCase):
    def setUp(self):
        self.maxDiff = None

    #def test_no_file(self):
    #    f = "/total/fantasy/path/file.log"
    #    result, _ = cls_loganalysis.Default.run(f, None)
    #    self.assertFalse(result)

    def test_no_error(self):
        f = "tests/log-analysis/empty.log"
        result, _ = cls_loganalysis.Default.run(f, None)
        self.assertTrue(result)

    def test_retx_check_good(self):
        f = "tests/log-analysis/retx-check-good.log"
        opt = "dl=1,0,0,0;ul=1,0,0,0"
        result, log = cls_loganalysis.RetxCheck.run(f, opt)
        expected = """UE f40a: dlsch_rounds 86/0/0/0, dlsch_errors 0, pucch0_DTX 0, BLER 0.00052 MCS (0) 0 CCE fail 3
UE f40a: ulsch_rounds 518/0/0/0, ulsch_errors 0, ulsch_DTX 0, BLER 0.00000 MCS (0) 0 (Qm 2 deltaMCS 0 dB) NPRB 5  SNR 51.0 dB CCE fail 0"""
        self.assertTrue(result)
        self.assertEqual(log, expected)

    def test_retx_check_good_single_val(self):
        f = "tests/log-analysis/retx-check-good.log"
        opt = "1,0,0,0"
        result, log = cls_loganalysis.RetxCheck.run(f, opt)
        expected = """UE f40a: dlsch_rounds 86/0/0/0, dlsch_errors 0, pucch0_DTX 0, BLER 0.00052 MCS (0) 0 CCE fail 3
UE f40a: ulsch_rounds 518/0/0/0, ulsch_errors 0, ulsch_DTX 0, BLER 0.00000 MCS (0) 0 (Qm 2 deltaMCS 0 dB) NPRB 5  SNR 51.0 dB CCE fail 0"""
        self.assertTrue(result)
        self.assertEqual(log, expected)

    def test_retx_check_good_ul_only(self):
        f = "tests/log-analysis/retx-check-good.log"
        opt = "ul=1,0,0,0"
        result, log = cls_loganalysis.RetxCheck.run(f, opt)
        expected = """UE f40a: dlsch_rounds 86/0/0/0, dlsch_errors 0, pucch0_DTX 0, BLER 0.00052 MCS (0) 0 CCE fail 3
UE f40a: ulsch_rounds 518/0/0/0, ulsch_errors 0, ulsch_DTX 0, BLER 0.00000 MCS (0) 0 (Qm 2 deltaMCS 0 dB) NPRB 5  SNR 51.0 dB CCE fail 0"""
        self.assertTrue(result)
        self.assertEqual(log, expected)

    def test_retx_check_bad(self):
        f = "tests/log-analysis/retx-check-bad.log"
        opt = "dl=10,100,100,100;ul=10,100,100,100"
        result, log = cls_loganalysis.RetxCheck.run(f, opt)
        expected = """UE a491: dlsch_rounds 13/1/0/0, dlsch_errors 0, pucch0_DTX 0, BLER 0.11000 MCS (0) 0 CCE fail 0
UE a491: ulsch_rounds 23/1/0/0, ulsch_errors 0, ulsch_DTX 1, BLER 0.08447 MCS (0) 0 (Qm 2 deltaMCS 0 dB) NPRB 5  SNR 34.0 dB CCE fail 0
UE bf9b: dlsch_rounds 370/368/368/368, dlsch_errors 368, pucch0_DTX 1377, BLER 1.00000 MCS (0) 0 CCE fail 64
UE bf9b: ulsch_rounds 150/148/147/147, ulsch_errors 147, ulsch_DTX 589, BLER 1.00000 MCS (0) 0 (Qm 2 deltaMCS 0 dB) NPRB 104  SNR 38.0 dB CCE fail 0
!!! Failure: UE bf9b: retx rounds DL [1] UL [1] exceeded threshold
UE 257a: dlsch_rounds 68071/142/14/0, dlsch_errors 0, pucch0_DTX 10, BLER 0.00427 MCS (1) 25 CCE fail 0
UE 257a: ulsch_rounds 41777/2863/10/0, ulsch_errors 0, ulsch_DTX 17, BLER 0.11456 MCS (1) 10 (Qm 4 deltaMCS 0 dB) NPRB 5  SNR 29.0 dB CCE fail 0"""
        self.assertFalse(result)
        self.assertEqual(log, expected)

    def test_retx_check_dl_only_bad(self):
        f = "tests/log-analysis/retx-check-bad.log"
        opt = "dl=10,80,100,100"
        result, log = cls_loganalysis.RetxCheck.run(f, opt)
        expected = """UE a491: dlsch_rounds 13/1/0/0, dlsch_errors 0, pucch0_DTX 0, BLER 0.11000 MCS (0) 0 CCE fail 0
UE a491: ulsch_rounds 23/1/0/0, ulsch_errors 0, ulsch_DTX 1, BLER 0.08447 MCS (0) 0 (Qm 2 deltaMCS 0 dB) NPRB 5  SNR 34.0 dB CCE fail 0
UE bf9b: dlsch_rounds 370/368/368/368, dlsch_errors 368, pucch0_DTX 1377, BLER 1.00000 MCS (0) 0 CCE fail 64
UE bf9b: ulsch_rounds 150/148/147/147, ulsch_errors 147, ulsch_DTX 589, BLER 1.00000 MCS (0) 0 (Qm 2 deltaMCS 0 dB) NPRB 104  SNR 38.0 dB CCE fail 0
!!! Failure: UE bf9b: retx rounds DL [1, 2] exceeded threshold
UE 257a: dlsch_rounds 68071/142/14/0, dlsch_errors 0, pucch0_DTX 10, BLER 0.00427 MCS (1) 25 CCE fail 0
UE 257a: ulsch_rounds 41777/2863/10/0, ulsch_errors 0, ulsch_DTX 17, BLER 0.11456 MCS (1) 10 (Qm 4 deltaMCS 0 dB) NPRB 5  SNR 29.0 dB CCE fail 0"""
        self.assertFalse(result)
        self.assertEqual(log, expected)

    def test_retx_check_ul_bad(self):
        f = "tests/log-analysis/retx-check-bad.log"
        opt = "dl=100,100,100,100;ul=10,100,100,100"
        result, log = cls_loganalysis.RetxCheck.run(f, opt)
        expected = """UE a491: dlsch_rounds 13/1/0/0, dlsch_errors 0, pucch0_DTX 0, BLER 0.11000 MCS (0) 0 CCE fail 0
UE a491: ulsch_rounds 23/1/0/0, ulsch_errors 0, ulsch_DTX 1, BLER 0.08447 MCS (0) 0 (Qm 2 deltaMCS 0 dB) NPRB 5  SNR 34.0 dB CCE fail 0
UE bf9b: dlsch_rounds 370/368/368/368, dlsch_errors 368, pucch0_DTX 1377, BLER 1.00000 MCS (0) 0 CCE fail 64
UE bf9b: ulsch_rounds 150/148/147/147, ulsch_errors 147, ulsch_DTX 589, BLER 1.00000 MCS (0) 0 (Qm 2 deltaMCS 0 dB) NPRB 104  SNR 38.0 dB CCE fail 0
!!! Failure: UE bf9b: retx rounds UL [1] exceeded threshold
UE 257a: dlsch_rounds 68071/142/14/0, dlsch_errors 0, pucch0_DTX 10, BLER 0.00427 MCS (1) 25 CCE fail 0
UE 257a: ulsch_rounds 41777/2863/10/0, ulsch_errors 0, ulsch_DTX 17, BLER 0.11456 MCS (1) 10 (Qm 4 deltaMCS 0 dB) NPRB 5  SNR 29.0 dB CCE fail 0"""
        self.assertFalse(result)
        self.assertEqual(log, expected)

    def test_last_line_contains_false(self):
        f = "tests/log-analysis/arbitrary.log"
        opt = "Bye."
        result, log = cls_loganalysis.LastLineContains.run(f, opt)
        expected = f"could not find '{opt}' in last line"
        self.assertFalse(result)
        self.assertEqual(log, expected)

    def test_last_line_contains_true(self):
        f = "tests/log-analysis/arbitrary.log"
        opt = "got sync"
        result, log = cls_loganalysis.LastLineContains.run(f, opt)
        expected = ""
        self.assertTrue(result)
        self.assertEqual(log, expected)

    def test_ends_no_bye(self):
        f = "tests/log-analysis/arbitrary.log"
        opt = None
        result, log = cls_loganalysis.EndsWithBye.run(f, opt)
        expected = "No Bye. message found, did not stop properly"
        self.assertFalse(result)
        self.assertEqual(log, expected)

    def test_ends_with_bye(self):
        f = "tests/log-analysis/with-bye.log"
        opt = None
        result, log = cls_loganalysis.EndsWithBye.run(f, opt)
        expected = ""
        self.assertTrue(result)
        self.assertEqual(log, expected)


if __name__ == '__main__':
	unittest.main()
