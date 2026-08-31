# SPDX-License-Identifier: LicenseRef-CSSL-1.0

#---------------------------------------------------------------------
# Python for CI testing
#
#   Required Python Version
#     Python 3.x
#---------------------------------------------------------------------

#-----------------------------------------------------------
# Parsing Command Line Arguements
#-----------------------------------------------------------
import argparse

def strToBool(s):
    if s.lower() in ['true', 'yes', '1']:
        return True
    if s.lower() in ['false', 'no', '0', '']:
        return False
    raise argparse.ArgumentTypeError(f"cannot interpret '{s}' as boolean")

def ArgsParse(argvs,CiTestObj,RAN,HTML,CONTAINERS,CLUSTER):

    p = argparse.ArgumentParser(description="OAI CI driver", ) #formatter_class?
    p.add_argument('--local', '-l', action='store_true', default=False, help='Force local execution: rewrites the test xml script before running to always execute on localhost. Assumes images are available locally, will not remove any images and will run inside the current repo directory')
    p.add_argument('--datefmt', '-f', help="date format to prepend to logs")
    p.add_argument('--mode', '-m', help="One of: TesteNB, InitiateHtml, FinalizeHtml")
    p.add_argument('--repository', '-r', default='', help="OAI RAN Repository URL")
    p.add_argument('--ranAllowMerge', type=strToBool, default=False, help="Allow Merge Request (with target branch) (true or false)")
    p.add_argument('--branch', '-b', default='', help="OAI RAN Repository Branch")
    p.add_argument('--targetBranch', '-t', default='', help="Target Branch in case of a Merge Request")
    p.add_argument('--workspace', '-w', default='', help="directory for workspaces on remote hosts")
    p.add_argument('--XMLTestFile', '-x', action='append', default=[])
    p.add_argument('--finalStatus', type=strToBool, default=False)
    oc = p.add_argument_group(title="OpenShift", description="OC-specific parameters")
    oc.add_argument('--OCUserName')
    oc.add_argument('--OCPassword')
    oc.add_argument('--OCProjectName')
    oc.add_argument('--OCUrl')
    oc.add_argument('--OCRegistry')
    p.add_argument('--FlexRicTag')

    args = p.parse_args()

    CiTestObj.repository = args.repository
    RAN.repository = args.repository
    HTML.repository = args.repository
    CONTAINERS.repository = args.repository
    CLUSTER.repository = args.repository

    doMerge = args.ranAllowMerge
    RAN.merge = args.ranAllowMerge
    CONTAINERS.merge = args.ranAllowMerge
    CLUSTER.merge = args.ranAllowMerge

    CiTestObj.branch = args.branch
    RAN.branch = args.branch
    HTML.branch = args.branch
    CONTAINERS.branch = args.branch
    CLUSTER.branch = args.branch

    RAN.targetBranch = args.targetBranch
    CONTAINERS.targetBranch = args.targetBranch
    CLUSTER.targetBranch = args.targetBranch

    RAN.workspace = args.workspace
    CONTAINERS.workspace = args.workspace
    CLUSTER.workspace = args.workspace

    CiTestObj.testXMLfiles = args.XMLTestFile
    HTML.testXMLfiles = args.XMLTestFile
    HTML.nbTestXMLfiles = len(args.XMLTestFile)

    finalStatus = args.finalStatus
    CiTestObj.finalStatus = args.finalStatus

    CLUSTER.OCUserName = args.OCUserName
    CLUSTER.OCPassword = args.OCPassword
    CLUSTER.OCProjectName = args.OCProjectName
    CLUSTER.OCUrl = args.OCUrl
    CLUSTER.OCRegistry = args.OCRegistry

    CONTAINERS.flexricTag = args.FlexRicTag

    return args.mode, args.local, args.datefmt
