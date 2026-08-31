# SPDX-License-Identifier: LicenseRef-CSSL-1.0

import os
from typing import NamedTuple
import logging

class GlobalTestCtx(NamedTuple):
    """Parameters that are identical for all test cases of a run."""
    repository: str
    workspace: str
    branch: str
    merge: bool
    targetBranch: str

    def Default(workspace=None):
        return GlobalTestCtx('', workspace if workspace is not None else os.getcwd(), '', False, '')

class TestCaseCtx(NamedTuple):
    test_idx: int
    logPath: str
    g: GlobalTestCtx

    def Default(logPath, g=None):
        return TestCaseCtx(112233, logPath, g if g is not None else GlobalTestCtx.Default())
    def baseFilename(self):
        # typically, the test ID is of form 001234 (6 digits)
        return f"{self.logPath}/{self.test_idx:06d}"

def archiveArtifact(cmd, ctx, remote_path):
    base = os.path.basename(remote_path)
    local = f"{ctx.baseFilename()}-{base}"
    logging.info(f"Archive artifact '{local}'")
    success = cmd.copyin(remote_path, local)
    if success:
        cmd.run(f'rm {remote_path}', silent=True)
    return local if success else None
