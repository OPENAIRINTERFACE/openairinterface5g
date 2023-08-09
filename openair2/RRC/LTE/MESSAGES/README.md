This directory contains the necessary schema files to generate all the LTE configuration data structures based on the ASN.1 RRC
source code from the 36.331 RRC specifications.  These structures are used by MAC and PHY for configuration purposes.

It contains the following files

```
README.txt                      : This file
ASN.1                           : Directory to place asn1c package
ASN.1/extract_asn1_from_spec.pl : Pearl script to extract ASN.1 source from 36311-860.txt
ASN.1/*.asn1                    : Extracted ASN.1 sources
ASN.1/*.cmake                   : Definitions of which files the corresponding *.asn1 will generate
CMakeLists.txt                  : The "driver" to generate and build generated source files at build time
```

Instructions to build data structures from ASN1 sources 

The asn.1 files have already been built using the `extract_asn1_from_spec.pl`
Pearl script.  This should be used again if a newer version of the RRC spec is
used to synthesize the data structures and encoding/decoding routines.  To do
this:
   1. use Microsoft WORD to generate a text version of the 3GPP 36.331 document
   2. run the script on the text file to generate the `*.asn1` file

You should install the asn1c utility using `./build_oai -I`.

Run
```bash
ASN1C_PREFIX=LTE_ asn1c -pdu=all -fcompound-names -gen-UPER -no-gen-BER -no-gen-JER -no-gen-OER -no-gen-APER -no-gen-example -D <dir> <asn.1-file>
```
to generate the files that result from the asn.1-file. Create an `ASN.1/*.cmake` file
with the following structure:
```
set(LTE_RRC_GRAMMAR <path-to-asn.1-file>)

set(lte_rrc_source
    <list-of-c-files>
)

set(lte_rrc_headers
    <list-of-h-files>
)
```

Modify the CMakeLists.txt to point to the newly generated .cmake file. On the
next LTE RRC compilation, the source files will be automatically generated from
the ASN.1 file and built henceforth.
