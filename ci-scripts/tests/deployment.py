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
import cls_oaicitest
import cls_containerize
from cls_ci_helper import GlobalTestCtx, TestCaseCtx
import cls_cmd

class TestDeploymentMethods(unittest.TestCase):
	def _pull_image(self, cmd, image):
		ret = cmd.run(f"docker inspect oai-ci/{image}:develop-12345678")
		if ret.returncode == 0: # exists
			return
		ret = cmd.run(f"docker pull oaisoftwarealliance/{image}:develop")
		self.assertEqual(ret.returncode, 0)
		ret = cmd.run(f"docker tag oaisoftwarealliance/{image}:develop oai-ci/{image}:develop-12345678")
		self.assertEqual(ret.returncode, 0)
		ret = cmd.run(f"docker rmi oaisoftwarealliance/{image}:develop")
		self.assertEqual(ret.returncode, 0)

	def setUp(self):
		with cls_cmd.getConnection("localhost") as cmd:
			self._pull_image(cmd, "oai-gnb")
			self._pull_image(cmd, "oai-nr-ue")
		self.html = cls_oai_html.HTMLManagement()
		self.html.testCaseId = "000000"
		self.ci = cls_oaicitest.OaiCiTest()
		self.cont = cls_containerize.Containerize()
		self.cont.yamlPath = ''
		self.cont.num_attempts = 3
		self.node = 'localhost'
		g = GlobalTestCtx(repository='', workspace=os.getcwd(), branch='', merge=True, targetBranch='')
		self.ctx = TestCaseCtx.Default(tempfile.mkdtemp(), g)
	def tearDown(self):
		with cls_cmd.LocalCmd() as c:
			c.run(f'rm -rf {self.ctx.logPath}')

	def test_deploy(self):
		self.cont.yamlPath = 'tests/simple-dep/'
		self.cont.deploymentTag = "noble"
		deploy = self.cont.DeployObject(self.ctx, self.node, self.html)
		undeploy = self.cont.UndeployObject(self.ctx, self.node, self.html, [])
		self.assertTrue(deploy)
		self.assertTrue(undeploy)

	def test_stop_services(self):
		self.cont.yamlPath = 'tests/simple-undep/'
		self.cont.deploymentTag = "noble"
		# should deploy both testA and testB
		deploy = self.cont.DeployObject(self.ctx, self.node, self.html)
		# should fail (no such service)
		self.cont.services = "testC"
		stopC = self.cont.StopObject(self.ctx, self.node, self.html)
		# should stop testA
		self.cont.services = "testA"
		stopA = self.cont.StopObject(self.ctx, self.node, self.html)
		# should (re-)stop testA (no-op)
		self.cont.services = "testA"
		stopA2 = self.cont.StopObject(self.ctx, self.node, self.html)
		# should deploy testB
		self.cont.services = "testB"
		stopB = self.cont.StopObject(self.ctx, self.node, self.html)
		# should not undeploy anything (everything already stopped)
		self.cont.services = None
		undeployAll = self.cont.UndeployObject(self.ctx, self.node, self.html, [])
		self.assertTrue(deploy)
		self.assertFalse(stopC)
		self.assertTrue(stopA)
		self.assertTrue(stopA2)
		self.assertTrue(stopB)
		self.assertTrue(undeployAll)

	def test_deployfails(self):
		# fails reliably
		old = self.cont.yamlPath
		self.cont.yamlPath = 'tests/simple-fail/'
		deploy = self.cont.DeployObject(self.ctx, self.node, self.html)
		self.cont.UndeployObject(self.ctx, self.node, self.html, [])
		self.assertFalse(deploy)
		self.cont.yamlPath = old

	def test_deployfails_2svc(self):
		# fails reliably
		old = self.cont.yamlPath
		self.cont.yamlPath = 'tests/simple-fail-2svc/'
		deploy = self.cont.DeployObject(self.ctx, self.node, self.html)
		self.cont.UndeployObject(self.ctx, self.node, self.html, [])
		self.assertFalse(deploy)
		self.cont.yamlPath = old

	def test_deploy_ran(self):
		self.cont.yamlPath = 'yaml_files/5g_rfsimulator_tdd_dora'
		self.cont.services = "oai-gnb"
		self.cont.deploymentTag = 'develop-12345678'
		deploy = self.cont.DeployObject(self.ctx, self.node, self.html)
		undeploy = self.cont.UndeployObject(self.ctx, self.node, self.html, [])
		self.assertTrue(deploy)
		self.assertTrue(undeploy)

	def test_deploy_multiran(self):
		self.cont.yamlPath = 'yaml_files/5g_rfsimulator_tdd_dora'
		self.cont.services = "oai-gnb oai-nr-ue"
		self.cont.deploymentTag = 'develop-12345678'
		deploy = self.cont.DeployObject(self.ctx, self.node, self.html)
		undeploy = self.cont.UndeployObject(self.ctx, self.node, self.html, [])
		self.assertTrue(deploy)
		self.assertTrue(undeploy)

	def test_deploy_staged(self):
		self.cont.yamlPath = 'yaml_files/5g_rfsimulator_tdd_dora'
		self.cont.services = "oai-gnb"
		self.cont.deploymentTag = 'develop-12345678'
		deploy1 = self.cont.DeployObject(self.ctx, self.node, self.html)
		self.cont.services = "oai-nr-ue"
		deploy2 = self.cont.DeployObject(self.ctx, self.node, self.html)
		undeploy = self.cont.UndeployObject(self.ctx, self.node, self.html, [])
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
		self.cont.yamlPath = 'yaml_files/5g_rfsimulator_tdd_dora'
		self.cont.services = "oai-gnb"
		self.cont.deploymentTag = 'develop-12345678'
		deploy = self.cont.DeployObject(self.ctx, self.node, self.html)
		analyze = ["oai-gnb"]
		undeploy = self.cont.UndeployObject(self.ctx, self.node, self.html, analyze)
		self.assertTrue(deploy)
		self.assertTrue(undeploy)

	def test_undeploy_multi_loganalysis(self):
		self.cont.yamlPath = 'yaml_files/5g_rfsimulator_tdd_dora'
		self.cont.services = "oai-gnb"
		self.cont.deploymentTag = 'develop-12345678'
		deploy = self.cont.DeployObject(self.ctx, self.node, self.html)
		analyze = ["oai-gnb", "oai-gnb=ContainsString=NOTFOUND"]
		undeploy = self.cont.UndeployObject(self.ctx, self.node, self.html, analyze)
		self.assertTrue(deploy)
		self.assertFalse(undeploy)

	# TODO this does not work: we don't evaluate return codes yet
	#def test_undeploy_rc_nonnull(self):
	#	# fails reliably
	#	old = self.cont.yamlPath
	#	self.cont.yamlPath = 'tests/simple-fail/'
	#	deploy = self.cont.DeployObject(self.ctx, self.node, self.html)
	#	# should be false because wrong exit code
	#	undeploy = self.cont.UndeployObject(self.ctx, self.node, self.html, ["test"])
	#	self.assertFalse(deploy)
	#	self.assertFalse(undeploy)
	#	self.cont.yamlPath = old

if __name__ == '__main__':
	unittest.main()
