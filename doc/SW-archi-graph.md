<!-- SPDX-License-Identifier: CC-BY-4.0 -->

This document is a high-level overview over the L1 threading mechanism.

```mermaid
flowchart TB
    ru_thread --> RFin[block rx_rf] --> UL{is UL slot?}
    UL -- yes --> wait_free_rx_tti --> fep_rx --> rx_nr_prach_ru --> msg:L1_tx_out
    UL -- no  --> msg:L1_tx_out
    msg:L1_tx_out -- asnyc launch --> L1_tx_thread
    msg:L1_tx_out --> RFin
```

The main thread is `ru_thread()`. It blocks on reception of radio samples
(either time domain or frequency domain).  In the case of an UL slot, it waits
that no more than N UL jobs are scheduled (via `wait_free_rx_tti()`, which
waits on queue `L1_rx_out`, cf. the RX L1 processing further below). Then:

- if the radio is time domain-based, it performs RX front-end processing (RX
  FEP -> `fep_rx()`, i.e. DFT) to reach a frequency domain representation of
  the RX signal, as well as does DFT for PRACH.
- if the radio is frequency domain-based, nothing is done.

Afterwards, it triggers TX processing by pushing a message into the FIFO queue
`L1_tx_out`, which asynchronously starts a TX job in `L1_tx_thread()` (see
below). After that, it blocks again on reception on the radio.


```mermaid
flowchart TB
    L1_tx_thread --> TX_in[block L1_tx_out] --> NR_slot_indication

    subgraph tx_func

        NR_slot_indication --> msg:resp_L1 --> phy_procedures_gNB_tx --> feptx_prec/feptx_ofdm --> fh_south_out
        NR_slot_indication["run the scheduler:
          - monolithic: run_scheduler_monolithic()
          - nFAPI: send indication via pnf_send_slot_ind()"]
        msg:resp_L1 -- async launch --> L1_RX_thread
    end
    fh_south_out --> TX_in
```

The `L1_tx_thread()` processes individual TX jobs sequentially, by waiting for
new messages on queue `L1_tx_out`, signalling individual TX jobs. For each
message, it calls `tx_func()` which does in order:

- run the scheduler through `NR_slot_indication`, which corresponds to a
  "Slot.indication" in FAPI parlance. This runs the scheduler, and schedules a
  given slot (either downlink, uplink, or both).
- trigger RX processing by pushing a message into the FIFO queue `resp_L1`,
  asynchronously starting an RX job in `L1_rx_thread()` (see below).
- process the current L1 TX job through `phy_procedures_gNB_tx()`
- write to the radio board via `fh_south_out()`.

After these steps, `tx_func()` return to `L1_tx_thread()`, which will wait for
the next TX job.

```mermaid
flowchart TB
    L1_rx_thread --> RX_in[block resp_L1] --> L1_nr_prach_proc

    subgraph rx_func

        L1_nr_prach_proc --> phase_comp{apply phase comp.?}
        phase_comp -- yes --> apply_nr_rotation --> phy_procedures_gNB_uespec_RX
        phase_comp -- no --> phy_procedures_gNB_uespec_RX
        phy_procedures_gNB_uespec_RX --> NR_ul_indication --> msg:L1_rx_out
        NR_ul_indication["run the scheduler: NR_UL_indication()"]
        msg:L1_rx_out -- async signal free --> ru_thread
    end
    msg:L1_rx_out --> RX_in
```

The `L1_rx_thread()` processes individual RX jobs sequentially. It waits for a
new RX job through the queue `resp_L1`, and then calls `rx_func()`, which does
in order:

- run PRACH processing via `L1_nr_prach_proc()`
- optionally apply rotation to the RX signal if phase compensation is to be
  applied
- run the current L1 RX job through (`phy_procedures_gNB_uespec_RX()`), which
  notably includes PUCCH, PUSCH, SRS processing
- call the scheduler through `NR_ul_indication()`, which corresponds to FAPI
  uplink messages (e.g., `RX_data.indication`, `CRC.indication`,
  `UCI.indication` etc.)
- signal completion via FIFO queue `L1_rx_out()`, which tells `ru_thread()`
  that RX processing finished.

The signalling of scheduler data is done through a variable `UL_INFO`, which is
filled by `L1_nr_prach_proc()` (for PRACH) and `phy_procedures_gNB_uespec_RX()`
(for PUCCH, PUSCH, SRS).

After these steps, `rx_func()` returns to `L1_rx_thread()`, which will wait the
next RX job.

Note that while individual TX (RX) jobs are run sequentially through
`L1_tx_thread()` (`L1_rx_thread()`), both TX and RX processing run in
parallel.
