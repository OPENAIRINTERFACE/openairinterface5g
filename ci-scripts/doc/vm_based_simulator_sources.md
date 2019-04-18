<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI CI Virtual-Machine-based Simulator OAI source management</font></b>
    </td>
  </tr>
</table>

## Table of Contents ##

1.  [Introduction](#1-introduction)
2.  [Centralized Workspace](#2-centralized-workspace)
3.  [Create the ZIP file](#3-create-the-zip-file)

# 1. Introduction #

The idea of this section is to optimize/uniform source management for several VM instances.

If we were cloning the same repository in each VM we are creating, we would put so much pressure on the central GitLab repository.

The solution:

* clone/fetch on a given clean workspace and tar/zip only the source files without any artifacts.
* we then copy the tar/zip file to each VM instance
* and within each VM instance, unzip

# 2. Centralized Workspace #

You can create a brand new workspace by cloning:

```bash
$ mkdir /tmp/CI-raphael
$ cd /tmp/CI-raphael
$ git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git .
$ git checkout develop
```

You can also use your current cloned workspace and any `develop`-based branch.

```bash
$ cd /home/raphael/openairinterface5g
$ git fetch
$ git checkout develop-improved-documentation
# CAUTION: the following command will remove any file that has not already been added to GIT
$ sudo git clean -x -d -ff
$ git status
On branch develop-improved-documentation
Your branch is up-to-date with 'origin/develop-improved-documentation'.
nothing to commit, working directory clean
```

You can also have modified files.

**The main point is to have NO ARTIFACTS from a previous build in your workspace.**

Last point, the workspace folder name is not necesseraly `openairinterface5g`. But all the following commands will be run for the root of the workspace.

For clarity, I will always use `/tmp/CI-raphael` as $WORKSPACE.

# 3. Create the ZIP file #

```bash
# go to root of workspace
$ cd /tmp/CI-raphael
$ zip -r -qq localZip.zip .
```

The **Jenkins Pipeline** performs automatically these operations.

In addition, in case of a merge request, it tries to merge with the target branch and might create a dummy local commit.

See [section](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/ci/enb-master-job#32-verify-guidelines-stage)

---

Next step: [the main scripts](./vm_based_simulator_main_scripts.md)

You can also go back to the [CI dev main page](./ci_dev_home.md)

