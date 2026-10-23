# SPDX-License-Identifier: LicenseRef-CSSL-1.0

import sys
import logging
logging.basicConfig(
	level=logging.DEBUG,
	stream=sys.stdout,
	format="[%(asctime)s] %(levelname)8s: %(message)s"
)
import tempfile

import unittest

sys.path.append('./') # to find OAI imports below
import cls_corenetwork
from cls_ci_helper import TestCaseCtx
import cls_cmd

class TestCoreNetwork(unittest.TestCase):
    def setUp(self):
        self.ctx = TestCaseCtx.Default(tempfile.mkdtemp())
    def tearDown(self):
        with cls_cmd.LocalCmd() as c:
            c.run(f'rm -rf {self.ctx.logPath}')

    def test_simple_core(self):
        c = cls_corenetwork.CoreNetwork("test", filename="tests/config/test_core_infra.yaml")
        success, output = c.deploy()
        self.assertTrue(success)
        self.assertEqual(output, "deploy")
        # should not have it set in config
        self.assertTrue(c.runIperf3Server())
        self.assertEqual(c.getIP(), "127.0.0.1")
        log_files, output = c.undeploy(self.ctx)
        self.assertEqual(output, "undeploy")
        with cls_cmd.LocalCmd() as cmd:
            # there must be one log file for this (test) core
            self.assertEqual(len(log_files), 1)
            # undeploy uses archiveArtifact(), which writes to {prefix}-logs
            l = log_files[0]
            self.assertEqual(l, f"{self.ctx.baseFilename()}-logs")
            ret = cmd.run(f"cat {l}")
            self.assertEqual(ret.returncode, 0) # command must succeed
            self.assertEqual(ret.stdout, "logs") # output should be "logs"

    def test_core_list(self):
        c = cls_corenetwork.CoreNetwork("test_list", filename="tests/config/test_core_infra.yaml")
        success, _ = c.deploy()
        self.assertTrue(success)
        self.assertFalse(c.runIperf3Server())
        c.undeploy(None)

    def test_core_fail(self):
        c = cls_corenetwork.CoreNetwork("test_fail", filename="tests/config/test_core_infra.yaml")
        success, _ = c.deploy()
        self.assertFalse(success)
        # undeployment should still work
        c.undeploy(None)

    def test_core_script(self):
        c = cls_corenetwork.CoreNetwork("test_script", filename="tests/config/test_core_infra.yaml")
        success, output = c.deploy()
        self.assertTrue(success)
        self.assertEqual(output, "deployment from script")
        c.undeploy()

    def test_core_script_fail(self):
        c = cls_corenetwork.CoreNetwork("test_script_fail", filename="tests/config/test_core_infra.yaml")
        success, output = c.deploy()
        self.assertFalse(success)
        self.assertEqual(output, "deployment from script\nfailing")
        c.undeploy()

if __name__ == '__main__':
    unittest.main()
