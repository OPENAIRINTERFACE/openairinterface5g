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
import cls_containerize
from cls_ci_helper import GlobalTestCtx, TestCaseCtx
import cls_cmd

class TestDeploymentMethods(unittest.TestCase):
	def _pull_image(self, cmd, image):
		ret = cmd.run(f"docker inspect oai-ci/{image}:{self.tag}")
		if ret.returncode == 0: # exists
			return
		ret = cmd.run(f"docker pull oaisoftwarealliance/{image}:develop")
		self.assertEqual(ret.returncode, 0)
		ret = cmd.run(f"docker tag oaisoftwarealliance/{image}:develop oai-ci/{image}:{self.tag}")
		self.assertEqual(ret.returncode, 0)
		ret = cmd.run(f"docker rmi oaisoftwarealliance/{image}:develop")
		self.assertEqual(ret.returncode, 0)

	def setUp(self):
		self.tag = 'develop-12345678' # tag of images to deploy
		with cls_cmd.getConnection("localhost") as cmd:
			self._pull_image(cmd, "oai-gnb")
			self._pull_image(cmd, "oai-nr-ue")
		self.html = cls_oai_html.HTMLManagement()
		self.html.testCaseId = "000000"
		self.node = 'localhost'
		g = GlobalTestCtx(repository='', workspace=os.getcwd(), branch=self.tag, merge=True, targetBranch='')
		self.ctx = TestCaseCtx.Default(tempfile.mkdtemp(), g)
	def tearDown(self):
		with cls_cmd.LocalCmd() as c:
			c.run(f'rm -rf {self.ctx.logPath}')

	def test_deploy(self):
		opt = cls_containerize.DeploymentOpt('tests/simple-dep/', '', 3)
		deploy = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, opt)
		undeploy = cls_containerize.Containerize.UndeployObject(self.ctx, self.node, self.html, opt, [])
		self.assertTrue(deploy)
		self.assertTrue(undeploy)

	def test_stop_services(self):
		yaml_path = 'tests/simple-undep/'
		# should deploy both testA and testB
		deploy = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, cls_containerize.DeploymentOpt(yaml_path, '', 3))
		# should fail (no such service)
		stopC = cls_containerize.Containerize.StopObject(self.ctx, self.node, self.html, cls_containerize.DeploymentOpt(yaml_path, "testC"))
		# should stop testA
		stopA = cls_containerize.Containerize.StopObject(self.ctx, self.node, self.html, cls_containerize.DeploymentOpt(yaml_path, "testA"))
		# should (re-)stop testA (no-op)
		stopA2 = cls_containerize.Containerize.StopObject(self.ctx, self.node, self.html, cls_containerize.DeploymentOpt(yaml_path, "testA"))
		# should deploy testB
		stopB = cls_containerize.Containerize.StopObject(self.ctx, self.node, self.html, cls_containerize.DeploymentOpt(yaml_path, "testB"))
		# should not undeploy anything (everything already stopped)
		undeployAll = cls_containerize.Containerize.UndeployObject(self.ctx, self.node, self.html, cls_containerize.DeploymentOpt(yaml_path), [])
		self.assertTrue(deploy)
		self.assertFalse(stopC)
		self.assertTrue(stopA)
		self.assertTrue(stopA2)
		self.assertTrue(stopB)
		self.assertTrue(undeployAll)

	def test_deployfails(self):
		# fails reliably
		opt = cls_containerize.DeploymentOpt('tests/simple-fail/', '', 3)
		deploy = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, opt)
		cls_containerize.Containerize.UndeployObject(self.ctx, self.node, self.html, opt, [])
		self.assertFalse(deploy)

	def test_deployfails_2svc(self):
		# fails reliably
		opt = cls_containerize.DeploymentOpt('tests/simple-fail-2svc/', '', 3)
		deploy = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, opt)
		cls_containerize.Containerize.UndeployObject(self.ctx, self.node, self.html, opt, [])
		self.assertFalse(deploy)

	def test_deploy_ran(self):
		opt = cls_containerize.DeploymentOpt('yaml_files/5g_rfsimulator_tdd_dora', "oai-gnb", 3)
		deploy = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, opt)
		undeploy = cls_containerize.Containerize.UndeployObject(self.ctx, self.node, self.html, opt, [])
		self.assertTrue(deploy)
		self.assertTrue(undeploy)

	def test_deploy_multiran(self):
		opt = cls_containerize.DeploymentOpt('yaml_files/5g_rfsimulator_tdd_dora', "oai-gnb oai-nr-ue", 3)
		deploy = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, opt)
		undeploy = cls_containerize.Containerize.UndeployObject(self.ctx, self.node, self.html, opt, [])
		self.assertTrue(deploy)
		self.assertTrue(undeploy)

	def test_deploy_staged(self):
		yaml_path = 'yaml_files/5g_rfsimulator_tdd_dora'
		opt = cls_containerize.DeploymentOpt(yaml_path, "oai-gnb", 3)
		deploy1 = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, opt)
		opt = cls_containerize.DeploymentOpt(yaml_path, "oai-nr-ue", 3)
		deploy2 = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, opt)
		undeploy = cls_containerize.Containerize.UndeployObject(self.ctx, self.node, self.html, opt, [])
		self.assertTrue(deploy1)
		self.assertTrue(deploy2)
		self.assertTrue(undeploy)

	def test_create_workspace(self):
		workspace = tempfile.mkdtemp()
		repo = "https://github.com/duranta-project/openairinterface5g.git"
		g = GlobalTestCtx(repository=repo, workspace=workspace, branch="develop", merge=False, targetBranch='')
		ctx = TestCaseCtx.Default(self.ctx.logPath, g)
		ws = cls_containerize.Containerize.Create_Workspace(ctx, self.node, self.html)
		with cls_cmd.LocalCmd() as cmd:
			cmd.run(f"rm -rf {workspace}")
		self.assertTrue(ws)

	def test_undeploy_loganalysis(self):
		opt = cls_containerize.DeploymentOpt('yaml_files/5g_rfsimulator_tdd_dora', "oai-gnb", 3)
		deploy = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, opt)
		analyze = ["oai-gnb"]
		undeploy = cls_containerize.Containerize.UndeployObject(self.ctx, self.node, self.html, opt, analyze)
		self.assertTrue(deploy)
		self.assertTrue(undeploy)

	def test_undeploy_multi_loganalysis(self):
		opt = cls_containerize.DeploymentOpt('yaml_files/5g_rfsimulator_tdd_dora', "oai-gnb", 3)
		deploy = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, opt)
		analyze = ["oai-gnb", "oai-gnb=ContainsString=NOTFOUND"]
		undeploy = cls_containerize.Containerize.UndeployObject(self.ctx, self.node, self.html, opt, analyze)
		self.assertTrue(deploy)
		self.assertFalse(undeploy)

	# TODO this does not work: we don't evaluate return codes yet
	#def test_undeploy_rc_nonnull(self):
	#	# fails reliably
	#	opt = cls_containerize.DeploymentOpt('tests/simple-fail/', '', 3)
	#	deploy = cls_containerize.Containerize.DeployObject(self.ctx, self.node, self.html, opt)
	#	# should be false because wrong exit code
	#	undeploy = cls_containerize.Containerize.UndeployObject(self.ctx, self.node, self.html, opt, ["test"])
	#	self.assertFalse(deploy)
	#	self.assertFalse(undeploy)

if __name__ == '__main__':
	unittest.main()
