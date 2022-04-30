import argparse
import os
import re
import subprocess
import sys

AUTH_SERVICE = 'registry.docker.io'
AUTH_SCOPE   = 'repository:oaisoftwarealliance/oai-enb:pull'

def main() -> None:
    args = _parse_args()

    cmd = 'curl -fsSL "https://auth.docker.io/token?service=' + AUTH_SERVICE + '&scope=' + AUTH_SCOPE + '" 2>/dev/null | jq --raw-output ".token"'
    token = subprocess.check_output(cmd, shell=True, universal_newlines=True)
    token = str(token).strip()
    cmd = 'curl -fsSL -H "Authorization: Bearer ' + token + '" "https://index.docker.io/v2/oaisoftwarealliance/oai-enb/tags/list" 2>/dev/null | jq .'
    listOfTags = subprocess.check_output(cmd, shell=True, universal_newlines=True)

    foundTag = False
    for tag in listOfTags.split('\n'):
        if re.search('"' + args.start_tag + '"', tag) is not None:
            foundTag = True

    if not foundTag:
        print (args.start_tag)
        sys.exit(0)

    proposedVariants = ['a', 'b', 'c', 'd']
    for variant in proposedVariants:
        foundTag = False
        currentVariant = variant
        for tag in listOfTags.split('\n'):
            if re.search('"' + args.start_tag + variant + '"', tag) is not None:
                foundTag = True
                break
        if not foundTag:
            break

    if not foundTag:
        print (args.start_tag + currentVariant)

def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Provides an unique new image tag for DockerHub')

    parser.add_argument(
        '--start_tag', '-st',
        action='store',
        required=True,
        help='Proposed Starting Tag',
    )
    return parser.parse_args()

if __name__ == '__main__':
    main()
