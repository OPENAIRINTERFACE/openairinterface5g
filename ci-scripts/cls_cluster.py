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
# Python for CI of OAI-eNB + COTS-UE
#
#   Required Python Version
#     Python 3.x
#
#   Required Python Package
#     pexpect
#---------------------------------------------------------------------

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------
import logging
import sshconnection as SSH
import cls_oai_html
#import os
import re
import time
#import subprocess
import sys
import constants as CONST
import helpreadme as HELP
import cls_containerize

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

	def _recreate_entitlements(self, sshSession):
		# recreating entitlements, don't care if deletion fails
		sshSession.command('oc delete secret etc-pki-entitlement', '\$', 5)
		if re.search(r"not found", sshSession.getBefore()):
			logging.warning("no secrets etc-pki-entitlement found, recreating")
		sshSession.command('ls /etc/pki/entitlement/???????????????????.pem | tail -1', '\$', 5, silent=True)
		regres1 = re.search(r"/etc/pki/entitlement/[0-9]+.pem", sshSession.getBefore())
		sshSession.command('ls /etc/pki/entitlement/???????????????????-key.pem | tail -1', '\$', 5, silent=True)
		regres2 = re.search(r"/etc/pki/entitlement/[0-9]+-key.pem", sshSession.getBefore())
		if regres1 is None or regres2 is None:
			logging.error("could not find entitlements")
			return false
		file1 = regres1.group(0)
		file2 = regres2.group(0)
		sshSession.command(f'oc create secret generic etc-pki-entitlement --from-file {file1} --from-file {file2}', '\$', 5)
		regres = re.search(r"secret/etc-pki-entitlement created", sshSession.getBefore())
		if regres is None:
			logging.error("could not create secret/etc-pki-entitlement")
			return False
		return True

	def _recreate_bc(self, sshSession, name, newTag, filename):
		self._retag_image_statement(sshSession, name, name, newTag, filename)
		sshSession.command(f'oc delete -f {filename}', '\$', 5)
		sshSession.command(f'oc create -f {filename}', '\$', 5)
		before = sshSession.getBefore()
		if re.search('buildconfig.build.openshift.io/[a-zA-Z\-0-9]+ created', before) is not None:
			return True
		logging.error('error while creating buildconfig: ' + sshSession.getBefore())
		return False

	def _recreate_is_tag(self, sshSession, name, newTag, filename):
		sshSession.command(f'oc describe is {name}', '\$', 5)
		if sshSession.getBefore().count('NotFound') > 0:
			sshSession.command(f'oc create -f {filename}', '\$', 5)
			before = sshSession.getBefore()
			if re.search(f'imagestream.image.openshift.io/{name} created', before) is None:
				logging.error('error while creating imagestream: ' + sshSession.getBefore())
				return False
		else:
			logging.debug(f'-> imagestream {name} found')
		image = f'{name}:{newTag}'
		sshSession.command(f'oc delete istag {image}', '\$', 5) # we don't care if this fails, e.g., if it is missing
		sshSession.command(f'oc create istag {image}', '\$', 5)
		before = sshSession.getBefore()
		if re.search(f'imagestreamtag.image.openshift.io/{image} created', before) is not None:
			return True
		logging.error('error while creating imagestreamtag: ' + sshSession.getBefore())
		return False

	def _start_build(self, sshSession, name):
		# will return "immediately" but build runs in background
		# if multiple builds are started at the same time, this can take some time, however
		ret = sshSession.command(f'oc start-build {name} --from-file={self.eNBSourceCodePath}', '\$', 300)
		before = sshSession.getBefore()
		regres = re.search(r'build.build.openshift.io/(?P<jobname>[a-zA-Z0-9\-]+) started', str(before))
		if ret != 0 or before.count('Uploading finished') != 1 or regres is None:
			logging.error("error during oc start-build: " + sshSession.getBefore())
			self._delete_pod(sshSession, name)
			return None
		return regres.group('jobname') + '-build'

	def _delete_pod(self, sshSession, shortName):
		sshSession.command(f"oc get pods | grep {shortName}", '\$', 5)
		regres = re.search(rf'{shortName}-[0-9]+-build', sshSession.getBefore())
		if regres is not None:
			sshSession.command(f"oc delete pod {regres.group(0)}", '\$', 5)
		else:
			logging.warning(f"no pod found with name {shortName}")

	def _wait_build_end(self, sshSession, jobs, timeout_sec, check_interval_sec = 5):
		logging.debug(f"waiting for jobs {jobs} to finish building")
		while timeout_sec > 0:
			# check status
			for j in jobs:
				sshSession.command(f'oc get pods | grep {j}', '\$', 10, silent = True)
				if sshSession.getBefore().count('Completed') > 0: jobs.remove(j)
				if sshSession.getBefore().count('Error') > 0:
					logging.error(f'error for job {j}: ' + sshSession.getBefore())
					return False
			if jobs == []:
				logging.debug('all jobs completed')
				return True
			time.sleep(check_interval_sec)
			timeout_sec -= check_interval_sec
		logging.error(f"timeout while waiting for end of build of {jobs}")
		return False

	def _retag_image_statement(self, sshSession, oldImage, newImage, newTag, filename):
		sshSession.command(f'sed -i -e "s#{oldImage}:latest#{newImage}:{newTag}#" {filename}', '\$', 5)

	def _get_image_size(self, sshSession, image, tag):
		# get the SHA of the image we built using the image name and its tag
		sshSession.command(f'oc describe is {image} | grep -A4 {tag}', '\$', 5)
		result = re.search(f'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/(?P<imageSha>{image}@sha256:[a-f0-9]+)', sshSession.getBefore())
		if result is None:
			return -1
		imageSha = result.group("imageSha")

		# retrieve the size
		sshSession.command(f'oc get -o json isimage {imageSha} | jq -Mc "{{dockerImageSize: .image.dockerImageMetadata.Size}}"', '\$', 5)
		result = re.search('{"dockerImageSize":(?P<size>[0-9]+)}', str(sshSession.getBefore()))
		if result is None:
			return -1
		return int(result.group("size"))

	def _deploy_pod(self, sshSession, filename, timeout = 30):
		sshSession.command(f'oc create -f {filename}', '\$', 10)
		result = re.search(f'pod/(?P<pod>[a-zA-Z0-9_\-]+) created', sshSession.getBefore())
		if result is None:
			logging.error(f'could not deploy pod: {sshSession.getBefore()}')
			return None
		pod = result.group("pod")
		logging.debug(f'checking if pod {pod} is in Running state')
		while timeout > 0:
			sshSession.command(f'oc get pod {pod} -o json | jq -Mc .status.phase', '\$', 5, silent=True)
			if re.search('"Running"', sshSession.getBefore()) is not None: return pod
			timeout -= 1
			time.sleep(1)
		logging.error(f'pod {pod} did not reach Running state')
		self._undeploy_pod(sshSession, filename)
		return None

	def _undeploy_pod(self, sshSession, filename):
		# to save time we start this in the background and trust that oc stops correctly
		sshSession.command(f'oc delete -f {filename} &', '\$', 5)

	def BuildClusterImage(self, HTML):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit(f'Insufficient Parameter: ranRepository {self.ranRepository} ranBranch {ranBranch} ranCommitID {self.ranCommitID}')

		lIpAddr = self.eNBIPAddress
		lSourcePath = self.eNBSourceCodePath
		if lIpAddr == '' or lSourcePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter: no SSH Credentials')
		ocUserName = self.OCUserName
		ocPassword = self.OCPassword
		ocProjectName = self.OCProjectName
		if ocUserName == '' or ocPassword == '' or ocProjectName == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter: no OC Credentials')
		if self.OCRegistry.startswith("http") and not self.OCRegistry.endswith("/"):
			sys.exit(f'ocRegistry {self.OCRegistry} should not start with http:// or https:// and end on a slash /')

		logging.debug(f'Building on cluster triggered from server: {lIpAddr}')
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, "oaicicd", "SHOULD NOT BE NECESSARY")

		self.testCase_id = HTML.testCase_id

		# Workaround for some servers, we need to erase completely the workspace
		if self.forcedWorkspaceCleanup:
			mySSH.command(f'rm -Rf {lSourcePath}', '\$', 15)
		cls_containerize.CreateWorkspace(mySSH, lSourcePath, self.ranRepository, self.ranCommitID, self.ranTargetBranch, self.ranAllowMerge)

		# we don't necessarily need a forced workspace cleanup, but in
		# order to reduce the amount of data send to OpenShift, we
		# manually delete all generated files in the workspace
		mySSH.command(f'rm -rf {lSourcePath}/cmake_targets/ran_build', '\$', 30);

		baseTag = 'develop'
		forceBaseImageBuild = False
		imageTag = 'develop'
		if self.ranAllowMerge: # merging MR branch into develop -> temporary image
			imageTag = 'ci-temp'
			if self.ranTargetBranch == 'develop':
				mySSH.command(f'git diff HEAD..origin/develop -- cmake_targets/build_oai cmake_targets/tools/build_helper docker/Dockerfile.base.rhel8.2 | grep --colour=never -i INDEX', '\$', 5)
				result = re.search('index', mySSH.getBefore())
				if result is not None:
					forceBaseImageBuild = True
					baseTag = 'ci-temp'
		else:
			forceBaseImageBuild = True

		# logging to OC Cluster and then switch to corresponding project
		mySSH.command(f'oc login -u {ocUserName} -p {ocPassword} --server {self.OCUrl}', '\$', 31)
		if mySSH.getBefore().count('Login successful.') == 0:
			logging.error('\u001B[1m OC Cluster Login Failed\u001B[0m')
			mySSH.close()
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
			return False

		mySSH.command(f'oc project {ocProjectName}', '\$', 30)
		if mySSH.getBefore().count(f'Already on project "{ocProjectName}"') == 0 and mySSH.getBefore().count(f'Now using project "{self.OCProjectName}"') == 0:
			logging.error(f'\u001B[1mUnable to access OC project {ocProjectName}\u001B[0m')
			mySSH.command('oc logout', '\$', 30)
			mySSH.close()
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_PROJECT_FAIL)
			return False

		self._recreate_entitlements(mySSH)

		status = True # flag to abandon compiling if any image fails
		attemptedImages = []
		if forceBaseImageBuild:
			self._recreate_is_tag(mySSH, 'ran-base', baseTag, 'openshift/ran-base-is.yaml')
			self._recreate_bc(mySSH, 'ran-base', baseTag, 'openshift/ran-base-bc.yaml')
			ranbase_job = self._start_build(mySSH, 'ran-base')
			attemptedImages += ['ran-base']
			status = ranbase_job is not None and self._wait_build_end(mySSH, [ranbase_job], 600)
			if not status: logging.error('failure during build of ran-base')
			mySSH.command(f'oc logs {ranbase_job} > cmake_targets/log/ran-base.log', '\$', 10)
			# recover logs by mounting image
			self._retag_image_statement(mySSH, 'ran-base', 'ran-base', baseTag, 'openshift/ran-base-log-retrieval.yaml')
			pod = self._deploy_pod(mySSH, 'openshift/ran-base-log-retrieval.yaml')
			if pod is not None:
				mySSH.command(f'mkdir -p cmake_targets/log/ran-base', '\$', 5)
				mySSH.command(f'oc rsync {pod}:/oai-ran/cmake_targets/log/ cmake_targets/log/ran-base', '\$', 5)
				self._undeploy_pod(mySSH, 'openshift/ran-base-log-retrieval.yaml')
			else:
				status = False

		if status:
			self._recreate_is_tag(mySSH, 'oai-physim', imageTag, 'openshift/oai-physim-is.yaml')
			self._recreate_bc(mySSH, 'oai-physim', imageTag, 'openshift/oai-physim-bc.yaml')
			self._retag_image_statement(mySSH, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.phySim.rhel8.2')
			physim_job = self._start_build(mySSH, 'oai-physim')
			attemptedImages += ['oai-physim']

			self._recreate_is_tag(mySSH, 'ran-build', imageTag, 'openshift/ran-build-is.yaml')
			self._recreate_bc(mySSH, 'ran-build', imageTag, 'openshift/ran-build-bc.yaml')
			self._retag_image_statement(mySSH, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.build.rhel8.2')
			ranbuild_job = self._start_build(mySSH, 'ran-build')
			attemptedImages += ['ran-build']

			wait = ranbuild_job is not None and physim_job is not None and self._wait_build_end(mySSH, [ranbuild_job, physim_job], 1200)
			if not wait: logging.error('error during build of ranbuild_job or physim_job')
			status = status and wait
			mySSH.command(f'oc logs {ranbuild_job} > cmake_targets/log/ran-build.log', '\$', 10)
			mySSH.command(f'oc logs {physim_job} > cmake_targets/log/oai-physim.log', '\$', 10)

		if status:
			self._recreate_is_tag(mySSH, 'oai-enb', imageTag, 'openshift/oai-enb-is.yaml')
			self._recreate_bc(mySSH, 'oai-enb', imageTag, 'openshift/oai-enb-bc.yaml')
			self._retag_image_statement(mySSH, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.eNB.rhel8.2')
			self._retag_image_statement(mySSH, 'ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.eNB.rhel8.2')
			enb_job = self._start_build(mySSH, 'oai-enb')
			attemptedImages += ['oai-enb']

			self._recreate_is_tag(mySSH, 'oai-gnb', imageTag, 'openshift/oai-gnb-is.yaml')
			self._recreate_bc(mySSH, 'oai-gnb', imageTag, 'openshift/oai-gnb-bc.yaml')
			self._retag_image_statement(mySSH, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.rhel8.2')
			self._retag_image_statement(mySSH, 'ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.gNB.rhel8.2')
			gnb_job = self._start_build(mySSH, 'oai-gnb')
			attemptedImages += ['oai-gnb']

			self._recreate_is_tag(mySSH, 'oai-lte-ue', imageTag, 'openshift/oai-lte-ue-is.yaml')
			self._recreate_bc(mySSH, 'oai-lte-ue', imageTag, 'openshift/oai-lte-ue-bc.yaml')
			self._retag_image_statement(mySSH, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.lteUE.rhel8.2')
			self._retag_image_statement(mySSH, 'ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.lteUE.rhel8.2')
			lteue_job = self._start_build(mySSH, 'oai-lte-ue')
			attemptedImages += ['oai-lte-ue']

			self._recreate_is_tag(mySSH, 'oai-nr-ue', imageTag, 'openshift/oai-nr-ue-is.yaml')
			self._recreate_bc(mySSH, 'oai-nr-ue', imageTag, 'openshift/oai-nr-ue-bc.yaml')
			self._retag_image_statement(mySSH, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.nrUE.rhel8.2')
			self._retag_image_statement(mySSH, 'ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.nrUE.rhel8.2')
			nrue_job = self._start_build(mySSH, 'oai-nr-ue')
			attemptedImages += ['oai-nr-ue']

			wait = enb_job is not None and gnb_job is not None and lteue_job is not None and nrue_job is not None and self._wait_build_end(mySSH, [enb_job, gnb_job, lteue_job, nrue_job], 600)
			if not wait: logging.error('error during build of eNB/gNB/lteUE/nrUE')
			status = status and wait
			# recover logs
			mySSH.command(f'oc logs {enb_job} > cmake_targets/log/oai-enb.log', '\$', 10)
			mySSH.command(f'oc logs {gnb_job} > cmake_targets/log/oai-gnb.log', '\$', 10)
			mySSH.command(f'oc logs {lteue_job} > cmake_targets/log/oai-lte-ue.log', '\$', 10)
			mySSH.command(f'oc logs {nrue_job} > cmake_targets/log/oai-nr-ue.log', '\$', 10)

		# split and analyze logs
		imageSize = {}
		for image in attemptedImages:
			mySSH.command(f'mkdir -p cmake_targets/log/{image}', '\$', 5)
			mySSH.command(f'python3 ci-scripts/docker_log_split.py --logfilename=cmake_targets/log/{image}.log', '\$', 5)
			tag = imageTag if image != 'ran-base' else baseTag
			size = self._get_image_size(mySSH, image, tag)
			if size <= 0:
				imageSize[image] = 'unknown -- BUILD FAILED'
				status = False
			else:
				sizeMb = float(size) / 1000000
				imageSize[image] = f'{sizeMb:.1f} Mbytes (uncompressed: ~{sizeMb*2.5:.1f} Mbytes)'
			logging.info(f'\u001B[1m{image} size is {imageSize[image]}\u001B[0m')

		grep_exp = "\|".join(attemptedImages)
		mySSH.command(f'oc get images | grep -e \'{grep_exp}\' > cmake_targets/log/image_registry.log', '\$', 10);

		build_log_name = f'build_log_{self.testCase_id}'
		cls_containerize.CopyLogsToExecutor(mySSH, lSourcePath, build_log_name, lIpAddr, 'oaicicd', CONST.CI_NO_PASSWORD)

		mySSH.command('for pod in $(oc get pods | tail -n +2 | awk \'{print $1}\'); do oc delete pod ${pod}; done', '\$', 60)

		# logout will return eventually, but we don't care when -> start in background
		mySSH.command('oc logout &', '\$', 5)
		mySSH.close()

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
