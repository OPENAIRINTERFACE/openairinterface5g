/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

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
