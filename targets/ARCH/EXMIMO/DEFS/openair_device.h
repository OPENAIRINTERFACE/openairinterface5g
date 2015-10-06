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

#ifndef OPENAIR_DEVICE_H
#define OPENAIR_DEVICE_H


// Maximum number of concurrently supported cards
//
#define MAX_CARDS   8
#define INIT_ZEROS {0, 0, 0, 0, 0, 0, 0, 0};


/** PCIe subsystem configuration Space
 */
// Vendor and System IDs
//
#define XILINX_VENDOR 0x10ee
#define XILINX_ID 0x0007
//
// PCIe Subsystem Vendor ID
#define EURECOM_VENDOR           0x0001
#define TELECOM_PARISTECH_VENDOR 0x0002
/*
   PCIe Subsystem ID = exmimo_id.board_exmimoversion(1,2) (4 bits) | exmimo_id.board_hwrev (4 bits) | exmimo_id.board_swrev (Protocol Revision, 8 bits)

   Board IDs (exmimo_id.board_exmimoversion(1,2) (4 bits) + exmimo_id.board_hwrev (4 bits))
     0x11 => ExpressMIMO-1, first run/HW revision
     0x12 => ExpressMIMO-1, second run
     0x21 => ExpressMIMO-2, first run
     0x22 => ExpressMIMO-2, second run

   SW/Protocol revisions: (exmimo_id.board_swrev (Protocol Revision, 8 bits)

   BOARD_SWREV_LEGACY:
     - IRQ Leon->PC Bit 7 (AHBPCIE_INTERRUPT_ASSERT_BIT) must be cleared in PC kernel driver
     - PC->Leon and Leon->PC commands share a single register CONTROL1

   BOARD_SWREV_CMDREGISTERS:
     - IRQ Leon->PC Bit 7 (AHBPCIE_INTERRUPT_ASSERT_BIT) is automatically cleared on PCIe read
     - PC->Leon and Leon->PC commands have separete command registers CONTROL1 (PC->Leon) and CONTROL2 (Leon->PC)

   BOARD_SWREV_CMDFIFOS:
     - uses two command FIFOs (for PC->Leon and Leon->PC), by Telecom Paristech
*/
#define BOARD_SWREV_LEGACY   0x07
#define BOARD_SWREV_CMDREGISTERS 0x08
#define BOARD_SWREV_CNTL2 0x0A
#define BOARD_SWREV_CMDFIFOS 0x11



// ExpressMIMO PCIe register offsets to bar0
//
#define PCIE_CONTROL0        0x00
#define PCIE_CONTROL1        0x04
#define PCIE_CONTROL2        0x64
#define PCIE_STATUS          0x08
#define PCIE_PCIBASEL        0x1c
#define PCIE_PCIBASEH        0x20


// Device IO definitions and operations
//
#define openair_MAJOR 127

//#define openair_writel(dev,offset,val)     pci_write_config_dword(dev,(int)offset,(unsigned int)val)//{writel((uclong)(val),(ulong)(port)); mb();}

//#define openair_readl(dev,offset,val)     pci_read_config_dword(dev,(int)offset,(unsigned int*)val)//{writel((uclong)(val),(ulong)(port)); mb();}

#define openair_IOC_MAGIC         'm'

#define openair_GET_BIGSHMTOPS_KVIRT         _IOR(openair_IOC_MAGIC,1,int)
#define openair_GET_PCI_INTERFACE_BOTS_KVIRT _IOR(openair_IOC_MAGIC,2,int)
#define openair_GET_NUM_DETECTED_CARDS       _IOR(openair_IOC_MAGIC,3,int)

#define openair_DUMP_CONFIG                  _IOR(openair_IOC_MAGIC,18,int)
#define openair_GET_FRAME                    _IOR(openair_IOC_MAGIC,6,int)
#define openair_START_RT_ACQUISITION         _IOR(openair_IOC_MAGIC,28,int)
#define openair_STOP                         _IOR(openair_IOC_MAGIC,5,int)
#define openair_STOP_WITHOUT_RESET           _IOR(openair_IOC_MAGIC,9,int)
#define openair_RECONFIGURE                  _IOR(openair_IOC_MAGIC,10,int)
#define openair_UPDATE_FIRMWARE              _IOR(openair_IOC_MAGIC,40,int)


/* Update firmware commands */
#define UPDATE_FIRMWARE_TRANSFER_BLOCK    0x1
#define UPDATE_FIRMWARE_CLEAR_BSS         0x2
#define UPDATE_FIRMWARE_START_EXECUTION   0x3
#define UPDATE_FIRMWARE_FORCE_REBOOT      0x4
#define UPDATE_FIRMWARE_TEST_GOK          0x5


// mmap page offset vg_pgoff is used to pass arguments to kernel
// bit0..3: memory block: BIGSHM:0, RX:1,3,5,7, TX:2,4,6,8
// bit4..7: card_id
#define openair_mmap_BIGSHM            0
#define openair_mmap_RX(ant) (((ant)<<1)+1)
#define openair_mmap_TX(ant) (((ant)<<1)+2)

#define openair_mmap_getMemBlock(o)  ((o)&0xF)
#define openair_mmap_getAntRX(o) (((o)-1)>>1)
#define openair_mmap_getAntTX(o) (((o)-2)>>1)

#define openair_mmap_Card(c)    ( ((c)&0xF)<<4 )
#define openair_mmap_getCard(o) ( ((o)>>4)&0xF )


#endif /* OPENAIR_DEVICE_H */
