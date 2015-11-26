/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
#ifndef __PHY_DEFS_SPEC_H__
#define __PHY_DEFS_SPEC_H__

#include "types.h"


/*! \mainpage OpenAirInterface.org Specifications

\section scope_openair Scope

The present document specifies the architecture of the following components of a OpenAirInterface network
-# \ref _RN_TOPOLOGY_
-# \ref _PHY_PROCEDURES_MESH_
-# \ref _PHY_PROCEDURES_CELL_
-# \ref _L2_PROTOCOLS
\subsection _RN_TOPOLOGY_ Radio Network Topology and Components
This section describes the different components which constitute the OpenAirInterface. Two types of network topologies are supported, mesh and cellular.
The mesh topology is depicted as:

\image html mesh_topology_small.png "OpenAirInterface Mesh Topology" width=5cm
\image latex mesh_topology.pdf "OpenAirInterface Mesh Topology"

The cellular topology is depicted as:

\image html cellular_topology_small.png "OpenAirInterface Cellular Topology" width=5cm
\image latex cellular_topology.pdf "OpenAirInterface Cellular Topology"

The mesh topology contains Clusterheads (CH) and Mesh Routers (MR) whereas the cellular topology contains Clusterheads/NodeB (CH/NodeB) and User Equipment (UE).
The main difference at the physical layer between the two topologies is that direct communications between UE is not permitted in a cellular topology.  Other major
differences exist at layers 2 and 3.  In both topologies, MR/UE can be connected to more than one CH at any time, if radio connectivity is possible.

The two types of physical networking devices are specified as follows:

- Cluster-head (CH or NodeB): A cluster-head can be defined as a node with a maximum visibility/connectivity in terms of number of nodes in its neighborhood.
From the point-of-view of the MAC layer, it assumes the fine-grain management of radio resources in the cluster (cell). It determines the frame partitioning
(see \ref _framing_modulation) and bandwidth allocation and communicates this information to nodes in the cluster through a beacon. At the
physical layer, the cluster-head provides mechanisms for timing and carrier frequency synchronization. The primary role of the CH is to manage radio resources in their cluster.  The cluster is defined as the set of nodes which are characterized by one-hop connectivity with the clusterhead.  One-hop connectivity is further defined as the capacity to reliably receive and transmit basic signaling channels with the clusterhead using at least at the lowest datarate communication mode. Reliable communication is defined by a transmission which falls below a maximal error probability threshold.
CH can only be connected to MR on the same frequency-carrier since they use the same temporal resources as other CH.  Thus direct CH<->CH communication is not possible on the same frequency carrier.
The downlink (CH -> MR) signaling channels allow for the CH to schedule transmission of labels (in the form of time and frequency mappings on the radio resource) which each carry different types of traffic throughout the mesh network.  The Uplink (UL) signaling channels (MR -> CH) are used for relaying bandwidth requirement indicators and channel quality measurements from nodes within the cluster.  These feed the scheduling algorithms residing in the CH and allow for proper resource allocation satisfying quality-of-service (QoS) negotiations carried out using Layer 3 (L3) signaling.

The CH further provides mechanisms for measurement reporting to L3 (for routing, QoS management, labeling, etc...).  This is achieved by a set of signaling channels which relay measurement information (UL) for the nodes in the cluster to the CH.  The CH processes these raw measurements into a form which is expected by L3 mechanisms.
Some CH can assume the role of network synchronization by sending special synchronization pilots (see Section 1.1.5).  These will be called Primary CH when network synchronization is achieved using this method.  Other CH using this method are called Secondary CH.

 - Regular node/Mesh Router (UE): All nodes have the ability to play at the same time the role of a host and of a router, although this functionality is not activated
in a cellular topology.

The primary role of an MR is to interpret the scheduling information from the CH on the DL signaling channels in order to route the traffic corresponding to the
scheduled labels on the allocated physical resources.  MR can be connected to other MR (direct link) in the same cluster. MR can also be connected to more than one cluster
at the same time. It is also expected to using the UL signaling channels to relay measurements to the CH with which is connected.
A secondary role of some MR is to search, on behalf of the CH, for isolated nodes which need to be connected to the mesh. These MR use a special signaling resource (MRBCH) to
exchange basic topological parameters with the IN which then results in overall network topology updates.  If several IN are contending for access with the cluster,
joint processing of the requests will be performed by the mesh during topology adjustments.  The most likely nodes to assume this role will be those at the extremities
of the mesh.

Either type of equipment can also assume the role of relay/gateway to a secondary network. `

In mesh topologies some nodes (CH or MR) assume the role of and edge router (from a layer 2/3 perspective). An edge router is either a CH or MR with an IP interface to
another network. The role of ER is to aggregate traffic (ingress) from IP flows to MPLS-like labels for transmission in the mesh.  On reception it must demultiplex
traffic (degress) from MPLS-like labels to IP for traffic exiting the mesh.  Edge routers, potentially all CH and MR, must have MAC-layer interfaces to IP in order to
perform these functions.

Both OpenAirInterface topologies require Network Synchronization (NS) at least between adjacent clusters.  This must be on the order of a few microseconds.  Three mechanisms are
supported to ensure NS.  Firstly, a secondary synchronization source (e.g. GPS) can be used as a common time reference by all nodes.  Secondly, one CH (Primary CH) in the
network use a special synchronization signal which has longer range than the range of communication, in order to cover the region which a common time reference.  This is
suitable for small networks.  Finally the method of distributed relaying of synchronization is possible.  This is a method by which all nodes propagate a time reference.  Nodes
switch between reception (for timing acquisition and tracking) and transmission of the reference. This guarantees coverage of network synchronization over long distances
in the absence of a secondary synchronization source.


\subsection _L2_PROTOCOLS  Layer 2 Protocols
Layer 2 is structured as below. It comprises:
- A IP/MPLS networking device (NAS DRIVER) responsible for provision of IP/MPLS layer services to Layer 2 and vice-versa
- An MPLS label-switching entity (NAS MPLS) responsible for routing/forwarding within the mesh network (MESH topology only)
- A Radio resource control (RRC) entity responsible for MAC layer signalling for configuration of logical flows (labels) and retrieval of measurement information.
- A Radio Link Control (RLC) entity which is responsible for automatic repeat request protocols (ARQ) and IP/MPLS packet segmentation
- A convergence protocol (PDCP) responsible for IP interface and related functions (header compression, ciphering, etc.)
- A scheduling and multiplexing unit (\ref _openair_mac_layer_specs_) responsible for the mapping between logical channels (labels and control-plane signalling)
and transport channels.  It implements the interface with the PHY, which is the collection of transport channels as well as a primitives for
collection of PHY measurements and configuration of PHY parameters.

\image latex layer2_stack_overview.pdf "Global View of OpenAirInterface Protocol Stack and Communication Primitives"
\image html layer2_stack_overview.png "Global View of OpenAirInterface Protocol Stack and Communication Primitives"

These entities are described in the following subsections.

\subsubsection _RRC_ Radio Resource Control (RRC)
The radio resource control entity is responsible for the L2 signalling implementing the radio channels establishment. It also implements the control of
measurement procedures described in Section 1.3.4. Its internal state machine controls the basic procedures for startup, monitoring of synchronization
through the measurement system and update of the nodes role in the network (Sections 1.3-0).

RRC is responsible for configuration of all MAC entities (and PHY via MAC), both dynamic (during label establishment) and static (control channels).  This functionality
is in response to event occurring in the interaction with L3 and based on dynamic measurements of radio quality.

RRC signalling makes use of DCCH, CCCH and BCCH for transport of the various protocols.


\subsubsection _RLC_ Radio Link Control (RLC)
RLC segments IP packets. The segment size is configurable for each QoS class and is signalled by higher layers during route establishment.  The sizes
are chosen based on the granularity of the underlying MAC/PHY resources (transport blocks).

RLC is responsible for ARQ and indexing of SDUs from the user traffic and signalling SDUs from RRC.  The SDU inputs from LS form the
set of radio bearers, and those from RRC the set of signalling radio bearers. It has two modes of functionality: acknowledged and unacknowledged.
Each logical channel can have an associated ARQ process which is managed by RLC.   The ARQ mechanisms are based on Release 6 3GPP RLC (25.8xx).
The interface with RRC for configuration is not yet described.  The interface with MAC is designed such that data for each logical channel is
buffered in data queues, whose occupancy can be measured by the MAC scheduling entity.

\subsubsection _MAC_ MAC scheduling Entity (MAC)
The MAC entity is responsible for scheduling control plane and user-plane traffic on the physical OFDMA resources.
On transmission, the inputs to this entity are connected to data queues originating in the RLC layer which form the set of logical channels. The
control plane traffic is represented by logical channels which form the interface with the RLC.  Logical channels contain both user-plane (originating
in the IP/MPLS entity via the PDCP entity) and control-plane traffic (originating in the RRC entity).  MAC layer specifications are found in \ref _openair_mac_layer_specs_.
The MAC is responsible the transport channel interface which exchange data (MAC SDUs) and PHY measurement data for RRC measurement procedures.

\subsection _PHY_PROCEDURES_MESH_ Physical Layer Procedures - Mesh Topology
\subsection _PHY_PROCEDURES_CELL_ Physical Layer Procedures - Cellular Topology

\subsubsection _POWER_ON_CH_ Power-on procedures of a CH
This clause briefly describes the power-on procedure of a CH. The CH RRC receives basic cell configuration information (CHBCH/RACH configuration) from
L3 Radio Resource Management (RRM)  and configures the MAC and RLC layers.  The MAC, in turn, configures the static paramters of the PHY. Upon completion
of this initialization phase it enters the steady-state mode.

\subsubsection _POWER_ON_UE_ Power-on procedures of a UE
This clause briefly describes the power-on procedures of a UE which is managed by the UE-RRC state machine.  The first function is to search for at least one
existing CH in range which is under the responsibility of the PHY.  This procedure attempts to analyze the received signal power over a pre-defined time period. If a
signal is detected on the desired carrier, the node attempts PHY synchronization using the pilots of the candidate CHs.  The postulated frame start position is used
to demodulate the CHBCH resources.  If the PHY returns an error-free CHBCH with acceptable receive quality, the node is said to be pre-synchronized to the CH.
It then attempts to decode the CHBCH of the rest of the CH in range.  At the end of this stage, it has a list of acceptable CH.  This procedure is repeated periodically,
until active communication is sought with the network.

Upon passing to the state of active communication, it demodulates the CHBCH continuously from the CHs to obtain the UL-CCCH configurations as well as MCCH/MTCH.
Once configured, it attempts to establish a connection (connection request) with each of the candidates using the RACH resources of each CH.  Upon successful completion
of the association procedure it is said to be synchronized to the CH and enters the steady-state mode for each CH.

\subsubsection _CH_SS_OP_ CH Steady-state operation
In TTI N, the CH transmits all DL flows as determined by the CHBCH scheduler during the end of TTI N-2. Furthermore, it detects the UL-SACCH for all UE flows and the
UE-SACH for all flows.  It also detects the RACH (CCCH). At the end of each TTI, once it has received all feedback indicators (channel and queuing), it invokes
the CHBCH scheduling entity to determine the allocations for TTI N+2.  The sequence of operations after receiving flows from TTI N (towards the end of TTI N+1) is:

Entity: RRC TX

-#  generate BCCH and CCCH to be conveyed in TTI N+2
-#  program new logical channels and measurement procedures starting in TTI N+2 based on L3 signaling requests
-#  generate signaling radio bearers (measurement requests, UE radio bearer/logical channel configuration) for TTI N+2, and invoke RLC

Entity: MAC TX

-#  Invoke MAC Scheduler for TTI N+2 allocations : compute DL_SACCH_PDU and UL_ALLOC_PDU for N+2, corresponding to  PHY allocations in N+3.
-#  Generate CHBCH PDU for TTI N+2
-#  Retrieve RLC SDUs and generate DL_SACH for TTI N+2
-#  Generate MACPHY_DATA_REQ for TX transport channels in TTI N+2

MAC RX

-#  Generate MACPHY_DATA_REQ for RX transport channels in TTI N+2 (based on previously scheduled UL_ALLOC_PDU received by UE in TTI N, decoded by end of TTI N+1)
-#  Invoked by PHY through macphy_data_confirm, the received flows are routed to RLC data queues and MAC signaling information is
stored for MAC TX scheduling in next TTI.  The MACPHY_DATA_IND primitive (invoked by PHY) also provides CH RX measurements information in an UL_MEAS structure.

RRC RX

-#  Retrieve RACH and process association requests
-#  Retrieve signaling radio bearers from RLC
-#  Retrieve CH RX measurements from PHY/MAC

This sequence is invoked at the end of each TTI by the system scheduler.

\subsubsection  _UE_SS_OP_ UE Steady-state operation

In the steady-state of TTI N, the UE PHY demodulates the CHBCH.  The CHBCH PDU is then available during TTI N+1 for the MAC. to determine the allocations of the CH and
itself in TTI N+2. Based on the decoded information, its scheduling entity generates the transmission for the next TTI and configures the PHY to demodulate the data for
which it is destination in the current TTI.  The UE RRC acts on PHY/MAC measurements to maintain proper synchronization and received signal quality, for example by
detecting a loss of connection of degradation of service. The sequence of operations at the end of TTI N is

Entity: MAC RX

-#  Parse CHBCH_PDU
-#  Generate macphy_data_req for RX transport channels in TTI N+2 (based on previously scheduled UL_ALLOC received by UE in TTI N, decoded by end of TTI N+1)
-#  Invoked by PHY through macphy_data_confirm, the received flows are routed to RLC data queues and MAC signaling information is stored for MAC TX scheduling in next TTI.  The reported PHY RF measurements (i.e. in DL_MEAS structure) are processed and used to generate UL_SACCH_FB.
-#  Process measurements for RRC measurement reports and invoke mac_meas_ind for each logical channel requiring a measurement report.

Entity: RRC RX

-#  Retrieve BCCH  and CCCH and generate association requests
-#  Retrieve signaling radio bearers from RLC
-#  Retrieve UE RX measurements from PHY/MAC for L3 measurement reporting

Entity: RRC TX

-#  program new logical channels and measurement procedures starting in TTI N+2 based on L3 signaling requests signaled by CH-RRC
-#  generate signaling radio bearers (measurement reports, configuration ACKs) for TTI N+2, and invoke RLC

Entity: MAC TX

-#  Invoke MAC multiplexer for TTI N+2 allocations : compute UL_SACCH_FB and UL_SACCH_PDU for N+2, corresponding to  PHY allocations in N+3.
-#  Generate CHBCH PDU for TTI N+2
-#  Retrieve RLC SDUs and generate UL_SACH for TTI N+2
-#  Generate MACPHY_DATA_REQ for UL_SACH in TTI N+2

\subsubsection _QOS_MEAS_PROC_ QoS Measurement Procedures

CH RRC manages L3 measurement reports at L2 for nodes within the cell. Measurement reports are exchanged between UE and CH using a logical channel (DCCH)
for topological control signaling, and edge routers can provide these measurements to IP
Since the CH scheduler has access to low-level PHY measurements, the MAC layer is responsible for measurement reporting on behalf of the PHY and itself.
The CH obtains raw measurements of all links in the cell.  RRC acquires these measurements from MAC scheduling entity.
Measurements are processed in nodes to the degree required for higher level services. For example nodes will extract link quality (rate/delay) indicators from
low-level services (MAC to L3 measurement messages) which are transported using special signaling flows offered by the MAC. This is then used for L2.5 topology
maintenance (radio-bearer (re)-assignment). Edge routers will extract L2.5 measurement information on labels to provide IP with quality indicators.

The interface with RRC for measurement reports is very similar to existing Release 6 HSPA. The types of measurements are:
- periodic (or one-shot) with configurable reporting interval and total number of measurements
- event-driven - in order to handle degradation of QoS level or loss of connection.

The available measurements for CH RRC(L2) are:
- RSSI (dBm) on physical resources corresponding to logical channel.
- Average SINR (dB) on physical resources corresponding to logical channel.
- Average number of transmission rounds (times 10) on transport channel associated with logical channel.
- Average residual block error rate (times 1000) on transport channel associated with logical channel (after HARQ!).
- Actual Spectral efficiency (bits/symbol times 10) of transport channel associated with logical channel.
*/

/** @defgroup _openair_specs_ OpenAirInterface Layer1/2 Specifications
* @defgroup physical_layer_ OpenAirInterface Physical Layer (PHY) Specifications
* @ingroup _openair_specs_
* @{
This clause specifies the PHY layer for a multiple-antenna orthogonal frequency-division multiple-access (OFDMA) system applied and
the generic openair MAC interface.
\image html PHY_arch.png "openair PHY"
\image latex PHY_arch.png "openair PHY" width=15cm
The specification is intended for reconfigurable equipment, so that actual parameters and mechanisms can be configured
prior to deployment of the equipment or potentially over-the-air, although the latter is not yet supported by any of the openair MAC implementations.  The
PHY and MAC layers are tightly coupled so that the MAC entity can directly influence the occupied physical resources. The OFDMA system provides the means for
transmitting several multiple-bitrate streams (multiplexed over sub-carriers and antennas) in parallel. Moreover, PHY signaling strategies are included to
provide the means for exploiting channel state feedback at the transmitters in order to allow for advanced PHY allocation of OFDMA resources via the MAC.
Modulation and channel coding formats are generic allowing for specific techniques to be employed in different deployment scenarios although configurations
based on the 802.11a legacy standard (binary/quaternary phase shift keying BPSK/QPSK, 16-point quadrature amplitude modulation (16-QAM),
64-QAM, rate 1/2,2/3,3/4 punctured convolutional codes) are provided here. The specifications are not specific to any frequency band or bandwidth,
although the minimum expected channel bandwidth should not be less than 1 MHz.
*/

/** @defgroup _phy_scope Scope
* @ingroup physical_layer_
* @{
This subclause describes the PHY services provided to the openair MAC.  The openair PHY consists of several protocol functions which provide the interface
between the MAC and PHY for allocation MAC layer PDUs to physical resources.  Physical channels are used to convey signaling, data and training information
across the radio medium.  OpenAirInterface implements the following physical channels:

-# The PCHSCH (Physical Clusterhead Synchronization Channel) is a pilot resource reserved to a clusterhead (CH) which is responsible for
delivering synchronization information to nodes in the cluster.  This channel is used by nodes to acquire timing information regarding the
beginning of the TTI and to perform initial frequency offset adjustments with respect to the carrier frequency of the CH.  The channel
is also used by adjacent clusterheads to synchronize the network, in order to facilitate inter-cluster communication under quality-of-service guarantees.
-# The PCHBCH (Physical Cluster-head Broadcast Channel) is a signaling resrouce reserved to a clusterhead which is responsible for delivering layer 2/3
protocol information to the nodes of the cluster.  It can also used by the nodes in the cluster to acquire accurate timing and frequency synchronization information.
-# the PRACH (Physical Random Access Channel) is a signaling resource used by a node to provide layer 2 protocol information to its clusterhead.
-# the PCHSCH (Physical Clusterhead Synchronization Channel) is a pilot resource used by a CH to allow the UE/MR to estimate the channel of CH and to acquire timing synchronization.
-# the PSCH (Physical Synchronization Channel) is a pilot resource used by a node to allow the CH to estimate the channel of an MR/UE.
-# the PSACH (Physical Scheduled-Access Channel) is a multi-cast data resource used by a node or CH to send MAC data PDUs to one or more destinations in
parallel (using multi-antenna OFDMA).
-# the PSACCH (Physical Scheduled-Access Control Channel) is a signaling resource used by a node to provide MAC protocol information to the destinations of its
transmissions as well as the CH.
-# the PMRBCH (Physical Mesh Router Broadcast Channel) is a signaling resource used by a node to provide layer 2 broadcast protocol information to neighbouring nodes
outside the range of clusterheads.  It is used by nodes in mesh network topologies to relay network synchronization and provide initial configuration information to

Transport channels constitute the control and user plane interfaces between the MAC and PHY layers.  The are used to exchange both data and measurement information
and are mapped onto the above physical channels.  OpenAirInterface implements the following transport channels:

-# The CHBCH (Clusterhead Broadcast Channel) is the transport channel mapped to the PCHBCH.
-# The RACH (Random-Access Channel) is the transport channel mapped to the PRACH.
-# The DL-SACH (Downlink Scheduled-Access Channel) is the transport channel mapped to the PSACH.
-# The UL/MR-SACH (Uplink or Mesh-Router Scheduled-Access Channel) is the transport channel with components mapped to both the PSACH and PSACCH.
-# The MRBCH (Mesh-Router Broadcast Channel) is the transport channel mappted to the PMRBCH.

During reception, all transport channels convey measurement information.
* @}
*/

/** @defgroup _phy_framing Framing and Channel Multiplexing
* @ingroup physical_layer_
* @{
*/


/** @defgroup _framing_modulation TTI and Modulation Parameters
* @ingroup _phy_framing

The physical layer uses OFDM symbols organized into frames (corresponding to TTIs) of complex baseband samples at a sampling rate of \f$ f_\mathrm{s} \f$ samples/s.  The carrier frequency is denoted \f$f_c\f$. Each TTI is made up of \f$N_{\mathrm{symb}}\f$ OFDM symbols. OFDM symbols,  \f$\mathbf{s}\f$, of length \f$N_\mathrm{s}\f$ samples contain two distinct parts,\f$\mathbf{s}_\mathrm{I}\f$ and \f$\mathbf{s}_\mathrm{E}\f$.

OpenAirMesh framing is completely configurable, but the nominal OFDMA configuration is shown below

\image html mesh_frame.png "OpenAir PHY Framing"
\image latex mesh_frame.pdf "OpenAir PHY Framing" width=15cm

One frame consists of 64 OFDM symbols and is  divided in a CH transmission time interval (TTI) and a MR TTI. The first four symbols of the CH TTI are reserved for pilot symbols. Each CH transmits one common pilot symbol (CHSCH$_0$) at position 0 and one dedicated pilot symbol (CHSCH\f$_i\f$) at position \f$i \in \{1,2,3\}\f$. This way we can ensure orthogonality between the pilots of different CH received at one MR. The pilot symbols are followed by the broadcast channel (CH-BCH).  The rest of the CH TTI frame is reserved for the multiplexed scheduled access channels (CH-SACH).

The MR TTI contains the random access channel (MR-RACH) with an associated pilot symbol (SCH$_0$). The next two symbols are reserved for pilots. Each MR transmits a pilot symbol SCH\f$_i\f$, \f$i \in \{1,2\}\f$ corresponding to the cluster it belongs to. The pilot symbols are followed by the uplink broadcast channel (MR-BCH) with an associated pilot symbol (MRSCH). The rest of the uplink frame contains the multiplexed scheduled access channels (MR-SACH). The end of the CH and MR TTIs are protected by a guard interval of two symbols. All pilots are designed for MIMO and/or Multiuser channel estimation at the corresponding end.

MAC PDUs arrive at the MAC interface from different logical resources (control, broadcast, multiple-user data streams) in parallel at the start of each TTI and must be mapped to the available radio resources.  Each PDU is scrambled, encoded with a CRC check, and encoded using a channel code with associated bit-interleaving. The output of the channel coding block contains the information content to be transfered across the channel via the modulator. The modulated information content, \f$\mathbf{s}_\mathrm{I}\f$, is built starting either from a frequency-domain signal (classical OFDM) or several time-domain signals (digital FDM). Both techniques yield what are denoted herein as OFDM symbols. In the first method (classical OFDM), \f$\mathbf{S}_\mathrm{I}\f$ is specified in the frequency-domain and is made up of \f$N_\mathrm{d}\f$ samples. This is transfered to the time-domain via the inverse discrete-time Fourier transform (DFT) yielding a time-domain signal also of length \f$N_\mathrm{d}\f$ samples, \f$\mathbf{s}_\mathrm{I}=\mathrm{idft}(\mathbf{S}_\mathrm{I})\f$. In the second method, up to \f$N_\mathrm{f}\f$ different time-domain signals each comprising \f$N_{\mathrm{d,2}}=\frac{N_\mathrm{d}}{N_\mathrm{f}}\f$ samples, where \f$N_\mathrm{f}\f$ denots the number of frequency groups making up an OFDM symbol.  Here each signal \f$\mathbf{s}_{i}\f$ is transformed to the frequency-domain via an \f$N_\mathrm{d,2}\f$-dimensional DFT yielding \f$\mathbf{S}_i\f$ and the combined frequency-domain signal is the concatenation of the \f$\mathbf{S}_i\f$, \f$\mathbf{S}_\mathrm{I} = [\mathbf{S}_0 | \mathbf{S}_1 | \cdots | \mathbf{S}_{N_\mathrm{f}-1}]\f$

The redundant (or null) portion, \f$\mathrm{s}_\mathrm{E}\f$, comprises \f$N_\mathrm{c}=N_\mathrm{s}-N_\mathrm{d}\f$ extra samples, and is concatenated to \f$\mathbf{s}_\mathrm{I}\f$. It is either a cyclic extension or zeros.  The overall symbol is \f$\mathbf{s} = [\mathbf{s}_\mathrm{E} | \mathbf{s}_\mathrm{I}]\f$ (prefix configuration) or \f$\mathbf{s} = [\mathbf{s}_\mathrm{I} | \mathbf{s}_\mathrm{E}]\f$ (suffix configuration).

The cyclic prefix or zero-padding is used to absorb a channel with a delay spread (including propagation delay of the primary paths) equal to its length so that adjacent OFDM symbols do not overlap in time.  If the cyclic prefix method is used, then \f$s_{\mathrm{E},i} = s_{\mathrm{I},N_\mathrm{d}-N_\mathrm{c}+i}, i=0,\cdots,N_\mathrm{c}-1\f$, whereas if the cyclic suffix method is used, then \f$s_{\mathrm{E},i} = s_{\mathrm{I},i}, i=0,\cdots,N_\mathrm{c}-1\f$ otherwise \f$s_{\mathrm{E},i}=0, i=0,\cdots,N_\mathrm{c}-1\f$. The value \f$N_\mathrm{c}\f$ should be chosen based on the maximum propagation delay in the system.  For outdoor channels this will be on the order of a few microseconds.  In addition, for large \f$N_\mathrm{c}\f$, the value of \f$N_\mathrm{d}\f$ should also be large so that the overhead due to the propagation channel be kept to a minimum.  \f$N_\mathrm{d}\f$ should be large enough to allow for frequency-domain multiplexing of user data streams if OFDMA is employed.  Very large \f$N_\mathrm{c},N_\mathrm{d}\f$ are probably not required for openair except perhaps in the case of long-distance point-to-point links
(e.g. to link different hotspot areas).

OFDM symbols typically provide for a certain spectral roll-off to satisfy RF spectral mask requirements and aid in transmit filtering and adjacent channel suppression.  This is usually accomplished by inserting \f$N_{\mathrm{z}}\f$ zeros in \f$\mathbf{S}_f\f$. The total number of useful samples in \f$\mathbf{S}_f\f$ is therefore \f$N_\mathrm{d}-N_\mathrm{z}\f$.

For use in OFDMA multiplexing, the useful carriers can be split into \f$N_\mathrm{f}\f$ groups of contiguous carriers. Each group of carriers can be used to transmit a different data stream in the same OFDM symbol.  This particularly useful for achieving dynamic FDMA on the uplink of a cellular system.

A summary of the framing parameters is given in following table and is represented by the primitive PHY_FRAMING.  It represents part of the static configuration of the air-interface and is set during the initialization phase of the equipment via the MAC-layer interface (see \ref _mac_phy_primitives_,MACPHY_CONFIG_REQ) .

*/


/** @defgroup _chsch_sig Clusterhead Synchronization Channel (P-CHSCH,S-CHSCH) Signaling Format
* @ingroup _phy_framing

The clusterhead synchronization channel is a signaling channel generated in the PHY layer and is comprised of
\f$N_{\mathrm{s,CHSCH}}(N_\mathrm{d}+N_\mathrm{c})\f$ samples, or the equivalent of one \f$N_{\mathrm{s,CHSCH}}\f$ OFDM symbols.   The main purposes of
this channel are

-# Timing (TTI/symbol) synchronization for nodes inside the cluster
-# Frequency sychronization for nodes inside the cluster
-# Timing (TTI/symbol) sychronization for clusterheads in adjacent clusters
-# Frequency synchronization for clusterheads in adjacent clusters

It is emitted once per TTI in conjunction with the clusterhead broadcast channel (P-CHBCH,S-CHSCH).  Aside from its primary purposes above, it is also
intended to be exploited for channel estimation prior to demodulation of the CHBCH, since the CHBCH does not comprise pilot signals and the clusterhead
does not use the MCH. The number of symbols required, \f$N_{\mathrm{s,CHSCH}}\f$, depend on the desired time and frequency acquisition precision.

The x-CHSCH is a pseudo-random QPSK sequence defined in the frequency domain by the bit sequences
\f$\mathrm{Re}\{\mathbf{c}_i\}, \mathrm{Im}\{\mathbf{c}_i\}, i=0,1,\cdots,\lfloor(N_{\mathrm{d}} - N_{\mathrm{z}})N_{\mathrm{s,CHSCH}}/32 \rfloor -1\f$.
If multiple transmit antennas (up to \f$N_d/N_c\f$) are used on the same frequency carrier, the CHSCH sequence,
\f$\mathbf{c}_{\mathrm{CHSCH,2}}\f$, shall be cyclicly shifted by \f$iN_\mathrm{c}, i=1,2,\cdots,N_\mathrm{ant}\f$ samples on antenna \f$i\f$ prior to
cyclic extension. This is to allow nodes to estimate the different channels of the clusterhead efficiently in the frequency domain.

The transmit power of the CHSCH shall be adjustable by higher layers in order to control the detection range of the clusterhead.

The parameters of the CHSCH are summarized in the following table and represented by the primitive PHY_CHSCH

*/


/** @defgroup _CHBCH Clusterhead Broadcast Channel (P-CHBCH,S-CHBCH) Signaling Format
* @ingroup _phy_framing


The CHBCH is the signaling channel used by the DLC for passing basic protocol information from the clusterhead to the nodes in its
cluster.  This information is used to distribute physical resources during the TTI and some additional protocol information
(association, QoS reservation, etc.). It is located in the first symbol in the TTI,\f$s_{\mathrm{CHBCH}}\f$. In the case of several
clusterheads operating on the same carrier frequency, the CHBCH of adjacent clusters (i.e. those within range of the CHSCH) cannot collide
and thus must be allocated different symbols in the TTI or use disjoint frequency carrier sets. The same is true of the CHBCH and MCH/RACH of
adjacent clusters.  The time/frequency allocation of CHSCH/MCH/RACH across several clusters must be accomplished in a distributed fashion based on the
activation times of the different clusterheads and mobility of the clusters.  This is beyond the scope of this preliminary specification.

The CHBCH must use the lowest spectral efficiency (highest sensitivity) coded-modulation format in order to be detectable at large distances.
It will thus employ a rate 1/2 forward-error-correcting code with QPSK modulation \ref _phy_coded_modulation.  Information will be coded
across \f$N_{\mathrm{s,CHBCH}}-1\f$ OFDM symbols using all non-zero carriers.  Interleaving shall be performed across frequencies with depth
\f$IntDepth_{\mathrm{CHBCH}}\f$. \f$IntDepth_{\mathrm{CHBCH}}\f$ shall be an integer divisor of \f$N_{\mathrm{d}}-N_{\mathrm{z}}\f$. Prior to
forward-error correction coding, a CRC of length 32 bits shall be applied to the PDU arriving from the MAC layer interface.

Channel estimation can be obtained from the CHSCH which is located in the adjacent OFDM symbols.  The CHSCH symbols shall be found starting
in symbol number \f$\lfloor.5N_{\mathrm{symb}}\rfloor\f$ of the CHBCH.

The number of bits per TTI delivered by the MAC layer interface (FEC + CRC) bits is determined by the formula
\f$(N_{\mathrm{symb}}-1)*(N_{\mathrm{d}}-N_{\mathrm{f}}) - 32\f$.

If \f$N_\mathrm{pilot}\f$ additional pilot symbols per OFDM symbol are required to handle large frequency offsets due to high-mobility (Doppler)
or significant carrier frequency offsets due to the RF equipment,  then these are to be placed at equally spaced positions starting from the
first non-zero carrier in each CHBCH symbol.  These simply puncture the coded bit sequence.  Care must be taken when choosing the value of
\f$N_\mathrm{f}\f$ with respect to \f$IntDepth_{\mathrm{CHBCH}}\f$ so that consecutive bits of the encoded sequence are not punctured.
A judicious choice would take \f$IntDepth_{\mathrm{CHBCH}}\f$ and \f$N_{\mathrm{d}}-N_{\mathrm{z}}\f$ to be relatively prime.

In order to allow for multi-cell deployment the CHBCH can use a reduced set of subbands which is controlled by the parameter \f$FreqReuse\f$. It should be set to the maximum number of base stations. Let \f$N_{FreqGroup}\f$ be the number of carriers of one frequency group. Then, the set of carriers used by the \f$i^\mathrm{th}\f$ CHBCH \f$i=1,2,3\f$ (CHBCH 0 is unused) is given by
\f$\left\{((i-1) FreqReuse N_{pilot} ) + k*N_{FreqGroup}, k=0,\cdots,N_{Pilots}-1\right\}\f$.

The transmit power of the CHBCH shall be adjustable by higher layers in order to control the detection range of the clusterhead.  This value will be
transfered via higher layer signaling so that nodes may perform open-loop power control. The following table summarizes the parameters of the CHBCH
which are transfered from higher layers using the primitive PHY_CHBCH.

*/


/** @defgroup _RACH Random-Access Channel (RACH) Signaling Format
* @ingroup _phy_framing


The RACH is a signaling channel used only during the association phase of a node and for other management services.
Several RACHs can be used in parallel to reduce contention in dense networks, and the exact number are signaled by the higher layer.
This resource is not meant, however, to be used for intensive data transmission.  Data streams consisting of small sporadic packets
could potentially use this channel if required.

The RACH is subject to power control.  The transmit power should be adjusted in a closed-loop fashion based on the
measured path loss between the node and the clusterhead.

For adhoc/mesh configurations, one or more MCH are reserved for the RACH.  For a cellular
scenario, a SACH resource is opened periodically for the RACH by the clusterhead.

*/


/** @defgroup _SACH Scheduled-Access (SACH) and Scheduled-Access Control Channel (SACCH) Signaling Format
* @ingroup _phy_framing

The SACH/SACCH is a multiplexed resource containing both signaling information (Scheduled Access Control CHannel) and user plane traffic
(Scheduled Access CHannel).  The SACH is a set of data streams multiplexed by multiple-antenna OFDMA containing user traffic for several
destinations.  The SACCH contains low-layer protocol information regarding sequencing (for ARQ and channel decoding) and signaling for
channel feedback mechanisms.  In AdHoc/Mesh configuration, the SACCH is a resource common to all destinations sharing the SACH and
its data is multiplexed with those of the SACH. In the cellular configuration, the SACCH
is located in the first allocated symbol and uses the lowest-order (highest protection) coded-modulation format.  Aside from low-level signaling
it contains the allocation formats used by the set of streams in the SACH, specifically the coded-modulation formats and frequency-allocations in the
case of the AdHoc/Mesh configuration.  In the downlink of a cellular configuration, the SACCH information is embedded in the CHBCH PDU.

The transmission format of the SACH/SACCH can either use classical OFDM or digital FDM, and this is signaled by the higher layers.

Each SACH/SACCH is made up of \f$N_{\mathrm{symb,SACH}}\f$ OFDM symbols. The number of symbols
used by a particular SACH in a particular TTI is broadcast via MAC-layer signaling in the CHBCH and depends QoS parameters and
measurements.  It is thus a dynamic parameter known by all nodes in the cluster at the beginning of each TTI.

A SACH/SACCH contains \f$N_\mathrm{pilot,SACCH}\f$ pilot symbols during the SACCH symbols to allow for multi-antenna wideband channel estimation, and
\f$N_\mathrm{pilot,SACH}\f$ pilot symbols per SACH symbol for carrier frequency offset tracking.  The encoding rules for the SACCH
pilot symbols is identical to the MCH/RACH on the first transmit antenna.  If multiple transmit antennas are employed, the pilot symbols for antenna
\f$i=0,1,\cdots,N_\mathrm{ant}\f$ in position $k$ is multiplied by the complex phasor sequence \f$e^{j*2*pi*kiN_\mathrm{p}/N_\mathrm{d}}\f$.
This phasor sequence ensures that the \f$N_\mathrm{ant}\f$ channel responses are orthogonal at the receiver provided
\f$N_\mathrm{ant}N_\mathrm{p}\leq N_\mathrm{d}\f$ and \f$N_\mathrm{p}\f$ is less than the maximum channel duration plus maximum propagation delay.

Both \f$N_\mathrm{pilot,SACH}\f$ and \f$N_\mathrm{ant}\f$ are broadcast using higher layer signaling or pre-configured.

The number of samples for SACH/SACCH data is
\f$N_\mathrm{samp,SACH} = N_\mathrm{symb,SACH}(N_\mathrm{d}-N_\mathrm{f} - N_\mathrm{pilot,SACCH} - N_\mathrm{pilot,SACH}) - 32\f$.
SACCH data is to be encoded using the lowest spectral efficiency coded-modulation format, namely a rate 1/2 forward error-correcting code with QPSK
modulation.  If multiple transmit antennas are used, then the lowest spectral-efficiency BICM space-time code is to be employed.  Prior to FEC coding a
CRC shall be computed on the SACCH data and should be concatenated to the tail of the information.  The total number of
coded bits for the SACCH depends on the number of streams.  It should be kept to a minimum with respect to the total number of data bits in the SACH
streams in order to guarantee efficiency.

Since the SACH is a dynamically allocated resource based on channel quality measures,
sample interleaving across different OFDM carriers is not required.

Prior to forward error-correction coding, a CRC of length 32 bits shall be applied to the PDU.

The SACH is the only resource for which the output of the channel coding block is vectorial, in the case of multiple transmit antennas. Space-time signal
processing is therefore possible on the SACH data streams. Identical interleaving is performed on all antenna streams and the choice of coded-modulation
and space-time processing methods is determined by the MAC scheduler.  A choice of 16 different formats are considered including some of the legacy
802.11a formats. The formats could potentially be reconfigurable and installed at run-time or over-the-air. This is discussed in \ref _phy_coded_modulation.
Each stream can use a separate coded-modulation format.

Streams can use a subset of the OFDM carriers according to the frequency allocation vector as mentioned earlier.  The allocations are chosen by the opportunistic
scheduling algorithm in the MAC layer in order to achieve multi-user diversity and potentially spatial-multiplexing.

The configuration information for the SACH/SACCH resource are partially signaled by higher layers and partially computed dynamically by the scheduling algorithm in the MAC.  They are described by the primitive PHY_SACH summarized in the following table.


*/

/*! \brief Extension Type */
typedef enum {
  CYCLIC_PREFIX,
  CYCLIC_SUFFIX,
  ZEROS,
  NONE
} Extension_t;


/*! \brief Transmit Signal Format */
typedef enum {
  OFDM=0,
  Digital_FDM
} Signal_format_t;


/*! \brief  PHY Framing Structure
*/

typedef struct PHY_FRAMING {
  u_long   fc_khz;         /*!< \brief Carrier Frequency (kHz)*/
  u_long   fs_khz;         /*!< \brief Sampling Frequency (kHz)*/
  u_short         Nsymb ;         /*!< \brief Number of OFDM Symbols per TTI */
  u_short        Nd;             /*!< \brief Number of OFDM Carriers */
  u_char         log2Nd;         /*!< \brief Log2 of Number of OFDM Carriers */
  u_short  Nc;             /*!< \brief Number of Prefix Samples*/
  u_short  Nz;             /*!< \brief Number of Zero Carriers*/
  u_char   Nf;             /*!< \brief Number of Frequency Groups*/
  Extension_t    Extension_type; /*!< \brief Prefix method*/
} PHY_FRAMING;


/*! \brief  PHY_CHSCH Configuration Structure
*/

typedef struct PHY_CHSCH {
  u_short  symbol;                 /*!< \brief First Symbol of CHSCH in TTI */
  u_short  Nsymb;                  /*!< \brief Number of Symbols of CHSCH in TTI*/
  u_char   dd_offset;
  u_long   chsch_seq_re[32];       /*!< \brief Real part of \f$\mathbf{c}_i\f$  \f$(0\cdots n_1-1 \mathrm{LSBs})\f$*/
  u_long   chsch_seq_im[32];       /*!< \brief Imaginary part of \f$\mathbf{c}_i\f$  \f$(0\cdots n_1-1 \mathrm{LSBs})\f$*/
  char         CHSCH_POWER_dBm;    /*!< \brief Average CHSCH Transmit Power*/
} PHY_CHSCH;

/*! \brief  PHY_SCH Configuration Structure
*/
typedef struct PHY_SCH {
  u_short  Nsymb;                  /*!< \brief Number of Symbols of SCH in TTI */
  u_char   dd_offset;
  u_long   sch_seq_re[32];       /*!< \brief Real part of \f$\mathbf{c}_i\f$  \f$(0\cdots n_1-1 \mathrm{LSBs})\f$*/
  u_long   sch_seq_im[32];       /*!< \brief Imaginary part of \f$\mathbf{c}_i\f$  \f$(0\cdots n_1-1 \mathrm{LSBs})\f$*/
  char     SCH_POWER_dBm;        /*!< \brief Average SCH Transmit Power*/
} PHY_SCH;


/*! \brief  PHY_CHBCH Configuration Structure
*/

typedef struct PHY_CHBCH {
  u_short  symbol;           /*!< \brief First Symbol of CHBCH in TTI */
  u_short  Nsymb;            /*!< \brief Number of Symbols of CHBCH in TTI */
  u_short  IntDepth;          /*!< \brief Frequency Interleaving depth of CHBCH */
  u_char   dd_offset;
  u_short  Npilot;            /*!< \brief Number of pilot symbols in CHBCH */
  u_long   pilot_re[8];       /*!< \brief Pilot symbols (Real part) */
  u_long   pilot_im[8];       /*!< \brief Pilot symbols (Imag part) */
  u_char   FreqReuse;         /*!< \brief Frequency Reuse Factor */
  u_char   FreqReuse_ind;     /*!< \brief Frequency Reuse Index */
  char   CHBCH_POWER_dBm;     /*!< \brief Average CHBCH Transmit Power*/
} PHY_CHBCH;

typedef struct PHY_MRBCH {
  u_short  symbol;           /*!< \brief First Symbol of MRBCH in TTI */
  u_short  Nsymb;            /*!< \brief Number of Symbols of MRBCH in TTI */
  u_short  IntDepth;          /*!< \brief Frequency Interleaving depth of MRBCH */
  u_char   dd_offset;
  u_short  Npilot;            /*!< \brief Number of pilot symbols in MRBCH */
  u_long   pilot_re[8];       /*!< \brief Pilot symbols (Real part) */
  u_long   pilot_im[8];       /*!< \brief Pilot symbols (Imag part) */
  u_char   FreqReuse;         /*!< \brief Frequency Reuse Factor */
  u_char   FreqReuse_ind;     /*!< \brief Frequency Reuse Index */
  char     MRBCH_POWER_dBm;     /*!< \brief Average MRBCH Transmit Power*/
} PHY_MRBCH;

/*! \brief  PHY SACH/SACCH Configuration Structure
*/


typedef struct PHY_SACH {
  Signal_format_t Signal_format;                         /*!< \brief Transmit Signal format of SACH/SACCH*/
  u_char          Npilot;                                /*!< \brief Number of pilot symbols */
  u_long          pilot_re[8];                           /*!< \brief Pilot symbols (real part) */
  u_long          pilot_im[8];                           /*!< \brief Pilot symbols (imag part) */
  char            SACH_POWER_dBm;                        /*!< \brief Average MCH/RACH Transmit Power*/
} PHY_SACH;


/* @}*/



/** @defgroup _phy_coded_modulation Coded Modulation and H-ARQ
* @ingroup physical_layer_
* @{
OpenAirMesh makes use of punctured binary codes (64-state rate 1/2 convolutional or 8-state rate 1/3 3GPP/LTE Turbo code).
Puncturing can use either 3GPP rate matching or random puncturing in order to fine tune the coding rate to adapt to
configurable transport block sizes delivered to PHY by the MAC.  The overall coding sub-system is shown in
Figure below. New transport blocks arriving from the MAC layer (based on multi-user scheduling) are coded
using a CRC extension and the chosen binary code.  These are then fed to the active transport block buffer along with
those that are to be retransmitted.  Each transmitted block is punctured and then passed to a bit-interleaver and
modulation mapper (BICM).  OpenAirMesh supports QPSK, 16-QAM and 64-QAM modulation.

\image html openair_coding.png "Coded Modulation Subsystem"
\image latex openair_coding.png "Coded Modulation Subsystem" width=15cm

The transmitted transport blocks can be split into to two spatial streams in the case of point-to-point MIMO transmission.
Each stream receives an adjustable amplitude and then each is passed to a different (orthogonal) space-time parser which
guarantees that both streams use different antennas in the same time/frequency dimension.   This allows for low-complexity
successive detection at the receiver and maximizes diversity against fading. This is a form of superposition coding since
the two streams are combined additively in the air through the use of multiple transmit antennas.

A second design objective for this coding strategy, in addition to low-complexity point-to-point MIMO operation, is that
the same transmitter and receiver structure can be used in a distributed MIMO scenario. Here one spatial stream is used
at each source and the second stream originates in another part of the network, either in the same cluster or an adjacent
cluster.  Co-operation is needed in order to guarantee different STF parsing for the two streams so that they can be
decoupled at the SIC receiver.  A particular user can decode both streams or simply select the one it requires.

*/
/** @defgroup _phy_scrambling Transport block Scrambling
* @ingroup _phy_coded_modulation

Each transport block is scrambled using the LFSR shown in the following figure with a random initial state
in the LFSR determined at deployment time. The LFSR is used in the to descramble the SDU. Scrambling is always
performed on all transport channels.

\image html scrambler.png "MAC SDU Scrambling"
\image latex scrambler.png "MAC SDU Scrambling" width=15cm

\note More information on Linear feedback shift register (LFSR) can be found on http://homepage.mac.com/afj/lfsr.html or http://www-math.cudenver.edu/~wcherowi/courses/m5410/m5410fsr.html
*/

/** @defgroup _phy_crc Cyclic Redundancy Check
* @ingroup _phy_coded_modulation

For the purpose of error-detection a cyclic-redundancy check (CRC) is applied to every transport block entering the
PHY coding subsystem, including those destined for the CHBCH,MCH, RACH and SACCH.  To the latter are a applied an
8-bit systematic CRC defined by the generator \f$g_8(D)=1+D+D^3+D^4+D^7+D^8\f$.  Data on the SACH is encoded using a
24-bit CRC defined by the generator \f$g_{16}(D)=1+D^5+D^{12}+D^{15}\f$.
\note: Basic information about CRC can be found at: http://www.mathpages.com/home/kmath458.htm or http://utopia.knoware.nl/users/eprebel/Communication/CRC/
*/

/**@defgroup _phy_conv_cod Convolutional Coding
* @ingroup _phy_coded_modulation

For coded-modulation formats using convolutional coding, the 802.11a rate 1/2 64-state convolutional coder shown
below shall be applied to the scrambled and parity-checked transport blocks.

\image html convolutional.png "802.11a Convolutional Code"
\image latex convolutional.png "802.11a Convolutional Code" width=15cm

*/

/** @defgroup _phy_puncturing_ Puncturing
* @ingroup _phy_coded_modulation

Random puncturing makes use of a 32-bit Tausworthe random number generator.  Let \f$S_i,i\in\{0,1,2\}\f$ be
the 32-bit initial values of the generator state given by

\f$S_0(0) = \mathrm{1E23D852} + 16s\f$

\f$S_1(0) = \mathrm{81F38A1C} + 16s\f$

\f$S_2(0) = \mathrm{FE1A133E} + 16s\f$

where \f$s\f$ is a seed.  The seed is typically different for each transport block and each round in the
HARQ protocol.  It can be also used for layer 1 encryption.

The state recursions at iteration \f$n\f$ are given by

\f$b_0(n)=((2^{13}S_0(n-1))\oplus S_0(n-1))2^{-19}\f$

\f$b_1(n)=((2^{2}S_1(n-1))\oplus S_1(n-1))2^{-25}\f$

\f$b_2(n)=((2^{3}S_2(n-1))\oplus S_2(n-1))2^{-11}\f$

\f$S_0(n)=((S_0(n-1)\mathrm{and}\mathrm{FFFFFFFE})2^{12})\oplus b_0(n)\f$

\f$S_1(n)=((S_1(n-1)\mathrm{and}\mathrm{FFFFFFF8})2^{4})\oplus b_0(n)\f$

\f$S_2(n)=((S_2(n-1)\mathrm{and}\mathrm{FFFFFFF0})2^{17})\oplus b_0(n)\f$

and the output of the generator is \f$U(n)=S_0(n)\oplus S_1(n)\oplus S_2(n)\f$.

Let \f$N_c\f$ be the number of coded bits after puncturing, \f$N_i\f$ be the number of input bits and
\f$N_{\mathrm{bps}}\f$ be the number bits per symbol \f$N_{\mathrm{bps}}\in\{2,4,6\}\f$. Let the
input sequence be denoted \f$c[i],i\in\{0,1,\cdots,N_i-1\}\f$. The
Tausworthe puncturing procedure is achieved according to the following algorithm for a rate 1/2 binary code:

1. Based on \f$N_c\f$, \f$N_i\f$ check that the rate falls between (\f$\frac{N_{\mathrm{bps}}}{4}\f$,\f$\frac{3N_{\mathrm{bps}}}{8}\f$),
otherwise declare an error.

2. Set the number of punctured bits to \f$N_p\leftarrow (N_i-N_c)/N_{\mathrm{bps}}\f$.

3. Initially mark all \f$N_c\f$ coded bits to be transmitted.

4. Set the initial seed of the Tausworthe to the function value \f$\mathrm{s}\f$

5. Set \f$N_{i2}\leftarrow \lfloor N_i/N_{\mathrm{bps}}\rfloor \f$.

6. Set \f$i\leftarrow0\f$

7. Let \f$U(i)\f$ be the \f$i^{\mathrm{th}}\f$ output of the Tausworthe generator and
\f$U'(i)=U(i)\mathrm{mod}2^{\lceil \log_2(N_{i2})\rceil}\f$

8. if \f$U'(i) < N_{i2}\f$ and \f$\mathrm{c[U'(i)]}\f$ is not already punctured go to 10

9. goto 7

10. Mark \f$c[U' + jN_{i2}])\f$ as punctured for \f$j=0,1,\cdots,N_{\mathrm{bps}}-1\f$.

11. Set \f$i\leftarrow i+1\f$

12. if \f$i<N_{i2}\f$ goto 7
*/

/** @defgroup _phy_interl Bit-Interleaving and Modulation Mapping
* @ingroup _phy_coded_modulation

The interleaving depth for CHBCH,MCH,RACH and SACCH in the multiplexing sub-block is chosen to ensure sufficient separation in frequency
for adjacent coded outputs, and thus maximize frequency diversity.  For coded-modulation formats 0-5, let \f$D=(N_\mathrm{d}-N_\mathrm{z})/IntDepth\f$ where
\f$IntDepth\f$ depends on the particular transport channel at hand.  Each output bit from the convolutional coder, \f$c_i\f$, shall be placed in the set
\f$C_{i\mathrm{mod}D}\f$.  The bits in each set are permuted based on a pseudo-random sequence and then combined to form 2,4 or 6-tuples depending on the modulation
order of the coded-modulation format. The interleaving permutation sequence depends on \f$D\f$ and the number of coded bits.

For the SACH, \f$D\f$ is the number of allocated frequency groups.  The above interleaving strategy shall be applied with this value.

The QAM modulated symbols are mapped according to the Gray mapping as shown in the following figures.  QAM symbols must be scaled to
ensure constant average energy independent of the modulation order.  The scaling factors are \f$1/\sqrt(2),1/\sqrt{10}\f$ and \f$1/\sqrt{42}\f$ for
QPSK, 16-QAM and 64-QAM respectively.

\image html QAMmodulation.png "QAM Modulation Mapping"
\image latex QAMmodulation.png "QAM Modulation Mapping" width=15cm

* @}
* @}
* @defgroup _openair_mac_layer_specs_ MAC Layer (MAC) Specifications
* @ingroup _openair_specs_
*/


#endif /*__PHY_DEFS_SPEC_H__ */















