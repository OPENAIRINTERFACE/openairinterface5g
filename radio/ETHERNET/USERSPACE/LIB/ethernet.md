# Ethernet drivers 

This directory contains ethernet-based drivers for fronthaul. The only functional versions today are ECPRI time-domain (split 8) over UDP for AW2S and IF4.5 (split 7) for OAI RU over UDP. The RAW ethernet implementation is currently not tested and quite possibly obsolete.


## license
    Author:
      Raymond Knopp, EURECOM 
      OAI License V1.1
# Top-level

The files implement the OAI IF device interface which provides the transmit/receive (trx) functions for generic ethernet-based devices. Some minimal control-plane socket handling for IF4p5 is also provided. It comprises the following top-level functions (ethernet_lib.c) which are mapped to the OAI device structure:

* trx_eth_start       : create sockets and threads to handle I/O
* trx_eth_end         : close sockets
* trx_eth_stop        : stops 3rd party (AW2S) device
* trx_eth_set_freq    : empty
* trx_eth_set_gains   : empty
* trx_eth_get_stats   : empty
* trx_eth_reset_stats : empty
* trx_eth_write_init  : empty
* ethernet_tune       : write certain performance-related parameters to ethernet device
* transport_init      : entry routine to fill device data structure with function pointers 


# IF4p5 U-plane implementation

It is contained in the eth_udp.c and eth_raw.c files. The two basic routines are

* trx_eth_read_udp_IF4p5() : implements a blocking read for three particular IF4p5 packets, IF4p5_PULFFT (for OAI RCC/DU), IF4p5_PRACH and IF4p5_PDLFFT (for OAI RU). The packets are parsed and mapped to the appropriate physical channels by the OAI physical layer
* trx_eth_write_udp_IF4p5 : implements a write for the three IF4p5 packets.

* trx_eth_ctlsend_udp : implements the sending component for the control socket

* trx_eth_ctlrecv_udp : implements the receive component for the control socket (blocking read)

# ECPRI U-plane implementation 

It is contained in the eth_udp.c file and only implements ECPRI/UDP. The implementation has 2 top-level functions:

* trx_eth_write_udp : This sends a stream of packets containing 16-bit I/ 16-bit Q samples to the ECPRI RU. It uses the proprietary AW2S format (user-defined). Each packet carries 256 samples and some header information (timestamp, antenna index) and fits inslightly more than 1024 bytes. Each packet with Ethernet, IP and UDP headers fits into a regular Ethernet frame. The transmit function does not block and pushes the data to a worker thread (udp_write_thread) which runs in the background. The worker thread scales the IQ samples to fit in 16-bit units and forms the ECPRI packets for each antenna. The signals are split into 256-sample chunks and antennas are handled in sequence for each chunk. This ensures that all antennas receive their signals more or less at the same time. 

* trx_eth_read_udp : This is a blocking read which waits for a worker thread (udp_read_thread) listening on the U-plane socket to have acquired enough samples satisfying the request (nsamps). The requests are usually a slot (NR) or subframe (LTE) worth of samples. The worker thread identifies the antenna index (aid) and timestamp of each packet and copies the received chunk into the destination memory location according to the timestamp modulo the length of the receive buffer. The memory location for read is identifited during initialization of the interface and is written circularly. Typically the buffer would contain a frame's worth of samples, but this is not a requirement. There is only one memcpy in the driver and the end OAI application can use the data in its normal local buffer (RU.common.rxdata[aid]). The memcpy is needed since the destination address depends on the antenna index which is carried in the received packet itself. Kernel filtering and redirection (C/EBPF,XDP) doesn't seem to be possible with the AW2S packet format.

The two threads should be pinned to isolated CPUs to maximize performance. Their CPU id's can be passed to the driver from the OAI application. tx_fh_core and rx_fh_core in the RU section of the OAI .conf file.

# Obsolete code

Non ECPRI split 8 and IF4p5 with RAW sockets is not functional anymore (eth_raw.c). split-8 will be resusitated if an RU implementing ECPRI is integrated with OAI. IF4p5 with raw sockets is replaced with the FHI Split 7.2 interface.
 

