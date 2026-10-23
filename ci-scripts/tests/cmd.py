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
import cls_cmd

class TestCmd(unittest.TestCase):
    def test_local_cmd(self):
        with cls_cmd.getConnection("localhost") as cmd:
            self.assertTrue(isinstance(cmd, cls_cmd.LocalCmd))
        with cls_cmd.getConnection("None") as cmd:
            self.assertTrue(isinstance(cmd, cls_cmd.LocalCmd))
        with cls_cmd.getConnection("none") as cmd:
            self.assertTrue(isinstance(cmd, cls_cmd.LocalCmd))
        with cls_cmd.getConnection(None) as cmd:
            self.assertTrue(isinstance(cmd, cls_cmd.LocalCmd))
            ret = cmd.run("true")
            self.assertEqual(ret.args, "true")
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout, "")
            ret = cmd.run("false")
            self.assertEqual(ret.args, "false")
            self.assertEqual(ret.returncode, 1)
            self.assertEqual(ret.stdout, "")
            ret = cmd.run("echo test")
            self.assertEqual(ret.args, "echo test")
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout, "test")

    def test_local_script(self):
        with cls_cmd.getConnection("localhost") as ssh:
            ret = ssh.exec_script("tests/scripts/hello-world.sh", 1)
        self.assertEqual(ret.args, "tests/scripts/hello-world.sh")
        self.assertEqual(ret.returncode, 0)
        self.assertEqual(ret.stdout, "+ echo hello, world\nhello, world")

        with cls_cmd.getConnection("localhost") as ssh:
            ret = ssh.exec_script("tests/scripts/hello-fail.sh", 1, "TESTFAIL")
        self.assertEqual(ret.args, "tests/scripts/hello-fail.sh TESTFAIL")
        self.assertEqual(ret.returncode, 1)
        self.assertEqual(ret.stdout, "TESTFAIL")

        with cls_cmd.getConnection("localhost") as ssh:
            ret = ssh.exec_script("tests/scripts/hello-fail.sh", 1, "TESTFAIL2", "/tmp/result")
        self.assertEqual(ret.args, "tests/scripts/hello-fail.sh TESTFAIL2 &> /tmp/result")
        self.assertEqual(ret.returncode, 1)
        self.assertEqual(ret.stdout, "")
        with cls_cmd.getConnection("localhost") as ssh:
            ret = ssh.run("cat /tmp/result")
            self.assertEqual(ret.args, "cat /tmp/result")
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout, "TESTFAIL2")

    @unittest.skip("need to be able to passwordlessly SSH to localhost, also disable stty -ixon")
    def test_remote_cmd(self):
        with cls_cmd.getConnection("127.0.0.1") as cmd:
            self.assertTrue(isinstance(cmd, cls_cmd.RemoteCmd))
            ret = cmd.run("true")
            self.assertEqual(ret.args, "true")
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout, "")
            ret = cmd.run("false")
            self.assertEqual(ret.args, "false")
            self.assertEqual(ret.returncode, 1)
            self.assertEqual(ret.stdout, "")
            ret = cmd.run("echo test")
            self.assertEqual(ret.args, "echo test")
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout, "test")

    @unittest.skip("need to be able to passwordlessly SSH to localhost, also disable stty -ixon")
    def test_remote_script(self):
        with cls_cmd.getConnection("127.0.0.1") as ssh:
            ret = ssh.exec_script("tests/scripts/hello-world.sh", 1)
        self.assertEqual(ret.args, "tests/scripts/hello-world.sh")
        self.assertEqual(ret.returncode, 0)
        self.assertEqual(ret.stdout, "+ echo hello, world\nhello, world")

        with cls_cmd.getConnection("127.0.0.1") as ssh:
            ret = ssh.exec_script("tests/scripts/hello-fail.sh", 1, "TESTFAIL")
        self.assertEqual(ret.args, "tests/scripts/hello-fail.sh TESTFAIL")
        self.assertEqual(ret.returncode, 1)
        self.assertEqual(ret.stdout, "TESTFAIL")

        with cls_cmd.getConnection("127.0.0.1") as ssh:
            ret = ssh.exec_script("tests/scripts/hello-fail.sh", 1, "TESTFAIL2", "/tmp/result")
        self.assertEqual(ret.args, "tests/scripts/hello-fail.sh TESTFAIL2 &> /tmp/result")
        self.assertEqual(ret.returncode, 1)
        self.assertEqual(ret.stdout, "")
        with cls_cmd.getConnection("localhost") as ssh:
            ret = ssh.run("cat /tmp/result")
            self.assertEqual(ret.args, "cat /tmp/result")
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout, "TESTFAIL2")

if __name__ == '__main__':
	unittest.main()
