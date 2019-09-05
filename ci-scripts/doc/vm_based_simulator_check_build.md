<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI CI Virtual-Machine-based Test Environment: checking the build result</font></b>
    </td>
  </tr>
</table>

## Table of Contents ##

1.  [Introduction](#1-introduction)
2.  [Detailed Description](#2-detailed-description)

# 1. Introduction #

Function is called:

- when building in foreground
- when waiting for the background build process is finished

# 2. Detailed Description #

Source file concerned: `ci-scripts/waitBuildOnVM.sh`

## 2.1. check_on_vm_build function ##

*  Retrieve the build log files from the VM instance `ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/*.txt`
*  and copy them locally in the workspace at $ARCHIVES_LOC
*  List all log files that match the pattern. Each should have
   *  the `Built target` pattern (the library/executable SHALL link)
*  The number of patterned log files SHALL match $NB_PATTERN_FILES defined in `ci-scripts/oai-ci-vm-tool` script for the variant

---

Next step: [how to test a function](./vm_based_simulator_test.md)

You can also go back to the [CI dev main page](./ci_dev_home.md)

