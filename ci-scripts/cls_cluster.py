# SPDX-License-Identifier: LicenseRef-CSSL-1.0

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
import os

import cls_oai_html
import constants as CONST
import cls_containerize
import cls_cmd
from cls_ci_helper import archiveArtifact

IMAGE_REGISTRY_SERVICE_NAME = "image-registry.openshift-image-registry.svc"
NAMESPACE = "oaicicd-ran"
OCUrl = "https://api.oai.cs.eurecom.fr:6443"
OCRegistry = "default-route-openshift-image-registry.apps.oai.cs.eurecom.fr"
CI_OC_RAN_NAMESPACE = "oaicicd-ran"

def OC_login(cmd, ocUserName, ocPassword, ocProjectName):
	if ocUserName == '' or ocPassword == '' or ocProjectName == '':
		raise ValueError('Insufficient Parameter: no OC Credentials')
	if OCRegistry.startswith("http") or OCRegistry.endswith("/"):
		raise ValueError(f'ocRegistry {OCRegistry} should not start with http:// or https:// and not end on a slash /')
	ret = cmd.run(f'oc login -u {ocUserName} -p {ocPassword} --server {OCUrl} --insecure-skip-tls-verify')
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
		self.OCUserName = ""
		self.OCPassword = ""
		self.OCProjectName = ""
		self.OCUrl = OCUrl
		self.OCRegistry = OCRegistry
		self.cmd = None

	def _recreate_entitlements(self):
		# recreating entitlements, don't care if deletion fails
		self.cmd.run(f'oc delete secret etc-pki-entitlement')
		ret = self.cmd.run(f"oc get secret etc-pki-entitlement -n openshift-config-managed -o json | jq 'del(.metadata.resourceVersion)' | jq 'del(.metadata.creationTimestamp)' | jq 'del(.metadata.uid)' | jq 'del(.metadata.namespace)' | oc create -f -", silent=True)
		if ret.returncode != 0:
			logging.error("could not create secret/etc-pki-entitlement")
			return False
		return True

	def _recreate_bc(self, name, newTag, filename):
		self._retag_image_statement(name, name, newTag, filename)
		self.cmd.run(f'oc delete -f {filename}')
		ret = self.cmd.run(f'oc create -f {filename}')
		if re.search(r'buildconfig.build.openshift.io/[a-zA-Z\-0-9]+ created', ret.stdout) is not None:
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

	def _start_build(self, name, workspace):
		# will return "immediately" but build runs in background
		# if multiple builds are started at the same time, this can take some time, however
		ret = self.cmd.run(f'oc start-build {name} --from-dir={workspace} --exclude=""')
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

	def PullClusterImage(self, ctx, HTML, node, images, tag_prefix):
		logging.debug(f'Pull OC image {images} to server {node}')
		with cls_cmd.getConnection(node) as cmd:
			succeeded = OC_login(cmd, self.OCUserName, self.OCPassword, CI_OC_RAN_NAMESPACE)
			if not succeeded:
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
				return False
			ret = cmd.run(f'oc whoami -t | docker login -u oaicicd --password-stdin {self.OCRegistry}')
			if ret.returncode != 0:
				logging.error(f'cannot authenticate at registry')
				OC_logout(cmd)
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
				return False
			tag = ctx.g.branch
			registry = f'{self.OCRegistry}/{CI_OC_RAN_NAMESPACE}'
			success, msg = cls_containerize.Containerize.Pull_Image(cmd, images, tag, tag_prefix, registry, None, None)
			OC_logout(cmd)
		param = f"on node {node}"
		if success:
			HTML.CreateHtmlTestRowQueue(param, 'OK', [msg])
		else:
			HTML.CreateHtmlTestRowQueue(param, 'KO', [msg])
		return success

	def _retrieveOCLog(self, ctx, job, lSourcePath, image):
		fn = f'{lSourcePath}/cmake_targets/log/{image}.log'
		self.cmd.run(f'oc logs {job} &> {fn}')
		return (image, archiveArtifact(self.cmd, ctx, fn))

	def BuildClusterImage(self, ctx, node, HTML):
		if ctx.g.branch == '':
			raise ValueError(f'Insufficient Parameter: branch {ctx.g.branch}')
		lSourcePath = ctx.g.workspace
		if node == '' or lSourcePath == '':
			raise ValueError('Insufficient Parameter: workspace missing')
		ocUserName = self.OCUserName
		ocPassword = self.OCPassword
		ocProjectName = self.OCProjectName
		if ocUserName == '' or ocPassword == '' or ocProjectName == '':
			raise ValueError('Insufficient Parameter: no OC Credentials')
		if self.OCRegistry.startswith("http") or self.OCRegistry.endswith("/"):
			raise ValueError(f'ocRegistry {self.OCRegistry} should not start with http:// or https:// and not end on a slash /')

		logging.debug(f'Building on cluster triggered from server: {node}')
		self.cmd = cls_cmd.RemoteCmd(node)

		# Workaround for some servers, we need to erase completely the workspace
		self.cmd.cd(lSourcePath)
		# to reduce the amount of data send to OpenShift, we
		# manually delete all generated files in the workspace
		self.cmd.run(f'rm -rf {lSourcePath}/cmake_targets/ran_build');

		baseTag = 'develop'
		forceBaseImageBuild = False
		if ctx.g.merge: # merging MR branch into develop -> temporary image
			branchName = ctx.g.branch.replace('/','-')
			imageTag = f'{branchName}'
			if ctx.g.targetBranch == 'develop':
				ret = self.cmd.run(f'git diff HEAD..origin/develop -- cmake_targets/build_oai cmake_targets/tools/build_helper docker/Dockerfile.base.rhel9 | grep --colour=never -i INDEX')
				result = re.search('index', ret.stdout)
				if result is not None:
					forceBaseImageBuild = True
					baseTag = 'ci-temp'
			# if the branch name contains integration_20xx_wyy, let rebuild ran-base
			result = re.search('integration_20([0-9]{2})_w([0-9]{2})', ctx.g.branch)
			if not forceBaseImageBuild and result is not None:
				forceBaseImageBuild = True
				baseTag = 'ci-temp'
		else:
			imageTag = ctx.g.branch
			forceBaseImageBuild = True

		# logging to OC Cluster and then switch to corresponding project
		succeeded = OC_login(self.cmd, ocUserName, ocPassword, CI_OC_RAN_NAMESPACE)
		if not succeeded:
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
			self.cmd.close()
			return False

		# delete old images by Sagar Arora <sagar.arora@openairinterface.org>:
		# 1. retrieve all images and their timestamp
		# 2. awk retrieves those whose timestamp is older than 3 weeks
		# 3. issue delete command on corresponding istags (the images are dangling and will be cleaned by the registry)
		delete_cmd = "oc get istag -o go-template --template '{{range .items}}{{.metadata.name}} {{.metadata.creationTimestamp}}{{\"\\n\"}}{{end}}' | awk '$2 <= \"'$(date -d '-3weeks' -Ins --utc | sed 's/+0000/Z/')'\" { print $1 }' | xargs --no-run-if-empty oc delete istag"
		response = self.cmd.run(delete_cmd)
		logging.debug(f"deleted images:\n{response.stdout}")

		self._recreate_entitlements()

		status = True # flag to abandon compiling if any image fails
		log_files = []
		build_metrics = f"{lSourcePath}/cmake_targets/log/build-metrics.log"
		if forceBaseImageBuild:
			self._recreate_is_tag('ran-base', baseTag, 'openshift/ran-base-is.yaml')
			self._recreate_bc('ran-base', baseTag, 'openshift/ran-base-bc.yaml')
			ranbase_job = self._start_build('ran-base', lSourcePath)
			status = ranbase_job is not None and self._wait_build_end([ranbase_job], 1000)
			if not status: logging.error('failure during build of ran-base')
			log_files.append(self._retrieveOCLog(ctx, ranbase_job, lSourcePath, 'ran-base'))

		if status:
			self._recreate_is_tag('ran-build-fhi72', imageTag, 'openshift/ran-build-fhi72-is.yaml')
			self._recreate_bc('ran-build-fhi72', imageTag, 'openshift/ran-build-fhi72-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.build.fhi72.rhel9')
			ranbuildfhi72_job = self._start_build('ran-build-fhi72', lSourcePath)

			self._recreate_is_tag('oai-physim', imageTag, 'openshift/oai-physim-is.yaml')
			self._recreate_bc('oai-physim', imageTag, 'openshift/oai-physim-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.phySim.rhel9')
			physim_job = self._start_build('oai-physim', lSourcePath)

			self._recreate_is_tag('ran-build', imageTag, 'openshift/ran-build-is.yaml')
			self._recreate_bc('ran-build', imageTag, 'openshift/ran-build-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.build.rhel9')
			ranbuild_job = self._start_build('ran-build', lSourcePath)

			self._recreate_is_tag('oai-clang', imageTag, 'openshift/oai-clang-is.yaml')
			self._recreate_bc('oai-clang', imageTag, 'openshift/oai-clang-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.clang.rhel9')
			clang_job = self._start_build('oai-clang', lSourcePath)

			wait = ranbuildfhi72_job is not None and ranbuild_job is not None and physim_job is not None and clang_job is not None and self._wait_build_end([ranbuildfhi72_job, ranbuild_job, physim_job, clang_job], 1200)
			if not wait: logging.error('error during build of ranbuildfhi72_job or ranbuild_job or physim_job or clang_job')
			status = status and wait
			log_files.append(self._retrieveOCLog(ctx, ranbuildfhi72_job, lSourcePath, 'ran-build-fhi72'))
			log_files.append(self._retrieveOCLog(ctx, ranbuild_job, lSourcePath, 'ran-build'))
			log_files.append(self._retrieveOCLog(ctx, physim_job, lSourcePath, 'oai-physim'))
			log_files.append(self._retrieveOCLog(ctx, clang_job, lSourcePath, 'oai-clang'))
			self.cmd.run(f'oc get pods.metrics.k8s.io &>> {build_metrics}')

		if status:
			self._recreate_is_tag('oai-gnb-fhi72', imageTag, 'openshift/oai-gnb-fhi72-is.yaml')
			self._recreate_bc('oai-gnb-fhi72', imageTag, 'openshift/oai-gnb-fhi72-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.fhi72.rhel9')
			self._retag_image_statement('ran-build-fhi72', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build-fhi72', imageTag, 'docker/Dockerfile.gNB.fhi72.rhel9')
			gnb_fhi72_job = self._start_build('oai-gnb-fhi72', lSourcePath)

			self._recreate_is_tag('oai-enb', imageTag, 'openshift/oai-enb-is.yaml')
			self._recreate_bc('oai-enb', imageTag, 'openshift/oai-enb-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.eNB.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.eNB.rhel9')
			enb_job = self._start_build('oai-enb', lSourcePath)

			self._recreate_is_tag('oai-gnb', imageTag, 'openshift/oai-gnb-is.yaml')
			self._recreate_bc('oai-gnb', imageTag, 'openshift/oai-gnb-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.gNB.rhel9')
			gnb_job = self._start_build('oai-gnb', lSourcePath)

			self._recreate_is_tag('oai-gnb-aw2s', imageTag, 'openshift/oai-gnb-aw2s-is.yaml')
			self._recreate_bc('oai-gnb-aw2s', imageTag, 'openshift/oai-gnb-aw2s-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.aw2s.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.gNB.aw2s.rhel9')
			gnb_aw2s_job = self._start_build('oai-gnb-aw2s', lSourcePath)

			wait = gnb_fhi72_job is not None and enb_job is not None and gnb_job is not None and gnb_aw2s_job is not None and self._wait_build_end([gnb_fhi72_job, enb_job, gnb_job, gnb_aw2s_job], 800)
			if not wait: logging.error('error during build of eNB/gNB')
			status = status and wait
			# recover logs
			log_files.append(self._retrieveOCLog(ctx, gnb_fhi72_job, lSourcePath, 'oai-gnb-fhi72'))
			log_files.append(self._retrieveOCLog(ctx, enb_job, lSourcePath, 'oai-enb'))
			log_files.append(self._retrieveOCLog(ctx, gnb_job, lSourcePath, 'oai-gnb'))
			log_files.append(self._retrieveOCLog(ctx, gnb_aw2s_job, lSourcePath, 'oai-gnb-aw2s'))
			self.cmd.run(f'oc get pods.metrics.k8s.io &>> {build_metrics}')

			self._recreate_is_tag('oai-nr-cuup', imageTag, 'openshift/oai-nr-cuup-is.yaml')
			self._recreate_bc('oai-nr-cuup', imageTag, 'openshift/oai-nr-cuup-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.nr-cuup.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.nr-cuup.rhel9')
			nr_cuup_job = self._start_build('oai-nr-cuup', lSourcePath)

			self._recreate_is_tag('oai-lte-ue', imageTag, 'openshift/oai-lte-ue-is.yaml')
			self._recreate_bc('oai-lte-ue', imageTag, 'openshift/oai-lte-ue-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.lteUE.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.lteUE.rhel9')
			lteue_job = self._start_build('oai-lte-ue', lSourcePath)

			self._recreate_is_tag('oai-nr-ue', imageTag, 'openshift/oai-nr-ue-is.yaml')
			self._recreate_bc('oai-nr-ue', imageTag, 'openshift/oai-nr-ue-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.nrUE.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.nrUE.rhel9')
			nrue_job = self._start_build('oai-nr-ue', lSourcePath)

			wait = nr_cuup_job is not None and lteue_job is not None and nrue_job is not None and self._wait_build_end([nr_cuup_job, lteue_job, nrue_job], 800)
			if not wait: logging.error('error during build of nr-cuup/lteUE/nrUE')
			status = status and wait
			# recover logs
			log_files.append(self._retrieveOCLog(ctx, nr_cuup_job, lSourcePath, 'oai-nr-cuup'))
			log_files.append(self._retrieveOCLog(ctx, lteue_job, lSourcePath, 'oai-lte-ue'))
			log_files.append(self._retrieveOCLog(ctx, nrue_job, lSourcePath, 'oai-nr-ue'))
			self.cmd.run(f'oc get pods.metrics.k8s.io &>> {build_metrics}')

		# split and analyze logs
		imageSize = {}
		for image, _ in log_files:
			tag = imageTag if image != 'ran-base' else baseTag
			size = self._get_image_size(image, tag)
			if size <= 0:
				imageSize[image] = 'unknown -- BUILD FAILED'
				status = False
			else:
				sizeMb = float(size) / 1000000
				imageSize[image] = f'{sizeMb:.1f} Mbytes (uncompressed: ~{sizeMb*2.5:.1f} Mbytes)'
			logging.info(f'\u001B[1m{image} size is {imageSize[image]}\u001B[0m')

		archiveArtifact(self.cmd, ctx, build_metrics)
		logfile = f'{lSourcePath}/cmake_targets/log/image_registry.log'
		grep_exp = r"\|".join([i for i,f in log_files])
		self.cmd.run(f'oc get images | grep -e \'{grep_exp}\' &> {logfile}');
		archiveArtifact(self.cmd, ctx, logfile)
		logfile = f'{lSourcePath}/cmake_targets/log/build_pod_summary.log'
		self.cmd.run(f'for pod in $(oc get pods | tail -n +2 | awk \'{{print $1}}\'); do oc get pod $pod -o json &>> {logfile}; done')
		archiveArtifact(self.cmd, ctx, logfile)

		self.cmd.run('for pod in $(oc get pods | tail -n +2 | awk \'{print $1}\'); do oc delete pod ${pod}; done')

		# logout will return eventually, but we don't care when -> start in background
		self.cmd.run(f'oc logout')
		self.cmd.close()

		# Analyze the logs
		collectInfo = {}
		for image, lf in log_files:
			imgStatus, errors = cls_containerize.AnalyzeBuildLogs(image, lf)
			info = f"Analysis of {os.path.basename(lf)}: {imgStatus=}, size {imageSize[image]}, {len(errors)} errors"
			msg = "\n".join([info] + errors)
			HTML.CreateHtmlTestRowQueue(image, 'OK' if imgStatus else 'KO', [msg])
			status = status and imgStatus

		if status:
			logging.info('\u001B[1m Building OAI Image(s) Pass\u001B[0m')
		else:
			logging.error('\u001B[1m Building OAI Images Failed\u001B[0m')

		# TODO fix groovy script, remove the following.
		# the groovy scripts expects all logs in
		# <jenkins-workspace>/<pipeline>/ci-scripts, so copy it there
		with cls_cmd.LocalCmd() as c:
			c.run(f'mkdir -p {os.getcwd()}/test_log_{ctx.test_idx}/')
			c.run(f'cp -r {ctx.logPath} {os.getcwd()}/test_log_{ctx.test_idx}/')

		return status
