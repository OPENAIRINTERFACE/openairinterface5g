#!/bin/bash
# SPDX-License-Identifier: MIT
set -e

# This script downloads a PCAP file, unarchives it, and then runs a test executable.
#
# Arguments:
# $1: PCAP_DOWNLOAD_URL        - The URL to download the .xz archive from.
# $2: TEST_ORU_PCAP_EXECUTABLE - The full path to the test executable (e.g., test_oru_pcap).
# $3+: Extra arguments to the executable

DOWNLOAD_URL="$1"
shift
TEST_EXECUTABLE="$1"
shift
# Collect extra arguments
EXTRA_ARGS=()
while [[ $# -gt 0 ]]; do
	EXTRA_ARGS+=("$1")
	shift
done

#give each invocation its own scratch dir
WORKDIR="$(mktemp -d "/tmp/oru_pcap.XXXXXX")"
trap 'rm -rf "$WORKDIR"' EXIT
ARCHIVE_NAME="$WORKDIR/compressed_pcap.xz"
EXTRACTED_FILENAME="$WORKDIR/uncompressed.pcap"
if ! curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 -o "$ARCHIVE_NAME" "$DOWNLOAD_URL"; then
	echo "ERROR: cannot download test PCAP ${DOWNLOAD_URL} (network/GitHub access?)" >&2
	exit 2
fi
if ! xz --test "$ARCHIVE_NAME"; then
	echo "ERROR: downloaded PCAP archive is corrupt or truncated: ${ARCHIVE_NAME}" >&2
	exit 2
fi
xz --decompress -c "$ARCHIVE_NAME" > "$EXTRACTED_FILENAME"

echo "Running test ${TEST_EXECUTABLE} with PCAP ${DOWNLOAD_URL} and arguments: ${EXTRA_ARGS[*]}"
"${TEST_EXECUTABLE}" "${EXTRACTED_FILENAME}" "${EXTRA_ARGS[@]}"
