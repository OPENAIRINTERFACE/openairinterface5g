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
    A(Start) -->|UE_thread| B["readFrame<br/>--Reads samples worth 2 frames"]
    B --> |Tpool thread| C["UE_synch<br/>--PSS & SSS detection<br/>--PBCH decode"]
    B --> |UE_thread| D["readFrame<br/>--trash samples to unblock radio"]
    C --> |Tpool thread| E[syncInFrame<br/>--shift first sample to start of frame]
    D --> |UE_thread| E
```
## Regular Slot Processing
```mermaid
graph TD
    E[syncInFrame<br/>--shift first sample to start of frame] -->|UE_thread| F["trx_read_func (slot n)"]
    F --> |Tpool thread| G["processSlotTX (slot n+4)<br/>--PUSCH encode<br/>--PUCCH encode<br/>--trx_write_func"]
    F --> |Tpool thread| J["UE_processing (slot n)<br/>--PDCCH decode<br/>--PDSCH decode"]
    F --> |UE_thread| I(Merge)
    G --> |Tpool thread| I
    I --> |Go to next slot<br/>UE_thread| F
```
