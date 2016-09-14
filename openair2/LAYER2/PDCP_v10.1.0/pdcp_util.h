/*! \file pdcp_util.h
* \brief PDCP Util Methods
* \author Baris Demiray
* \date 2011
*/

#ifndef PDCP_UTIL_H
#define PDCP_UTIL_H

#include "UTIL/LOG/log.h"
#include "../../COMMON/platform_types.h"
#include "pdcp.h"

/*
 * Prints incoming byte stream in hexadecimal and readable form
 *
 * @param component Utilised as with macros defined in UTIL/LOG/log.h
 * @param data unsigned char* pointer for data buffer
 * @param size Number of octets in data buffer
 * @return none
 */
void util_print_hex_octets(comp_name_t component, unsigned char* data, unsigned long size);

/*
 * Flush incoming byte stream in hexadecimal without formating
 *
 * @param component Utilised as with macros defined in UTIL/LOG/log.h
 * @param data unsigned char* pointer for data buffer
 * @param size Number of octets in data buffer
 * @return none
 */
void util_flush_hex_octets(comp_name_t component, unsigned char* data, unsigned long size);

/*
 * Prints binary representation of given octet prepending
 * passed log message
 *
 * @param Octet as an unsigned character
 * @return None
 */
void util_print_binary_representation(unsigned char* message, uint8_t octet);

/*
 * Sets the bit of `octet' at index `index'
 *
 * @param octet Octet
 * @param index Index
 * @return TRUE on success, FALSE otherwise
 */
boolean_t util_mark_nth_bit_of_octet(uint8_t* octet, uint8_t index);

#endif // PDCP_UTIL_H
