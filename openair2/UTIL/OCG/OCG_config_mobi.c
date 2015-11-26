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

/*! \file OCG_config_mobi.c
* \brief Generate an XML to configure the mobility
* \author Lusheng Wang  & Navid Nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

/*--- INCLUDES ---------------------------------------------------------------*/
#include <string.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include "OCG.h"
#include "OCG_extern.h"
#include "OCG_config_mobi.h"
/*----------------------------------------------------------------------------*/

int config_mobi(char mobigen_filename[FILENAME_LENGTH_MAX], char filename[FILENAME_LENGTH_MAX])
{
  // for the xml writer, refer to http://xmlsoft.org/html/libxml-xmlwriter.html

  char dir_to_mobigen_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX] = "";
  char mobi_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX] = "";
  xmlTextWriterPtr writer;

  strcpy(dir_to_mobigen_file, DIR_TO_MOBIGEN);
  strcat(dir_to_mobigen_file, mobigen_filename);
  /* Create a new XmlWriter for uri, with no compression. */
  writer = xmlNewTextWriterFilename(dir_to_mobigen_file, 0);

  // set the output format of the XML file
  xmlTextWriterSetIndent(writer, 1);
  xmlTextWriterSetIndentString(writer, "	");

  /* Start the document with the xml default for the version,
   * encoding ISO 8859-1 and the default for the standalone
   * declaration. */
  xmlTextWriterStartDocument(writer, NULL, NULL, NULL);

  /* Start an element named "EXAMPLE". Since this is the first
   * element, this will be the root element of the document. */
  xmlTextWriterStartElement(writer, "universe");

  /* Write an element named "X_ORDER_ID" as child of HEADER. */
  xmlTextWriterWriteFormatElement(writer, "dimx", "%lf", oai_emulation.topo_config.area.x);

  /* Write an element named "X_ORDER_ID" as child of HEADER. */
  xmlTextWriterWriteFormatElement(writer, "dimy", "%lf", oai_emulation.topo_config.area.y);

  xmlTextWriterWriteFormatElement(writer, "seed", "%d", 1);

  xmlTextWriterStartElement(writer, "extension");
  xmlTextWriterWriteAttribute(writer, "class", "de.uni_stuttgart.informatik.canu.mobisim.simulations.TimeSimulation");
  xmlTextWriterWriteFormatAttribute(writer, "param", "%lf", oai_emulation.emu_config.emu_time);
  xmlTextWriterEndElement(writer);

  xmlTextWriterStartElement(writer, "extension");
  xmlTextWriterWriteAttribute(writer, "class", "de.uni_stuttgart.informatik.canu.spatialmodel.core.SpatialModel");
  xmlTextWriterWriteFormatAttribute(writer, "max_x", "%lf", oai_emulation.topo_config.area.x);
  xmlTextWriterWriteFormatAttribute(writer, "max_y", "%lf", oai_emulation.topo_config.area.y);
  xmlTextWriterWriteAttribute(writer, "min_x", "0");
  xmlTextWriterWriteAttribute(writer, "min_y", "0");

  xmlTextWriterWriteFormatElement(writer, "dump_boundary_points", "%s", "false");

  xmlTextWriterWriteFormatElement(writer, "separated_flow", "%s", "false");

  xmlTextWriterWriteFormatElement(writer, "max_traffic_lights", "%d", 500);

  xmlTextWriterEndElement(writer);

  xmlTextWriterStartElement(writer, "extension");
  xmlTextWriterWriteAttribute(writer, "class", "de.uni_stuttgart.informatik.canu.mobisim.extensions.ReportNodeMobility");

  strcpy(mobi_file, MOBI_XML_FOLDER);
  strcat(mobi_file, filename);
  xmlTextWriterWriteAttribute(writer, "output", mobi_file);

  xmlTextWriterWriteFormatElement(writer, "step", "%d", 1);

  xmlTextWriterEndElement(writer);

  xmlTextWriterStartElement(writer, "nodegroup");
  xmlTextWriterWriteAttribute(writer, "car2x", "false");
  xmlTextWriterWriteAttribute(writer, "id", "");
  xmlTextWriterWriteAttribute(writer, "n", "50");
  xmlTextWriterWriteAttribute(writer, "type", "car");

  xmlTextWriterStartElement(writer, "position");
  xmlTextWriterWriteAttribute(writer, "random", "true");

  xmlTextWriterWriteFormatElement(writer, "z", "%s", "0.0");

  xmlTextWriterEndElement(writer);

  xmlTextWriterStartElement(writer, "extension");
  xmlTextWriterWriteAttribute(writer, "class", "de.uni_stuttgart.informatik.canu.mobisim.mobilitymodels.RandomWaypointWalk");

  if (!strcmp(oai_emulation.topo_config.UE_mobility.UE_mobility_type.selected_option, "fixed")) {
    xmlTextWriterWriteFormatElement(writer, "minspeed", "%d", 0);
    xmlTextWriterWriteFormatElement(writer, "maxspeed", "%d", 0);
    xmlTextWriterWriteFormatElement(writer, "minstay", "%d", 9999);
    xmlTextWriterWriteFormatElement(writer, "maxstay", "%d", 9999);
  } else if (!strcmp(oai_emulation.topo_config.UE_mobility.UE_mobility_type.selected_option, "random_waypoint")) {
    xmlTextWriterWriteFormatElement(writer, "minspeed", "%lf", oai_emulation.topo_config.UE_mobility.UE_moving_dynamics.min_speed);
    xmlTextWriterWriteFormatElement(writer, "maxspeed", "%lf", oai_emulation.topo_config.UE_mobility.UE_moving_dynamics.max_speed);
    xmlTextWriterWriteFormatElement(writer, "minstay", "%lf", oai_emulation.topo_config.UE_mobility.UE_moving_dynamics.min_pause_time);
    xmlTextWriterWriteFormatElement(writer, "maxstay", "%lf", oai_emulation.topo_config.UE_mobility.UE_moving_dynamics.max_pause_time);
  }

  // Close the element named HEADER.
  xmlTextWriterEndElement(writer);


  xmlTextWriterEndDocument(writer);

  xmlFreeTextWriter(writer);

  return MODULE_OK;
}

