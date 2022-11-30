# Multi-threading Design
The `UE_thread` function in `nr-ue.c` is the main top level thread that interacts with the radio unit. Once the thread spawns, it starts the 'Initial Syncronization'. Once its complete, the regular processing of slots commences.

## Initial Syncronization Block
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
    F --> |UE_thread| H["UE_processing (slot n)<br/>--PDCCH decode<br/>--PDSCH decode"]
    G --> |Tpool thread| I(Merge)
    H --> |UE_thread| I(Merge)
    I --> |Go to next slot<br/>UE_thread| F
```
