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

	def _recreate_entitlements(cmd):
		# recreating entitlements, don't care if deletion fails
		cmd.run(f'oc delete secret etc-pki-entitlement')
		ret = cmd.run(f"oc get secret etc-pki-entitlement -n openshift-config-managed -o json | jq 'del(.metadata.resourceVersion)' | jq 'del(.metadata.creationTimestamp)' | jq 'del(.metadata.uid)' | jq 'del(.metadata.namespace)' | oc create -f -", silent=True)
		if ret.returncode != 0:
			logging.error("could not create secret/etc-pki-entitlement")
			return False
		return True

	def _recreate_bc(cmd, name, newTag, filename):
		Cluster._retag_image_statement(cmd, name, name, newTag, filename)
		cmd.run(f'oc delete -f {filename}')
		ret = cmd.run(f'oc create -f {filename}')
		if re.search(r'buildconfig.build.openshift.io/[a-zA-Z\-0-9]+ created', ret.stdout) is not None:
			return True
		logging.error('error while creating buildconfig: ' + ret.stdout)
		return False

	def _recreate_is_tag(cmd, name, newTag, filename):
		ret = cmd.run(f'oc describe is {name}')
		if ret.returncode != 0:
			ret = cmd.run(f'oc create -f {filename}')
			if ret.returncode != 0:
				logging.error(f'error while creating imagestream: {ret.stdout}')
				return False
		else:
			logging.debug(f'-> imagestream {name} found')
		image = f'{name}:{newTag}'
		cmd.run(f'oc delete istag {image}', reportNonZero=False) # we don't care if this fails, e.g., if it is missing
		ret = cmd.run(f'oc create istag {image}')
		if ret.returncode == 0:
			return True
		logging.error(f'error while creating imagestreamtag: {ret.stdout}')
		return False

	def _start_build(cmd, name, workspace):
		# will return "immediately" but build runs in background
		# if multiple builds are started at the same time, this can take some time, however
		ret = cmd.run(f'oc start-build {name} --from-dir={workspace} --exclude=""')
		regres = re.search(r'build.build.openshift.io/(?P<jobname>[a-zA-Z0-9\-]+) started', ret.stdout)
		if ret.returncode != 0 or ret.stdout.count('Uploading finished') != 1 or regres is None:
			logging.error(f"error during oc start-build: {ret.stdout}")
			return None
		return regres.group('jobname') + '-build'

	def _wait_build_end(cmd, jobs, timeout_sec, check_interval_sec = 5):
		logging.debug(f"waiting for jobs {jobs} to finish building")
		while timeout_sec > 0:
			# check status
			for j in jobs:
				ret = cmd.run(f'oc get pods | grep {j}', silent = True)
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

	def _retag_image_statement(cmd, oldImage, newImage, newTag, filename):
		cmd.run(f'sed -i -e "s#{oldImage}:latest#{newImage}:{newTag}#" {filename}')

	def _get_image_size(cmd, image, tag):
		# get the SHA of the image we built using the image name and its tag
		ret = cmd.run(f'oc describe is {image} | grep -A4 {tag}')
		result = re.search(f'{IMAGE_REGISTRY_SERVICE_NAME}:5000/{NAMESPACE}/(?P<imageSha>{image}@sha256:[a-f0-9]+)', ret.stdout)
		if result is None:
			return -1
		imageSha = result.group("imageSha")

		# retrieve the size
		ret = cmd.run(f'oc get -o json isimage {imageSha} | jq -Mc "{{dockerImageSize: .image.dockerImageMetadata.Size}}"')
		result = re.search('{"dockerImageSize":(?P<size>[0-9]+)}', ret.stdout)
		if result is None:
			return -1
		return int(result.group("size"))

	def PullClusterImage(ctx, oc, HTML, node, images, tag_prefix):
		logging.debug(f'Pull OC image {images} to server {node}')
		with cls_cmd.getConnection(node) as cmd:
			succeeded = OC_login(cmd, oc.username, oc.password, CI_OC_RAN_NAMESPACE)
			if not succeeded:
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
				return False
			ret = cmd.run(f'oc whoami -t | docker login -u oaicicd --password-stdin {OCRegistry}')
			if ret.returncode != 0:
				logging.error(f'cannot authenticate at registry')
				OC_logout(cmd)
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
				return False
			tag = ctx.g.branch
			registry = f'{OCRegistry}/{CI_OC_RAN_NAMESPACE}'
			success, msg = cls_containerize.Containerize.Pull_Image(cmd, images, tag, tag_prefix, registry, None, None)
			OC_logout(cmd)
		param = f"on node {node}"
		if success:
			HTML.CreateHtmlTestRowQueue(param, 'OK', [msg])
		else:
			HTML.CreateHtmlTestRowQueue(param, 'KO', [msg])
		return success

	def _retrieveOCLog(cmd, ctx, job, lSourcePath, image):
		fn = f'{lSourcePath}/cmake_targets/log/{image}.log'
		cmd.run(f'oc logs {job} &> {fn}')
		return (image, archiveArtifact(cmd, ctx, fn))

	def BuildClusterImage(ctx, oc, node, HTML):
		if ctx.g.branch == '':
			raise ValueError(f'Insufficient Parameter: branch {ctx.g.branch}')
		lSourcePath = ctx.g.workspace
		if node == '' or lSourcePath == '':
			raise ValueError('Insufficient Parameter: workspace missing')

		logging.debug(f'Building on cluster triggered from server: {node}')
		cmd = cls_cmd.RemoteCmd(node)

		# Workaround for some servers, we need to erase completely the workspace
		cmd.cd(lSourcePath)
		# to reduce the amount of data send to OpenShift, we
		# manually delete all generated files in the workspace
		cmd.run(f'rm -rf {lSourcePath}/cmake_targets/ran_build');

		baseTag = 'develop'
		forceBaseImageBuild = False
		if ctx.g.merge: # merging MR branch into develop -> temporary image
			branchName = ctx.g.branch.replace('/','-')
			imageTag = f'{branchName}'
			if ctx.g.targetBranch == 'develop':
				ret = cmd.run(f'git diff HEAD..origin/develop -- cmake_targets/build_oai cmake_targets/tools/build_helper docker/Dockerfile.base.rhel9 | grep --colour=never -i INDEX')
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
		succeeded = OC_login(cmd, oc.username, oc.password, CI_OC_RAN_NAMESPACE)
		if not succeeded:
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
			cmd.close()
			return False

		# delete old images by Sagar Arora <sagar.arora@openairinterface.org>:
		# 1. retrieve all images and their timestamp
		# 2. awk retrieves those whose timestamp is older than 3 weeks
		# 3. issue delete command on corresponding istags (the images are dangling and will be cleaned by the registry)
		delete_cmd = "oc get istag -o go-template --template '{{range .items}}{{.metadata.name}} {{.metadata.creationTimestamp}}{{\"\\n\"}}{{end}}' | awk '$2 <= \"'$(date -d '-3weeks' -Ins --utc | sed 's/+0000/Z/')'\" { print $1 }' | xargs --no-run-if-empty oc delete istag"
		response = cmd.run(delete_cmd)
		logging.debug(f"deleted images:\n{response.stdout}")

		Cluster._recreate_entitlements(cmd)

		status = True # flag to abandon compiling if any image fails
		log_files = []
		build_metrics = f"{lSourcePath}/cmake_targets/log/build-metrics.log"
		if forceBaseImageBuild:
			Cluster._recreate_is_tag(cmd, 'ran-base', baseTag, 'openshift/ran-base-is.yaml')
			Cluster._recreate_bc(cmd, 'ran-base', baseTag, 'openshift/ran-base-bc.yaml')
			ranbase_job = Cluster._start_build(cmd, 'ran-base', lSourcePath)
			status = ranbase_job is not None and Cluster._wait_build_end(cmd, [ranbase_job], 1000)
			if not status: logging.error('failure during build of ran-base')
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, ranbase_job, lSourcePath, 'ran-base'))

		if status:
			Cluster._recreate_is_tag(cmd, 'ran-build-fhi72', imageTag, 'openshift/ran-build-fhi72-is.yaml')
			Cluster._recreate_bc(cmd, 'ran-build-fhi72', imageTag, 'openshift/ran-build-fhi72-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.build.fhi72.rhel9')
			ranbuildfhi72_job = Cluster._start_build(cmd, 'ran-build-fhi72', lSourcePath)

			Cluster._recreate_is_tag(cmd, 'oai-physim', imageTag, 'openshift/oai-physim-is.yaml')
			Cluster._recreate_bc(cmd, 'oai-physim', imageTag, 'openshift/oai-physim-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.phySim.rhel9')
			physim_job = Cluster._start_build(cmd, 'oai-physim', lSourcePath)

			Cluster._recreate_is_tag(cmd, 'ran-build', imageTag, 'openshift/ran-build-is.yaml')
			Cluster._recreate_bc(cmd, 'ran-build', imageTag, 'openshift/ran-build-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.build.rhel9')
			ranbuild_job = Cluster._start_build(cmd, 'ran-build', lSourcePath)

			Cluster._recreate_is_tag(cmd, 'oai-clang', imageTag, 'openshift/oai-clang-is.yaml')
			Cluster._recreate_bc(cmd, 'oai-clang', imageTag, 'openshift/oai-clang-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.clang.rhel9')
			clang_job = Cluster._start_build(cmd, 'oai-clang', lSourcePath)

			wait = ranbuildfhi72_job is not None and ranbuild_job is not None and physim_job is not None and clang_job is not None and Cluster._wait_build_end(cmd, [ranbuildfhi72_job, ranbuild_job, physim_job, clang_job], 1200)
			if not wait: logging.error('error during build of ranbuildfhi72_job or ranbuild_job or physim_job or clang_job')
			status = status and wait
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, ranbuildfhi72_job, lSourcePath, 'ran-build-fhi72'))
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, ranbuild_job, lSourcePath, 'ran-build'))
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, physim_job, lSourcePath, 'oai-physim'))
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, clang_job, lSourcePath, 'oai-clang'))
			cmd.run(f'oc get pods.metrics.k8s.io &>> {build_metrics}')

		if status:
			Cluster._recreate_is_tag(cmd, 'oai-gnb-fhi72', imageTag, 'openshift/oai-gnb-fhi72-is.yaml')
			Cluster._recreate_bc(cmd, 'oai-gnb-fhi72', imageTag, 'openshift/oai-gnb-fhi72-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.fhi72.rhel9')
			Cluster._retag_image_statement(cmd, 'ran-build-fhi72', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build-fhi72', imageTag, 'docker/Dockerfile.gNB.fhi72.rhel9')
			gnb_fhi72_job = Cluster._start_build(cmd, 'oai-gnb-fhi72', lSourcePath)

			Cluster._recreate_is_tag(cmd, 'oai-enb', imageTag, 'openshift/oai-enb-is.yaml')
			Cluster._recreate_bc(cmd, 'oai-enb', imageTag, 'openshift/oai-enb-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.eNB.rhel9')
			Cluster._retag_image_statement(cmd, 'ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.eNB.rhel9')
			enb_job = Cluster._start_build(cmd, 'oai-enb', lSourcePath)

			Cluster._recreate_is_tag(cmd, 'oai-gnb', imageTag, 'openshift/oai-gnb-is.yaml')
			Cluster._recreate_bc(cmd, 'oai-gnb', imageTag, 'openshift/oai-gnb-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.rhel9')
			Cluster._retag_image_statement(cmd, 'ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.gNB.rhel9')
			gnb_job = Cluster._start_build(cmd, 'oai-gnb', lSourcePath)

			Cluster._recreate_is_tag(cmd, 'oai-gnb-aw2s', imageTag, 'openshift/oai-gnb-aw2s-is.yaml')
			Cluster._recreate_bc(cmd, 'oai-gnb-aw2s', imageTag, 'openshift/oai-gnb-aw2s-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.aw2s.rhel9')
			Cluster._retag_image_statement(cmd, 'ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.gNB.aw2s.rhel9')
			gnb_aw2s_job = Cluster._start_build(cmd, 'oai-gnb-aw2s', lSourcePath)

			wait = gnb_fhi72_job is not None and enb_job is not None and gnb_job is not None and gnb_aw2s_job is not None and Cluster._wait_build_end(cmd, [gnb_fhi72_job, enb_job, gnb_job, gnb_aw2s_job], 800)
			if not wait: logging.error('error during build of eNB/gNB')
			status = status and wait
			# recover logs
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, gnb_fhi72_job, lSourcePath, 'oai-gnb-fhi72'))
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, enb_job, lSourcePath, 'oai-enb'))
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, gnb_job, lSourcePath, 'oai-gnb'))
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, gnb_aw2s_job, lSourcePath, 'oai-gnb-aw2s'))
			cmd.run(f'oc get pods.metrics.k8s.io &>> {build_metrics}')

			Cluster._recreate_is_tag(cmd, 'oai-nr-cuup', imageTag, 'openshift/oai-nr-cuup-is.yaml')
			Cluster._recreate_bc(cmd, 'oai-nr-cuup', imageTag, 'openshift/oai-nr-cuup-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.nr-cuup.rhel9')
			Cluster._retag_image_statement(cmd, 'ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.nr-cuup.rhel9')
			nr_cuup_job = Cluster._start_build(cmd, 'oai-nr-cuup', lSourcePath)

			Cluster._recreate_is_tag(cmd, 'oai-lte-ue', imageTag, 'openshift/oai-lte-ue-is.yaml')
			Cluster._recreate_bc(cmd, 'oai-lte-ue', imageTag, 'openshift/oai-lte-ue-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.lteUE.rhel9')
			Cluster._retag_image_statement(cmd, 'ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.lteUE.rhel9')
			lteue_job = Cluster._start_build(cmd, 'oai-lte-ue', lSourcePath)

			Cluster._recreate_is_tag(cmd, 'oai-nr-ue', imageTag, 'openshift/oai-nr-ue-is.yaml')
			Cluster._recreate_bc(cmd, 'oai-nr-ue', imageTag, 'openshift/oai-nr-ue-bc.yaml')
			Cluster._retag_image_statement(cmd, 'ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.nrUE.rhel9')
			Cluster._retag_image_statement(cmd, 'ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.nrUE.rhel9')
			nrue_job = Cluster._start_build(cmd, 'oai-nr-ue', lSourcePath)

			wait = nr_cuup_job is not None and lteue_job is not None and nrue_job is not None and Cluster._wait_build_end(cmd, [nr_cuup_job, lteue_job, nrue_job], 800)
			if not wait: logging.error('error during build of nr-cuup/lteUE/nrUE')
			status = status and wait
			# recover logs
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, nr_cuup_job, lSourcePath, 'oai-nr-cuup'))
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, lteue_job, lSourcePath, 'oai-lte-ue'))
			log_files.append(Cluster._retrieveOCLog(cmd, ctx, nrue_job, lSourcePath, 'oai-nr-ue'))
			cmd.run(f'oc get pods.metrics.k8s.io &>> {build_metrics}')

		# split and analyze logs
		imageSize = {}
		for image, _ in log_files:
			tag = imageTag if image != 'ran-base' else baseTag
			size = Cluster._get_image_size(cmd, image, tag)
			if size <= 0:
				imageSize[image] = 'unknown -- BUILD FAILED'
				status = False
			else:
				sizeMb = float(size) / 1000000
				imageSize[image] = f'{sizeMb:.1f} Mbytes (uncompressed: ~{sizeMb*2.5:.1f} Mbytes)'
			logging.info(f'\u001B[1m{image} size is {imageSize[image]}\u001B[0m')

		archiveArtifact(cmd, ctx, build_metrics)
		logfile = f'{lSourcePath}/cmake_targets/log/image_registry.log'
		grep_exp = r"\|".join([i for i,f in log_files])
		cmd.run(f'oc get images | grep -e \'{grep_exp}\' &> {logfile}');
		archiveArtifact(cmd, ctx, logfile)
		logfile = f'{lSourcePath}/cmake_targets/log/build_pod_summary.log'
		cmd.run(f'for pod in $(oc get pods | tail -n +2 | awk \'{{print $1}}\'); do oc get pod $pod -o json &>> {logfile}; done')
		archiveArtifact(cmd, ctx, logfile)

		cmd.run('for pod in $(oc get pods | tail -n +2 | awk \'{print $1}\'); do oc delete pod ${pod}; done')

		# logout will return eventually, but we don't care when -> start in background
		cmd.run(f'oc logout')
		cmd.close()

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
