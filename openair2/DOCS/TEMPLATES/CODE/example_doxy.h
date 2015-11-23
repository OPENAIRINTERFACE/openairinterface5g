/*******************************************************************************

  Eurecom OpenAirInterface 2
  Copyright(c) 1999 - 2010 Eurecom

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information
  Openair Admin: openair_admin@eurecom.fr
  Openair Tech : openair_tech@eurecom.fr
  Forums       : http://forums.eurecom.fsr/openairinterface
  Address      : Eurecom, 2229, route des crêtes, 06560 Valbonne Sophia Antipolis, France

*******************************************************************************/


/*! \file doxy_template.h
* \brief explain how this block is organized, and how it works
* \author Navid Nikaein
* \date 2006-2010
* \version 4.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note this a note
* \bug  this is a bug
* \warning  this is a warning
*/

//-----------------------------------begin group-----------------------------


/** @defgroup _oai System definitions



There is different modules:
- OAI Address
- OAI Components
- \ref _frame

The following diagram is based on graphviz (http://www.graphviz.org/), you need to install the package to view the diagram.

 * \dot
 * digraph group_frame  {
 *     node [shape=rect, fontname=Helvetica, fontsize=8,style=filled,fillcolor=lightgrey];
 *     a [ label = " address"];
 *     b [ label = " component"];
 *     c [ label = " frame",URL="\ref _frame"];
 *    a->b;
 *    a->c;
 *    b->d;
 *  label="Architecture"
 *
 * }
 * \enddot

\section _doxy Doxygen Help
You can use the provided Doxyfile as the configuration file or alternatively run "doxygen -g Doxyfile" to generat the file.
You need at least to set the some variables in the Doxyfile including "PROJECT_NAME","PROJECT_NUMBER","INPUT","IMAGE_PATH".
Doxygen help and commands can be found at http://www.stack.nl/~dimitri/doxygen/commands.html#cmdprotocol

\section _arch Architecture

You need to set the IMAGE_PATH in your Doxyfile

\image html arch.png "Architecture"
\image latex arch.eps "Architecture"

\subsection _mac MAC
thisis the mac
\subsection _rlc RLC
this is the rlc
\subsection _impl Implementation
what about the implementation


*@{*/

/*!\brief OAI protocol verion */
#define OAI_PROTOCOL_Version  0x00
/*!\brief Length in bytes of the OAI address */
#define OAI_ADDR_LEN        6

/*!\brief OAI snode type */
enum NodeType {
  /*!\brief mesh routers are */
  meshrouter = 1,
  /*!\brief relay nodes are */
  relaynode = 2,
  /*!\brief clusterheads are */
  clusterhead = 3
};


/*@}*/

// --------------------------end group ------------------------------


//---------------------------begin group------------------------------
/** @defgroup _frame Frame Structure
 * @ingroup _oai
The Frame is composed of ....


*@{*/
/*! \brief the frame structure is ... */
struct frame {
  u_short   duration; /*!< \brief Duration in us (2 bytes) */
  u_char    da[OAI_ADDR_LEN];/*!< \brief Destination MAC@ (OAI_ADDR_LEN bytes) */
  u_char    sa[OAI_ADDR_LEN];/*!< \brief Source MAC@ (OAI_ADDR_LEN bytes)*/
  u_char    body[0]; /*!< \brief Body of the frame */
};
/*! \brief Broadcast ID is ... */
#define BROADCAST_ID 15


/*@}*/

//--------------------------end group-----------------------


//-----------------------begin func proto-------------------

/*! \fn int init(int,int)
* \brief this function initializes and allocates memories and etc.
* \param[in] src the memory area to copy frrm
* \param[out] dst the memory area to copy to
* \return 0 on success, otherwise -1
* \note
* @ingroup  _oai
*/
int init(int src, int dst);

//-----------------------end func proto-------------------
