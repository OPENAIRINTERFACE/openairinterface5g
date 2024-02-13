#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */
#---------------------------------------------------------------------
#
#   Required Python Version
#     Python 3.x
#
#---------------------------------------------------------------------

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------
import logging
import re
import time

import cls_oai_html
import constants as CONST
import helpreadme as HELP
import cls_containerize
import cls_cmd

IMAGE_REGISTRY_SERVICE_NAME = "image-registry.openshift-image-registry.svc"
NAMESPACE = "oaicicd-ran"
OCUrl = "https://api.oai.cs.eurecom.fr:6443"
OCRegistry = "default-route-openshift-image-registry.apps.oai.cs.eurecom.fr/"
CI_OC_RAN_NAMESPACE = "oaicicd-ran"
CI_OC_CORE_NAMESPACE = "oaicicd-core-for-ci-ran"

def OC_login(cmd, ocUserName, ocPassword, ocProjectName):
	if ocUserName == '' or ocPassword == '' or ocProjectName == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter: no OC Credentials')
	if OCRegistry.startswith("http") and not self.OCRegistry.endswith("/"):
		sys.exit(f'ocRegistry {OCRegistry} should not start with http:// or https:// and end on a slash /')
	ret = cmd.run(f'oc login -u {ocUserName} -p {ocPassword} --server {OCUrl}')
	if ret.returncode != 0:
		logging.error('\u001B[1m OC Cluster Login Failed\u001B[0m')
		return False
	ret = cmd.run(f'oc project {ocProjectName}')
	if ret.returncode != 0:
		logging.error(f'\u001B[1mUnable to access OC project {ocProjectName}\u001B[0m')
		OC_logout(cmd)
		return False
	return True

def OC_logout(cmd):
	cmd.run(f'oc logout')

class Cluster:
	def __init__(self):
		self.eNBIPAddress = ""
		self.eNBSourceCodePath = ""
		self.forcedWorkspaceCleanup = False
		self.OCUserName = ""
		self.OCPassword = ""
		self.OCProjectName = ""
		self.OCUrl = "https://api.oai.cs.eurecom.fr:6443"
		self.OCRegistry = "default-route-openshift-image-registry.apps.oai.cs.eurecom.fr/"
		self.ranRepository = ""
		self.ranBranch = ""
		self.ranCommitID = ""
		self.ranAllowMerge = False
		self.ranTargetBranch = ""
		self.cmd = None
		self.imageToPull = ''
		self.testSvrId = None

	def _recreate_entitlements(self):
		# recreating entitlements, don't care if deletion fails
		self.cmd.run('oc delete secret etc-pki-entitlement')
		ret = self.cmd.run('ls /etc/pki/entitlement/???????????????????.pem | tail -1', silent=True)
		regres1 = re.search(r"/etc/pki/entitlement/[0-9]+.pem", ret.stdout)
		ret = self.cmd.run('ls /etc/pki/entitlement/???????????????????-key.pem | tail -1', silent=True)
		regres2 = re.search(r"/etc/pki/entitlement/[0-9]+-key.pem", ret.stdout)
		if regres1 is None or regres2 is None:
			logging.error("could not find entitlements")
			return False
		file1 = regres1.group(0)
		file2 = regres2.group(0)
		ret = self.cmd.run(f'oc create secret generic etc-pki-entitlement --from-file {file1} --from-file {file2}')
		regres = re.search(r"secret/etc-pki-entitlement created", ret.stdout)
		if ret.returncode != 0 or regres is None:
			logging.error("could not create secret/etc-pki-entitlement")
			return False
		return True

	def _recreate_bc(self, name, newTag, filename):
		self._retag_image_statement(name, name, newTag, filename)
		self.cmd.run(f'oc delete -f {filename}')
		ret = self.cmd.run(f'oc create -f {filename}')
		if re.search('buildconfig.build.openshift.io/[a-zA-Z\-0-9]+ created', ret.stdout) is not None:
			return True
		logging.error('error while creating buildconfig: ' + ret.stdout)
		return False

	def _recreate_is_tag(self, name, newTag, filename):
		ret = self.cmd.run(f'oc describe is {name}')
		if ret.returncode != 0:
			ret = self.cmd.run(f'oc create -f {filename}')
			if ret.returncode != 0:
				logging.error(f'error while creating imagestream: {ret.stdout}')
				return False
		else:
			logging.debug(f'-> imagestream {name} found')
		image = f'{name}:{newTag}'
		self.cmd.run(f'oc delete istag {image}', reportNonZero=False) # we don't care if this fails, e.g., if it is missing
		ret = self.cmd.run(f'oc create istag {image}')
		if ret.returncode == 0:
			return True
		logging.error(f'error while creating imagestreamtag: {ret.stdout}')
		return False

	def _start_build(self, name):
		# will return "immediately" but build runs in background
		# if multiple builds are started at the same time, this can take some time, however
		ret = self.cmd.run(f'oc start-build {name} --from-dir={self.eNBSourceCodePath} --exclude=""')
		regres = re.search(r'build.build.openshift.io/(?P<jobname>[a-zA-Z0-9\-]+) started', ret.stdout)
		if ret.returncode != 0 or ret.stdout.count('Uploading finished') != 1 or regres is None:
			logging.error(f"error during oc start-build: {ret.stdout}")
			return None
		return regres.group('jobname') + '-build'

	def _wait_build_end(self, jobs, timeout_sec, check_interval_sec = 5):
		logging.debug(f"waiting for jobs {jobs} to finish building")
		while timeout_sec > 0:
			# check status
			for j in jobs:
				ret = self.cmd.run(f'oc get pods | grep {j}', silent = True)
				if ret.stdout.count('Completed') > 0: jobs.remove(j)
				if ret.stdout.count('Error') > 0:
					logging.error(f'error for job {j}: {ret.stdout}')
					return False
			if jobs == []:
				logging.debug('all jobs completed')
				return True
			time.sleep(check_interval_sec)
			timeout_sec -= check_interval_sec
		logging.error(f"timeout while waiting for end of build of {jobs}")
		return False

	def _retag_image_statement(self, oldImage, newImage, newTag, filename):
		self.cmd.run(f'sed -i -e "s#{oldImage}:latest#{newImage}:{newTag}#" {filename}')

	def _get_image_size(self, image, tag):
		# get the SHA of the image we built using the image name and its tag
		ret = self.cmd.run(f'oc describe is {image} | grep -A4 {tag}')
		result = re.search(f'{IMAGE_REGISTRY_SERVICE_NAME}:5000/{NAMESPACE}/(?P<imageSha>{image}@sha256:[a-f0-9]+)', ret.stdout)
		if result is None:
			return -1
		imageSha = result.group("imageSha")

		# retrieve the size
		ret = self.cmd.run(f'oc get -o json isimage {imageSha} | jq -Mc "{{dockerImageSize: .image.dockerImageMetadata.Size}}"')
		result = re.search('{"dockerImageSize":(?P<size>[0-9]+)}', ret.stdout)
		if result is None:
			return -1
		return int(result.group("size"))

	def _deploy_pod(self, filename, timeout = 30):
		ret = self.cmd.run(f'oc create -f {filename}')
		result = re.search(f'pod/(?P<pod>[a-zA-Z0-9_\-]+) created', ret.stdout)
		if result is None:
			logging.error(f'could not deploy pod: {ret.stdout}')
			return None
		pod = result.group("pod")
		logging.debug(f'checking if pod {pod} is in Running state')
		while timeout > 0:
			ret = self.cmd.run(f'oc get pod {pod} -o json | jq -Mc .status.phase', silent=True)
			if re.search('"Running"', ret.stdout) is not None: return pod
			timeout -= 1
			time.sleep(1)
		logging.error(f'pod {pod} did not reach Running state')
		self._undeploy_pod(filename)
		return None

	def _undeploy_pod(self, filename):
		self.cmd.run(f'oc delete -f {filename}')

	def PullClusterImage(self, HTML, RAN):
		if self.testSvrId == None: self.testSvrId = self.eNBIPAddress
		if self.imageToPull == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		logging.debug(f'Pull OC image {self.imageToPull} to server {self.testSvrId}')
		self.testCase_id = HTML.testCase_id
		cmd = cls_cmd.getConnection(self.testSvrId)
		succeeded = OC_login(cmd, self.OCUserName, self.OCPassword, CI_OC_RAN_NAMESPACE)
		if not succeeded:
			logging.error('\u001B[1m OC Cluster Login Failed\u001B[0m')
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
			return False
		ret = cmd.run(f'oc whoami -t | docker login -u oaicicd --password-stdin {self.OCRegistry}')
		if ret.returncode != 0:
			logging.error(f'\u001B[1m Unable to access OC project {CI_OC_RAN_NAMESPACE}\u001B[0m')
			OC_logout(cmd)
			cmd.close()
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
			return False
		for image in self.imageToPull:
			imagePrefix = f'{self.OCRegistry}{CI_OC_RAN_NAMESPACE}'
			imageTag = cls_containerize.ImageTagToUse(image, self.ranCommitID, self.ranBranch, self.ranAllowMerge)
			ret = cmd.run(f'docker pull {imagePrefix}/{imageTag}')
			if ret.returncode != 0:
				logging.error(f'Could not pull {image} from local registry : {self.OCRegistry}')
				OC_logout(cmd)
				cmd.close()
				HTML.CreateHtmlTestRow('msg', 'KO', CONST.ALL_PROCESSES_OK)
				return False
			cmd.run(f'docker tag {imagePrefix}/{imageTag} oai-ci/{imageTag}')
			cmd.run(f'docker rmi {imagePrefix}/{imageTag}')
		OC_logout(cmd)
		cmd.close()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		return True

	def BuildClusterImage(self, HTML):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit(f'Insufficient Parameter: ranRepository {self.ranRepository} ranBranch {ranBranch} ranCommitID {self.ranCommitID}')
		lIpAddr = self.eNBIPAddress
		lSourcePath = self.eNBSourceCodePath
		if lIpAddr == '' or lSourcePath == '':
			sys.exit('Insufficient Parameter: eNBSourceCodePath missing')
		ocUserName = self.OCUserName
		ocPassword = self.OCPassword
		ocProjectName = self.OCProjectName
		if ocUserName == '' or ocPassword == '' or ocProjectName == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter: no OC Credentials')
		if self.OCRegistry.startswith("http") and not self.OCRegistry.endswith("/"):
			sys.exit(f'ocRegistry {self.OCRegistry} should not start with http:// or https:// and end on a slash /')

		logging.debug(f'Building on cluster triggered from server: {lIpAddr}')
		self.cmd = cls_cmd.RemoteCmd(lIpAddr)

		self.testCase_id = HTML.testCase_id

		# Workaround for some servers, we need to erase completely the workspace
		if self.forcedWorkspaceCleanup:
			self.cmd.run(f'rm -Rf {lSourcePath}')
		cls_containerize.CreateWorkspace(self.cmd, lSourcePath, self.ranRepository, self.ranCommitID, self.ranTargetBranch, self.ranAllowMerge)

		# to reduce the amount of data send to OpenShift, we
		# manually delete all generated files in the workspace
		self.cmd.run(f'rm -rf {lSourcePath}/cmake_targets/ran_build');

		baseTag = 'develop'
		forceBaseImageBuild = False
		if self.ranAllowMerge: # merging MR branch into develop -> temporary image
			branchName = self.ranBranch.replace('/','-')
			imageTag = f'{branchName}-{self.ranCommitID[0:8]}'
			if self.ranTargetBranch == 'develop':
				ret = self.cmd.run(f'git diff HEAD..origin/develop -- cmake_targets/build_oai cmake_targets/tools/build_helper docker/Dockerfile.base.rhel9 | grep --colour=never -i INDEX')
				result = re.search('index', ret.stdout)
				if result is not None:
					forceBaseImageBuild = True
					baseTag = 'ci-temp'
			# if the branch name contains integration_20xx_wyy, let rebuild ran-base
			result = re.search('integration_20([0-9]{2})_w([0-9]{2})', self.ranBranch)
			if not forceBaseImageBuild and result is not None:
				forceBaseImageBuild = True
				baseTag = 'ci-temp'
		else:
			imageTag = f'develop-{self.ranCommitID[0:8]}'
			forceBaseImageBuild = True

		# logging to OC Cluster and then switch to corresponding project
		ret = self.cmd.run(f'oc login -u {ocUserName} -p {ocPassword} --server {self.OCUrl}')
		if ret.returncode != 0:
			logging.error('\u001B[1m OC Cluster Login Failed\u001B[0m')
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
			return False

		ret = self.cmd.run(f'oc project {ocProjectName}')
		if ret.returncode != 0:
			logging.error(f'\u001B[1mUnable to access OC project {ocProjectName}\u001B[0m')
			self.cmd.run('oc logout')
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_PROJECT_FAIL)
			return False

		# delete old images by Sagar Arora <sagar.arora@openairinterface.org>:
		# 1. retrieve all images and their timestamp
		# 2. awk retrieves those whose timestamp is older than 4 weeks
		# 3. issue delete command on corresponding istags (the images are dangling and will be cleaned by the registry)
		delete_cmd = "oc get istag -o go-template --template '{{range .items}}{{.metadata.name}} {{.metadata.creationTimestamp}}{{\"\\n\"}}{{end}}' | awk '$2 <= \"'$(date -d '-4weeks' -Ins --utc | sed 's/+0000/Z/')'\" { print $1 }' | xargs --no-run-if-empty oc delete istag"
		response = self.cmd.run(delete_cmd)
		logging.debug(f"deleted images:\n{response.stdout}")

		self._recreate_entitlements()

		status = True # flag to abandon compiling if any image fails
		attemptedImages = []
		if forceBaseImageBuild:
			self._recreate_is_tag('ran-base', baseTag, 'openshift/ran-base-is.yaml')
			self._recreate_bc('ran-base', baseTag, 'openshift/ran-base-bc.yaml')
			ranbase_job = self._start_build('ran-base')
			attemptedImages += ['ran-base']
			status = ranbase_job is not None and self._wait_build_end([ranbase_job], 800)
			if not status: logging.error('failure during build of ran-base')
			self.cmd.run(f'oc logs {ranbase_job} &> cmake_targets/log/ran-base.log') # cannot use cmd.run because of redirect
			# recover logs by mounting image
			self._retag_image_statement('ran-base', 'ran-base', baseTag, 'openshift/ran-base-log-retrieval.yaml')
			pod = self._deploy_pod('openshift/ran-base-log-retrieval.yaml')
			if pod is not None:
				self.cmd.run(f'mkdir -p cmake_targets/log/ran-base')
				self.cmd.run(f'oc rsync {pod}:/oai-ran/cmake_targets/log/ cmake_targets/log/ran-base')
				self._undeploy_pod('openshift/ran-base-log-retrieval.yaml')
			else:
				status = False

		if status:
			self._recreate_is_tag('oai-physim', imageTag, 'openshift/oai-physim-is.yaml')
			self._recreate_bc('oai-physim', imageTag, 'openshift/oai-physim-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.phySim.rhel9')
			physim_job = self._start_build('oai-physim')
			attemptedImages += ['oai-physim']

			self._recreate_is_tag('ran-build', imageTag, 'openshift/ran-build-is.yaml')
			self._recreate_bc('ran-build', imageTag, 'openshift/ran-build-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.build.rhel9')
			ranbuild_job = self._start_build('ran-build')
			attemptedImages += ['ran-build']

			self._recreate_is_tag('oai-clang', imageTag, 'openshift/oai-clang-is.yaml')
			self._recreate_bc('oai-clang', imageTag, 'openshift/oai-clang-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.clang.rhel9')
			clang_job = self._start_build('oai-clang')
			attemptedImages += ['oai-clang']

			wait = ranbuild_job is not None and physim_job is not None and clang_job is not None and self._wait_build_end([ranbuild_job, physim_job, clang_job], 1200)
			if not wait: logging.error('error during build of ranbuild_job or physim_job or clang_job')
			status = status and wait
			self.cmd.run(f'oc logs {ranbuild_job} &> cmake_targets/log/ran-build.log')
			self.cmd.run(f'oc logs {physim_job} &> cmake_targets/log/oai-physim.log')
			self.cmd.run(f'oc logs {clang_job} &> cmake_targets/log/oai-clang.log')
			self.cmd.run(f'oc get pods.metrics.k8s.io &>> cmake_targets/log/build-metrics.log', '\$', 10)

		if status:
			self._recreate_is_tag('oai-enb', imageTag, 'openshift/oai-enb-is.yaml')
			self._recreate_bc('oai-enb', imageTag, 'openshift/oai-enb-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.eNB.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.eNB.rhel9')
			enb_job = self._start_build('oai-enb')
			attemptedImages += ['oai-enb']

			self._recreate_is_tag('oai-gnb', imageTag, 'openshift/oai-gnb-is.yaml')
			self._recreate_bc('oai-gnb', imageTag, 'openshift/oai-gnb-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.gNB.rhel9')
			gnb_job = self._start_build('oai-gnb')
			attemptedImages += ['oai-gnb']

			self._recreate_is_tag('oai-gnb-aw2s', imageTag, 'openshift/oai-gnb-aw2s-is.yaml')
			self._recreate_bc('oai-gnb-aw2s', imageTag, 'openshift/oai-gnb-aw2s-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.aw2s.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.gNB.aw2s.rhel9')
			gnb_aw2s_job = self._start_build('oai-gnb-aw2s')
			attemptedImages += ['oai-gnb-aw2s']

			wait = enb_job is not None and gnb_job is not None and gnb_aw2s_job is not None and self._wait_build_end([enb_job, gnb_job, gnb_aw2s_job], 600)
			if not wait: logging.error('error during build of eNB/gNB')
			status = status and wait
			# recover logs
			self.cmd.run(f'oc logs {enb_job} &> cmake_targets/log/oai-enb.log')
			self.cmd.run(f'oc logs {gnb_job} &> cmake_targets/log/oai-gnb.log')
			self.cmd.run(f'oc logs {gnb_aw2s_job} &> cmake_targets/log/oai-gnb-aw2s.log')

			self._recreate_is_tag('oai-nr-cuup', imageTag, 'openshift/oai-nr-cuup-is.yaml')
			self._recreate_bc('oai-nr-cuup', imageTag, 'openshift/oai-nr-cuup-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.nr-cuup.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.nr-cuup.rhel9')
			nr_cuup_job = self._start_build('oai-nr-cuup')
			attemptedImages += ['oai-nr-cuup']

			self._recreate_is_tag('oai-lte-ue', imageTag, 'openshift/oai-lte-ue-is.yaml')
			self._recreate_bc('oai-lte-ue', imageTag, 'openshift/oai-lte-ue-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.lteUE.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.lteUE.rhel9')
			lteue_job = self._start_build('oai-lte-ue')
			attemptedImages += ['oai-lte-ue']

			self._recreate_is_tag('oai-nr-ue', imageTag, 'openshift/oai-nr-ue-is.yaml')
			self._recreate_bc('oai-nr-ue', imageTag, 'openshift/oai-nr-ue-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.nrUE.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.nrUE.rhel9')
			nrue_job = self._start_build('oai-nr-ue')
			attemptedImages += ['oai-nr-ue']

			wait = nr_cuup_job is not None and lteue_job is not None and nrue_job is not None and self._wait_build_end([nr_cuup_job, lteue_job, nrue_job], 600)
			if not wait: logging.error('error during build of nr-cuup/lteUE/nrUE')
			status = status and wait
			# recover logs
			self.cmd.run(f'oc logs {nr_cuup_job} &> cmake_targets/log/oai-nr-cuup.log')
			self.cmd.run(f'oc logs {lteue_job} &> cmake_targets/log/oai-lte-ue.log')
			self.cmd.run(f'oc logs {nrue_job} &> cmake_targets/log/oai-nr-ue.log')
			self.cmd.run(f'oc get pods.metrics.k8s.io &>> cmake_targets/log/build-metrics.log', '\$', 10)

		# split and analyze logs
		imageSize = {}
		for image in attemptedImages:
			self.cmd.run(f'mkdir -p cmake_targets/log/{image}')
			tag = imageTag if image != 'ran-base' else baseTag
			size = self._get_image_size(image, tag)
			if size <= 0:
				imageSize[image] = 'unknown -- BUILD FAILED'
				status = False
			else:
				sizeMb = float(size) / 1000000
				imageSize[image] = f'{sizeMb:.1f} Mbytes (uncompressed: ~{sizeMb*2.5:.1f} Mbytes)'
			logging.info(f'\u001B[1m{image} size is {imageSize[image]}\u001B[0m')

		grep_exp = "\|".join(attemptedImages)
		self.cmd.run(f'oc get images | grep -e \'{grep_exp}\' &> cmake_targets/log/image_registry.log');
		self.cmd.run(f'for pod in $(oc get pods | tail -n +2 | awk \'{{print $1}}\'); do oc get pod $pod -o json &>> cmake_targets/log/build_pod_summary.log; done', '\$', 60)

		build_log_name = f'build_log_{self.testCase_id}'
		cls_containerize.CopyLogsToExecutor(self.cmd, lSourcePath, build_log_name)

		self.cmd.run('for pod in $(oc get pods | tail -n +2 | awk \'{print $1}\'); do oc delete pod ${pod}; done')

		# logout will return eventually, but we don't care when -> start in background
		self.cmd.run(f'oc logout')
		self.cmd.close()

		# Analyze the logs
		collectInfo = cls_containerize.AnalyzeBuildLogs(build_log_name, attemptedImages, status)
		for img in collectInfo:
			for f in collectInfo[img]:
				status = status and collectInfo[img][f]['status']
		if not status:
			logging.debug(collectInfo)

		if status:
			logging.info('\u001B[1m Building OAI Image(s) Pass\u001B[0m')
			HTML.CreateHtmlTestRow('all', 'OK', CONST.ALL_PROCESSES_OK)
		else:
			logging.error('\u001B[1m Building OAI Images Failed\u001B[0m')
			HTML.CreateHtmlTestRow('all', 'KO', CONST.ALL_PROCESSES_OK)

		HTML.CreateHtmlNextTabHeaderTestRow(collectInfo, imageSize)

		return status
