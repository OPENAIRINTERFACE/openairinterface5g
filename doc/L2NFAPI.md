<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">L2 nFAPI Simulator Usage</font></b>
    </td>
  </tr>
</table>

This simulator allows to test L2 and above Layers using the nFAPI interface.

The UE executable is able to "simulate" multiple UEs in order to stimulate the scheduler in the eNB.

**This simulator is available starting the `v1.0.0` release on the `master` branch.**

Currently the Continuous Integration process is validating this simulator the following way:

*  the LTE modem executable is run on one host (in our CI deployment it is a **Xenial Virtual Machine**)
*  the UE(s) modem executable is run on another host (in our CI deployment it is also a **Xenial Virtual Machine**)
*  We are testing:
   *   in S1 mode (ie we are connected to a 3rd-party EPC)
   *   in noS1 mode (no need for an EPC)

Normally it should be fine to run both executables on the same host using the `loopback` interface to communicate. **But we are not guaranting it**

1. [With S1 -- eNB and UE on 2 hosts](L2NFAPI_S1.md)
2. [No S1 -- eNB and UE on 2 hosts](L2NFAPI_NOS1.md)

----

[oai wiki home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)

[oai softmodem features](FEATURE_SET.md)

[oai softmodem build procedure](BUILD.md)

