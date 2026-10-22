<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Implementation of Beamforming

This document explains the implementation of beamforming in the OAI codebase:
the beam-index based schemes in which the beam weights live outside L2 (analog
beamforming at the RU, and digital beamforming driven by a digital beam table at
L1), and the scheduler constraints they impose.

[[_TOC_]]

## Introduction to beamforming

Beamforming is a technique applied to antenna arrays to create a directional
radiation pattern. This often consists in providing a different phase shift to
each element of the array such that signals with a different angle of
arrival/departure experience a change in radiation pattern because of
constructive or destructive interference.

There are three main beamforming techniques: analog, digital and hybrid. The
names refer to the phase shift application before or after the digital to analog
conversion (or analog to digital in reception). When we speak about analog
beamforming we generally refer to a technique where the phase shifts that
produce the beam steering are applied by the radio unit (RU) choosing from a
finite set of steering directions. The advantage of analog beamforming is a
simplified analog circuitry and therefore reduced costs.

The presence of a limited number of predefined beams at RU poses constraints to
the scheduler at gNB. As a matter of fact, the scheduler can serve only a
limited number of beams, depending on the RU characteristics (possibly only 1),
in a given time scale, that also depends on the RU characteristics (e.g. 1 slot
or 1 symbol). This limitation doesn't exist for digital beamforming.

## Configuration file fields for beamforming

A set of parameters in configuration files controls the implementation of
beamforming and instructs the scheduler on how to behave in such scenarios.

Everything is controlled from the `MACRLC` section of the configuration file.
The single parameter `bf_method` selects the beamforming method:
- `straight-wire` (default): no beamforming is applied, each logical antenna
  port maps straight onto a physical one.
- `das`: distributed antenna system, where each beam corresponds to one antenna
  (or a set of antennas) of the system. The beam index is the logical antenna
  port index, so `ssb_beams` must not be set and the number of concurrent beams
  equals `beams_per_period`. The scheduler constraint above is alleviated,
  because the number of concurrent beams allowed normally equals the total
  number of beams. This is OAI-specific and is expected to be removed once the
  digital beam table is fully supported. The reference configuration file is
  [`gnb-du.sa.band77.273prb.fhi72.4x4-das-benetel650_650.conf`](../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb-du.sa.band77.273prb.fhi72.4x4-das-benetel650_650.conf).
- `predefined`: the beam IDs allocated by L2 are signalled to L1 over FAPI.
  `ssb_beams` gives the beam ID to use for each transmitted SSB, and what L1
  does with those IDs depends on whether a digital beam table (DBT) is
  configured:
  - with a DBT (`dbt_file`, or a DBT inlined in the `MACRLC` section), each beam
    ID names one of the DBT entries by its `beam_id` column and L1 applies the
    corresponding weights. The DBT is typically much larger than the number of
    SSBs, so the beam IDs in `ssb_beams` select a subset of it; a beam ID that
    does not appear in the table is rejected at startup.
  - without a DBT, L1 forwards the beam ID as is to the radio (e.g. an
    InterDigital frontend) or to the fronthaul (e.g. over 7.2x).
- `dynamic`: static beamforming for the control signals, with DLSCH/ULSCH
  precoded from the SRS channel estimate. Not implemented yet: selecting it
  aborts at startup.

The remaining parameters are:
- `beam_duration` is the number of slots (currently minimum duration of a beam)
  the scheduler is tied to a beam (default value is 1)
- `beams_per_period` is the number of concurrent beams the RU can handle in the
  beam duration (default value is 1)
- `ssb_beams` is a vector field containing the set of beam indices statically
  allocated to SSB/PRACH, required for `predefined`. The number of beam indices
  must be equal to the number of SSBs transmitted; a mismatch in either
  direction is rejected at startup.
- `dbt_file` is the path to a CSV file holding the digital beam table; the table
  can also be inlined in the same section as a `dbt` list. See [Digital beam
  table (DBT)](#digital-beam-table-dbt).

### Aerial section

With the NVIDIA Aerial L1 the antenna array is not described by an `RUs`
section, so a separate top-level `Aerial` section carries the counts advertised
to L1 in `CONFIG.request` as `numTxAnt`/`numRxAnt`:

```
Aerial = {
  num_tx_ant = 64;
  num_rx_ant = 64;
};
```

Both values are in 0..64. They count TXRUs (see [Digital beam table
(DBT)](#digital-beam-table-dbt)), kept apart from the logical antenna ports
(`numTxPort`/`numRxPort`), which are still derived from `pdsch_AntennaPorts_*`
and `pusch_AntennaPorts`: `numRxAnt` sizes the L1 uplink receive processing, so
deriving it from the logical ports used to cost the array gain of every TXRU
beyond the configured number of UL layers.

The section is required for `bf_method = "predefined"` and `"dynamic"`, which
apply the beam weights, or the SRS-based precoder, per TXRU. For `straight-wire`
and `das` it is optional, and the counts keep being derived from the logical
ports when it is absent.

> **Note:** `bf_method` replaces the previous `set_analog_beamforming`
parameter, and `ssb_beams` replaces `beam_weights`; DAS is no longer selected
with `enable_das` in the `L1` section. Tag `2026.w42` is the last one accepting
the old parameters. Configuration files still using them are rejected at startup
with a message pointing at the replacement.

## Digital beam table (DBT)

The digital beam table maps each beam ID to a vector of complex weights, one per
TXRU, the transceiver unit of the 3GPP antenna model: one RF chain, carried as
one eAxC in a 7.2x split and counted by FAPI as `numTxAnt`. A TXRU is not
necessarily one physical antenna element — it can drive several of them through
the analog part of the array — so the weights are per TXRU, not per element. It
is what turns `bf_method = "predefined"` from "pass the beam ID through to the
RU" into "L1 applies these weights": with a table, L1 resolves each beam ID
through it; without one, the ID is forwarded unchanged to the radio or the
fronthaul.

A DBT is only meaningful for `predefined`. Configuring one together with
`straight-wire` or `das` is rejected at startup.

It is also only supported with Aerial for the time being. The table does reach a
native L1, but `nr_feptx_prec()` copies the samples through instead of precoding
them, so the weights would have no effect; configuring a table without Aerial is
rejected rather than silently ignored.

A single CSI port is required as well, that is `pdsch_AntennaPorts_N1 *
pdsch_AntennaPorts_N2 * pdsch_AntennaPorts_XP` must be 1. The scheduler signals
one beam ID per PDU and repeats it on every FAPI beamforming port, so with more
ports the weights of a single beam would be applied to all of them instead of
one weight vector per port. A table configured together with more CSI ports is
rejected at startup.

The table can come from either of two places, checked in this order:

1. `dbt_file`, a path to a CSV file (relative to the working directory of the
   gNB process);
2. a `dbt` list inlined in the same `MACRLC` section.

If `dbt_file` is set to a non-empty string it wins, and an inline `dbt` list is
ignored.

### CSV format

The first line is a header. Its contents are ignored, but its column count
defines the number of weights per beam. Each following line describes one beam:
the first column is the beam ID (decimal, 0 to 32767), the remaining columns are
the complex weights.

```csv
beam_id,txru0,txru1,txru2,txru3
0,0.5,0.5,0.5,0.5
1,0.5,0.5-0.5i,-0.5,-0.5+0.5i
68,0.5,-0.5,0.5,-0.5
```

Every row must carry exactly as many weight columns as the header announced; a
row with too few or too many columns aborts startup. Note that the header is
mandatory: a file that starts directly with data has its first beam consumed as
the header.

### Inline format

The same table can be written directly in the `MACRLC` section as a `dbt` list.
Each element is itself a list of strings: the first is the beam ID, the rest are
the weights. Note that the elements are bare lists, not groups — there is no key
in front of them.

```
MACRLCs = (
{
  ...
  bf_method        = "predefined";
  beam_duration    = 1;
  beams_per_period = 1;
  ssb_beams        = [1];
  dbt = (
    ("0", "0.5", "0.5",      "0.5",  "0.5"),
    ("1", "0.5", "0.5-0.5i", "-0.5", "-0.5+0.5i")
  );
}
);
```

All rows must have the same number of weights; the first row sets the expected
width. Note that the values are quoted: the weights are parsed by OAI, not by
libconfig, which is what allows the complex forms below.

### Weight syntax

Each weight is a single token, in one of three forms, with either `i` or `j`
marking the imaginary unit:

| form           | examples                  |
|----------------|---------------------------|
| real only      | `0.5`, `-0.25`, `1`       |
| imaginary only | `0.5i`, `-0.5j`           |
| complex        | `0.5+0.5i`, `0.5-0.5j`    |

In the complex form the imaginary part must carry an explicit sign, so that the
two numbers can be told apart.

Both the real and the imaginary part must lie in [-1, 1]. They are converted to
Q15 (multiplied by 32767 and rounded) before being handed to L1, so the table is
written in normalised floating point and the fixed-point conversion is done for
you.

The number of weights per beam must equal the number of TXRUs, and a mismatch
is rejected at startup. Under Aerial that count is `num_tx_ant` of the
[`Aerial` section](#aerial-section). Should a native L1 gain DBT support, it
would instead be the sum of `nb_tx` over the `RUs` section.

### Beam IDs and their relation to `ssb_beams`

Beam IDs name the rows of the table; they are arbitrary 16-bit values and do not
have to start at zero, be contiguous, or be sorted, but they must be unique — a
table listing the same ID twice is rejected at startup. A table typically holds
many more beams than there are SSBs, so `ssb_beams` selects the subset used for
SSB/PRACH, one entry per transmitted SSB. Every ID listed in `ssb_beams` must
appear in the table, otherwise startup fails with

```
ssb_beams[<i>] = <id> is not a beam of the digital beam table (<n> entries)
```

With a table configured, L1 resolves each beam ID through it; without one, the
IDs are passed on to the RU or the fronthaul unchanged.

For an example without a digital beam table, see
[`gnb.sa.band257.u3.66prb.rfsim.4beams.conf`](../ci-scripts/conf_files/gnb.sa.band257.u3.66prb.rfsim.4beams.conf),
which maps the four transmitted SSBs onto four beams.

### What reaches L1

The table is sent to L1 in the `CONFIG.request` as a vendor-extension TLV
(`nfapi_nr_dbt_pdu_t`): `num_dig_beams` entries of `num_txrus` weights each,
every entry carrying its `beam_idx` and a list of Q15 real/imaginary pairs. At
startup the gNB logs

```
Loaded DBT: num_beams=<n> num_weights_per_beam=<m>
```

with the individual weights available at debug log level.

## Implementation in OAI scheduler

A new MAC structure `NR_beam_info_t` controls the behavior of the scheduler in
presence of analog beamforming. Besides the already mentioned parameters
`beam_duration` and `beams_per_period`, the structure also holds a matrix
`beam_allocation[i][j]`, whose indices `i` and `j` stands respectively for the
number of beams in the period and the slot index (the size of the latter depends
on the frame characteristics). This matrix contains the beams already allocated
in a given slot, to flag the scheduler to use one of these to schedule a UE in
one of these beams. If the matrix is full (all the beams in the given period,
e.g. slot) are already allocated, the scheduler can't allocate a UE in a new
beam. To this goal, we extended the virtual resource block (VRB) map by one
dimension to also contain information per allocated beam. As said, the scheduler
can independently schedule users in a number of beams up to `beams_per_period`
concurrently.

It is important to note that in current implementation, there are several
periodical channels, e.g. PRACH or PUCCH for CSI et cetera, that have the
precendence in being assigned a beam, that is because the scheduling is
automatic, set in RRC configuration, and not up to the scheduler. For these
instances, we assume the beam is available (if not there are assertions to stop
the process). For data channels, the currently implemented PF scheduler is used.
The only modification is that a UE can be served only if there is a free beam
available or the one of the beams already in use correspond to that UE beam.

## Beams in phy-test scheduler

In phy-test mode, beams are assigned to PDSCH slots in the same manner as SSB
slots with the only addition that it repeats for every TDD period in a frame.
For example if PDSCH is scheduled on all DL slot for TDD format DDDDDDDSUU, and
with SSB bit map = `0b1010101` and the following beam parameters in config file,
- `ssb_beams` = [10,11,12,13]
- `beam_duration` = 1
- `beams_per_period` = 1

The DL slots in every TDD period will have beams 10,11,12,13,0,0,0

## FAPI implementation

To be noted that in our implementation analog beamforming is only partially
supported in split mode. The index based beamforming relies on the beam-ID of
the Tx precoding and beamforming PDU, where the MSB signals whether the ID can
be used directly or is a pointer to a pre-stored vector of weights. This
definition of beam-ID is present only in the most recent versions of SCF PHY
API specifications (at least from v8 and later, possibly from v6).

In addition to that, a `config_request` structure defined as vendor extension
(`nfapi_nr_analog_beamforming_ve_t`) configures the lower layers at
initialization with the following information:
- `analog_bf_vendor_ext` which can assume values 1 or 0 for enabling or
  disabling analog beamforming

L2 then provides the beam index in each channel FAPI message via the beam-ID
parameter.

### Beam ID encoding

`convert_to_fapi_beam()` builds that value from the beam ID the scheduler
allocated:

- MSB set: the remaining 15 bits are the beam ID itself, which L1 does not
  resolve and forwards to the radio or to the fronthaul. This is the case for
  `das`, and for `predefined` without a digital beam table.
- MSB clear: the ID points to a vector of weights pre-stored at L1, i.e. a row
  of the digital beam table.

Beam IDs are therefore limited to the range [0, 32767], which is checked when
the FAPI value is built.

Aerial does not implement this part of the beam-ID definition and expects a
plain index into the beam table it was configured with, so the MSB is masked
off when packing towards it. A native L1 currently handles only the MSB-set
case and asserts otherwise, consistently with the digital beam table being
restricted to Aerial.

## L1 implementation

The total number of logical antenna ports available at L1 is same as
`pusch_AntennaPorts * beams_per_period` in UL and `pdsch_AntennaPorts_N1 *
pdsch_AntennaPorts_N2 * pdsch_AntennaPorts_XP * beams_per_period` in DL. To
handle multiple concurrent beams, L2 uses spatial stream indices specified by
FAPI to signal L1 on which logical ports to use for a DL or UL signal. The
config file parameter `spatial_stream_index` can be used to specify an array of
logical port indices to be used. If this parameter is not provided then the
indices defaults to `[0 ... pusch_AntennaPorts - 1]`. This parameter is
particularly useful when a specific subset of eAxCID has to be used. In case of
DAS (`bf_method = "das"`), since each beam corresponds to a specific antenna
port, the `beam_index_allocation` function is simplified in the sense that the
beam index corresponds to the antenna port index of the frequency domain
buffers.

## RU implementation

The implementation is still work in progress.

The first dimension of the Tx and Rx buffers contains the number of Tx/Rx
antennas which is at least the number of logical antenna ports.
