# PHY and MAC Interface
The PHY sends scheduling requests and data indication to MAC via `nr_ue_dl_indication` for DL path and `nr_ue_ul_indication()` for UL path and the MAC sends scheduling configuration to PHY via `nr_ue_scheduled_response()`. The following diagram shows the interaction for PDCCH and PDSCH reception.
```mermaid
sequenceDiagram
    PHY->>+MAC: Requests for PDCCH config (via nr_ue_dl_indication)
    MAC->>+PHY: Schedules PDCCH reception (via nr_ue_scheduled_response)
    PHY->>+MAC: Indicates decoded DCI(s) (via nr_ue_dl_indication)
    MAC->>+PHY: Schedules PDSCH reception (via nr_ue_scheduled_response)
```

# Multi-threading Design
The `UE_thread` function in `nr-ue.c` is the main top level thread that interacts with the radio unit. Once the thread spawns, it starts the 'Initial Synchronization'. Once its complete, the regular processing of slots commences.

The UE exits when at any point in operation it gets out of synchronization. When the command line option `--non-stop` is used, the UE goes to 'Initial Synchronization' mode when it loses synchronization with gNB. However, this feature is not fully implemented and it is a work in progress at the time of writing this documentation. This will be the default behavior (not a command line option) when the feature is fully implemented.

## Initial Synchronization Block
```mermaid
graph TD
    start(Start) -->|UE_thread| rxRu["RU read<br/>(Reads two frames)"]
    rxRu --> |Tpool thread| sync["SSB detection<br/>(PSS & SSS detection<br/>PBCH decoding<br/>SIB decoding)"]
    rxRu --> |UE_thread| rxRuDummy["RU read<br/>(Dummy read till sync detection to avoid buffer overflow at radio)"]
    sync --> |Tpool thread| frameSync["Frame synchronization<br/>(Shift received samples to align with frame)"]
    rxRuDummy --> |UE_thread| frameSync
```
## Regular Slot Processing
```mermaid
graph TD
    sync["Frame synchronization<br/>(Shift received samples to align with frame)"] -->|UE_thread| hw_read["RU read (slot n)"]
    hw_read --> |UE_thread| rxPreProc["PBCH & PDCCH decoding (slot n)"]
    hw_read --> |Tpool thread| txProc["Tx processing (slot n+3)<br/>PUSCH encode<br/>PUCCH encode (wait for DLSCH in slot n+3-k1 to finish)<br/>RU write"]
    rxPreProc --> |Tpool thread| rxProc["PDSCH decoding (slot n)"]
    rxPreProc --> |UE_thread| join(Merge)
    txProc --> |Tpool thread| join
    join --> |Go to next slot<br/>UE_thread| hw_read
```
