# SPDX-License-Identifier: LicenseRef-CSSL-1.0

import sys
import logging
logging.basicConfig(
	level=logging.DEBUG,
	stream=sys.stdout,
	format="[%(asctime)s] %(levelname)8s: %(message)s"
)
import os
import tempfile
os.system(f'rm -rf cmake_targets')
os.system(f'mkdir -p cmake_targets/log')
import unittest

sys.path.append('./') # to find OAI imports below
import cls_oai_html
import cls_cmd
import cls_containerize
from cls_ci_helper import TestCaseCtx

class TestDeploymentMethods(unittest.TestCase):
	def setUp(self):
		self.html = cls_oai_html.HTMLManagement()
		self.html.testCaseId = "000000"
		self.ctx = TestCaseCtx.Default(tempfile.mkdtemp())

	def tearDown(self):
		with cls_cmd.LocalCmd() as c:
			c.run(f'rm -rf {self.ctx.logPath}')

	def test_pull_clean_local_reg(self):
		# the pull function has the authentication at the internal cluster hardcoded
		# this is a refactoring opportunity: we should do it in a separate step
		# and allow to have pull work with any registry
		registry = cls_containerize.DEFAULT_REGISTRY
		with cls_cmd.getConnection("localhost") as cmd:
			ret = cmd.run(f"ping -c1 -w1 {registry}")
			if ret.returncode != 0: # could not ping once -> skip test
				self.skipTest(f"test_pull_clean_local_reg: could not reach {registry} (run inside sboai)")
		node = 'localhost'
		images = ["oai-gnb"]
		tag = "develop"
		pull = cls_containerize.Containerize.Pull_Image_from_Registry(self.ctx, self.html, node, images, tag=tag)
		clean = cls_containerize.Containerize.Clean_Test_Server_Images(self.ctx, self.html, node, images, tag=tag)
		self.assertTrue(pull)
		self.assertTrue(clean)

	def test_pull_clean_docker_hub(self):
		node = 'localhost'
		r = "docker.io"
		images = ["hello-world"]
		tag = "latest"
		pull = cls_containerize.Containerize.Pull_Image_from_Registry(self.ctx, self.html, node, images, tag=tag, registry=r, username=None, password=None)
		clean = cls_containerize.Containerize.Clean_Test_Server_Images(self.ctx, self.html, node, images, tag=tag)
		self.assertTrue(pull)
		self.assertTrue(clean)

if __name__ == '__main__':unittest.main()
