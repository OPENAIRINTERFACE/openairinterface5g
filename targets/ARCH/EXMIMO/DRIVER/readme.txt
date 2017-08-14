Here are the PC kernel drivers.

Currently, there are two drivers:


telecomparistech/
---
- supports the new ExpressMIMO-1 bitstream by Telecom Paristech
- uses command two command FIFOs to pass commands between Leon<->PC


eurecom/
---
- supports the legacy ExpressMIMO-1 bitstream
- supports ExpressMIMO-2, also multiple cards for sample-synchronous acquisition
- uses CONTROLx registers to pass commands between Leon<->PC
- allocates buffers in kernel space using pci_alloc_consistent
- used by ../USERSPACE/LIB/openair0_lib for simple access through userspace

