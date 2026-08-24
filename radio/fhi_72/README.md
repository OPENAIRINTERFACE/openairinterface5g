<!-- SPDX-License-Identifier: CC-BY-4.0 -->

This document describes the driver for the xran driver, implementing O-RAN FHI
7.2 interface. This is for developers; for user documentation, consider the
[O-RAN 7.2 tutorial instead](../../doc/ORAN_FHI7.2_Tutorial.md). 

The main source files and their purpose are as follows:

- `oaioran.c`: definition of xran-specific callback functions.
- `oran-config.c`: reading of 7.2-specific configuration and preparing xran
  configration.
- `oran-init.c`: initialization of xran.
- `oran_isolate.c`: main entry point and definition of function pointers for OAI
  callbacks.

The entry point for the driver library is `transport_init()` in `oran_isolate.c`.
This function is concerned with setting necessary function pointers for OAI
integration, and prepares xran configuration through `get_xran_config()`. This
function reads 7.2 specific configuration and deduces some of the parameters
(e.g., frequency) from OAI-provided radio configuration in `openair0_config_t`.

Actual xran initialization happens in `oai_oran_initialize()` in `oran-init.c`.
It calls various helper function in the same file to set up DPDK memory buffer
(pools). Notably, these callbacks are installed:

- `oai_xran_fh_rx_callback()` through `xran_5g_fronthault_config()`: callback
  for PUSCH data; used to provide timing data.
- `oai_xran_fh_rx_prach_callback()` through `xran_5g_prach_req()`: callback for
  PRACH data; not currently used and implemented in PUSCH data callback. 
- `oai_physide_dl_tti_call_back()` through `xran_reg_physide_cb()`: only used
  to unblock timing in `oai_xran_fh_rx_callback()` and `oai_xran_fh_rx_prach_callback()`
  upon first xran call.

More detailed information about the xran callbacks can be taken from the xran
documentation.

During normal operation, OAI calls into the driver through functions
`oran_fh_if4p5_south_in()` for PUSCH/PRACH, `oran_fh_if4p5_south_out()` for
PDSCH, and `oran_fh_if4p5_ctrl()` for CP UL/DL.

For PDSCH, `oran_fh_if4p5_south_out()` calls `xran_fh_tx_send_slot()` that
optionally compresses IQ data, then writes it into IQ buffers of xran.

For PUSCH/PRACH, `oran_fh_if4p5_south_in()` calls `xran_fh_rx_read_slot()`/
`xran_fh_rx_prach_read_slot()` that blocks and waits for the next slot.
This is done through a message queue which depends on xran calling the callbacks
`oai_xran_fh_rx_callback()`/`oai_xran_fh_rx_prach_callback()` as installed during
xran initialization.
Once unblocked, it reads first PUSCH data, then PRACH data, before returning to
OAI.
