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

/*****************************************************************
  *
  * File Name :   parser.h
  * Author:     Sharad Gupta
  * Decription:   contains declarations for parser functions
  *
  *    Copyright 2009, Hughes Systique India Private Limited
  *
  ***************************************************************/
#ifndef _PARSER_H
#    define _PARSER_H
#    ifdef PARSER_C
#        define private_at_commands(x)    x
#        define protected_at_commands(x)  x
#        define public_at_commands(x)     x
#    else
#        define private_at_commands(x)
#        define protected_at_commands(x)
#        define public_at_commands(x)     extern x
#    endif
#    include <sys/types.h>
#    include <sys/socket.h>
#    include <netinet/in.h>
#    include <netdb.h>
#    include <stdio.h>
#    include <arpa/inet.h>
#    include <stdlib.h>
#    include <string.h>
#    include <stdarg.h>
#    include <math.h>

#include "platform_types.h"
#include "log.h"


#define AT_CMD_BUFFSIZE 512
//----------------------------------------------------------------------------
typedef struct {
  char *command;
  void (*ResponseFunction) (char *,int);
} at_command_t;


//----------------------------------------------------------------------------
protected_at_commands(char at_error_log[AT_CMD_BUFFSIZE];)
protected_at_commands(char at_command[AT_CMD_BUFFSIZE];)
//----------------------------------------------------------------------------
/**
\fn void (*at_response)(char*, int)
Procedure callback for response to AT command.
@param string Response.
@param size   Size of response in bytes.
**/
public_at_commands(void (*at_response)(char* string, int size);)
/**
\fn int (*mt_attach_state_request)(void)
Procedure for requesting the status of the attachment of MT.
@return attach state of MT (1 for attached, 0 for detached).
**/
public_at_commands(int (*mt_attach_state_request)(void);)
/**
\fn int (*mt_attach_request)(void)
Procedure for requesting the attach of MT.
@return Result of attach (1 for OK, 0 for ERROR).
**/
public_at_commands(int (*mt_attach_request)(void);)
/**
\fn int (*mt_detach_request)(void)
Procedure for requesting the detach of MT.
@return Result of detach (1 for OK, 0 for ERROR).
**/
public_at_commands(int (*mt_detach_request)(void);)
/**
\fn size_t at_grab_string(const unsigned char *input, unsigned char **output)
 Grabs single string parameter from AT command reply. Removing possible quotes.
@param input Input string to parse.
@param output Pointer to pointer to char, buffer will be allocated.
@return Length of parsed string.
**/
private_at_commands(size_t at_grab_string(const unsigned char *input, unsigned char **output);)
/**
\fn int at_parse_command(const unsigned char *input, const char *format, ...)
This function actually parses the string on the basis of format string provided.
@param input String to be parsed.
@param format This is the format string which contains format of input string.
@return  success (1), or failure (0).
**/
private_at_commands(int    at_parse_command(const unsigned char *input, const char *format, ...);)
/**
\fn void   at_attach_or_detach(char *string,int position)
This function attaches or detaches the UE based on the string.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_attach_or_detach(char *string,int position);)
/**
\fn void   at_read_attach_status(char *string,int position)
This function checks whether the UE is attached or detached.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_read_attach_status(char *string,int position);)
/**
\fn void   at_set_pdcp_context(char *string,int position)
This function stores the PDP context of a UE at a particular CID.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_set_pdcp_context(char *string,int position);)
/**
\fn void   at_activate_pdp_context(char *string,int position)
This function activates a particular PDN connection on the basis of CID in the AT command.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_activate_pdp_context(char *string,int position);)
/**
\fn void   at_read_pdp_add(char *string,int position)
This function reads the PDP address on the basis of CID in the AT command.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_read_pdp_add(char *string,int position);)
/**
\fn void   at_read_pdp_context(char *string,int position)
This function reads the PDP context defined by the at_set_pdcp_context function.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_read_pdp_context(char *string,int position);)
/**
\fn void   at_read_act_status(char *string,int position)
This function reads the activation status of a PDP context defined by the at_set_pdcp_context function.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_read_act_status(char *string,int position);)
/**
\fn void   at_show_eps_qos(char *string,int position)
This function reads the QOS parameters on the basis of CID.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_show_eps_qos(char *string,int position);)
/**
\fn void   at_set_tfad_context(char *string,int position)
This function sets the TFAD context for the UE.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_set_tfad_context(char *string,int position);)
/**
\fn void   at_read_tfad_context(char *string,int position)
This function Read the TFAD context for the UE.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_read_tfad_context(char *string,int position);)
/**
\fn void   at_mod_bearer_req(char *string,int position)
This function sends the Modify bearer request to RRC.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_mod_bearer_req(char *string,int position);)
/**
\fn void   at_read_dedicated_bearer_cid(char *string,int position)
This function reads the dedicated bearer CIDs associated with the default bearer CID.
@param string string containing the AT command received.
@param position Position of the string from where the parsing is to be started.
**/
private_at_commands(void   at_read_dedicated_bearer_cid(char *string,int position);)
/**
\fn int    at_convert_hex_to_dec(char *temp,int size)
This function converts the Hex string to decimal.
@param temp Hexadecimal string.
@param size bytes of the string to be converted.
@return Converted decimal value as an integer.
**/
private_at_commands(int    at_convert_hex_to_dec(char *temp,int size);)
/**
\fn void   at_parse_string(char * str)
This function calls the appropriate parser function for parsing the command.
@param str AT command to be parsed.
**/
public_at_commands(void   at_parse_string(char * str);)
/**
\fn void at_parse_init(void (*at_response_handler)(char*, int))
This function register the handler, handling responses from AT commands parser.
@param at_response_handler handler procedure.
**/
public_at_commands(void at_parse_init(void (*at_response_handler)(char*, int), int (*mt_attach_state_request_handler)(void), int (*mt_attach_request_handler)(void),
                                      int (*mt_detach_request_handler)(void));)
/**
\fn void at_null_callback_handler(char* string, int size)
This function register the handler, handling responses from AT commands parser.
@param string message sent back from the AT commands parser to the caller.
@param size  Size in bytes of the message.
**/
public_at_commands(void at_null_callback_handler(char* string, int size);)
/**
\fn int mt_null_attach_state_request_handler(void)
This function is a dummy MT handler for requesting the status of aatachment of the MT.
@return Random/simulated attach state of MT (1 for attached, 0 for detached).
**/
public_at_commands(int mt_null_attach_state_request_handler(void);)
/**
\fn int mt_null_attach_request_handler(void)
This function is a dummy MT handler for requesting the attach of the MT.
@return Random/simulated attach state of MT (1 for OK, 0 for ERROR).
**/
public_at_commands(int mt_null_attach_request_handler(void);)
/**
\fn int mt_null_detach_request_handler(void)
This function is a dummy MT handler for requesting the detach of the MT.
@return Random/simulated detach state of MT (1 for OK, 0 for ERROR).
**/
public_at_commands(int mt_null_detach_request_handler(void);)

#endif
