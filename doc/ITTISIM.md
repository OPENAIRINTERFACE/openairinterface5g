<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">Running L3 ITTI simulator</font></b>
    </td>
  </tr>
</table>

This page is valid on the following branches:

- `develop` starting from tag `2020.w48`

# 1. Building the ITTI simulator.

The ITTI simulator is available directly from the standard build.

```bash
$ source oaienv
$ cd cmake_targets
$ sudo ./build_oai -x -w None -c -ittiSIM
```

# 2. Running the ITTI simulator.

The ITTI simulator establishes ITTI-threaded communication between the gNB RRC task and the UE RRC task.

This allows to test the sequence of NGAP/RRC/NAS messages.

The main limitations are:

- NAS is a simple stub that just sends and receives messages
- only initial Attach sequence

## 2.1. Starting the ITTI simulator

The ITTI simulator is able to run with a connected 5GC or without any.
The develop branch tag `2020.w48` only works RRC without 5GC connection.

```bash
$ sudo -E ./ran_build/build/nr-ittisim -O gnb.conf
```

