/*! \file OCG_parse_XML.h
* \brief Variables indicating the element which is currently parsed
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OCG_PARSE_XML_H__

#define __OCG_PARSE_XML_H__

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup _parsing_position_indicator Parsing Position Indicator
 *  @ingroup _parse_XML
 *  @brief Indicate the position where the program is current parsing in the XML file
 * @{*/

/* @}*/

/** @defgroup _parse_XML Parse XML
 *  @ingroup _fn
 *  @brief Parse the XML configuration file
 * @{*/
int parse_XML(char src_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX]);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif
