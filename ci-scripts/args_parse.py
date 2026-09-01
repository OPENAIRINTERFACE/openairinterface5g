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
from cls_ci_helper import GlobalTestCtx, OcCtx

def strToBool(s):
    if s.lower() in ['true', 'yes', '1']:
        return True
    if s.lower() in ['false', 'no', '0', '']:
        return False
    raise argparse.ArgumentTypeError(f"cannot interpret '{s}' as boolean")

def ArgsParse(argvs,HTML,CONTAINERS):

    p = argparse.ArgumentParser(description="OAI CI driver", ) #formatter_class?
    p.add_argument('--local', '-l', action='store_true', default=False, help='Force local execution: rewrites the test xml script before running to always execute on localhost. Assumes images are available locally, will not remove any images and will run inside the current repo directory')
    p.add_argument('--datefmt', '-f', help="date format to prepend to logs")
    p.add_argument('--mode', '-m', default="all-in-one", help="One of: TesteNB, InitiateHtml, FinalizeHtml")
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
    p.add_argument('--FlexRicTag')
    p.add_argument('xmls', nargs='*', default=[])

    args = p.parse_args()

    g = GlobalTestCtx(args.repository, args.workspace, args.branch, args.ranAllowMerge, args.targetBranch)

    all_xml = args.XMLTestFile + args.xmls
    HTML.testXMLfiles = all_xml
    HTML.nbTestXMLfiles = len(all_xml)

    oc = OcCtx(args.OCUserName, args.OCPassword, args.OCProjectName)

    CONTAINERS.flexricTag = args.FlexRicTag

    return args.mode, args.local, args.datefmt, args.finalStatus, g, oc
