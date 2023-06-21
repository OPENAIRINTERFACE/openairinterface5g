# Configuration files: naming style guide

The configuration files should have the following name format:

```
<type>[.<sa|nsa>][.band<num>.[u<num>.]<num>prb].<sdr>[.<opt>].conf
```

Where:
- `[]` means this field is optional
- `<type>`: one of `gnb`, `enb`, `gnb-cu`, `gnb-du`, `nrue`, `lteue`, etc.
- `<sa|nsa>`: in the case of 5G, notes whether this is SA or NSA
- `<num>` for `band`/`u`/`prb`: numerical value. Note that band/PRB are only
  applicable in some cases. The numerology `u` is only to be set for 5G if it
  differs from `u1`.
- `<sdr>`: what SDR board/split this is tailored for:
  * e.g., `usrpb210`, `usrpn310`, `rfsim`, `aw2s`
  * if there is no SDR (e.g., for a CU), use the south-bound split, e.g., `f1`, `if4p5`, etc.
  * for the L2simulator, write `l2sim`
- `<opt>`: optional specifiers. If there are multiple, concatenated
  using a dash (`-`). Examples:
  * `2x2`
  * `prs` (positioning reference signal)
  * `ddsuu` (specific TDD pattern)
  * `tm2`: Transmission Mode 2 in 4G
  * `oaiue`: specifies usage with OAI UE and not COTS UE

Examples:
- standard monolithic gNB, 2x2 configuration, with SDAP: `gnb.sa.band78.162prb.usrpn310.2x2-sdap.conf`
- gNB-CU: `gnb-cu.sa.f1.conf`
- monolithic gNB, L2sim: `gnb.sa.band77.106prb.l2sim.conf`
- monolithic eNB: `enb.band7.25prb.usrpb200.conf`
- RCC (4G eNB with IF4.5 split): `enb-rcc.band7.25prb.tm1.if4p5.conf`

Notes:
- there is no need for TDD/FDD, this is encoded in the band number
- there is no need for FR1/FR2, this is encoded in the band number
