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

#define PARSER_C
//-----------------------------------------------------------------------------
#include "rtos_header.h"
#include "platform_types.h"
//-----------------------------------------------------------------------------
#include "parser.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h> // for random
//-----------------------------------------------------------------------------
at_command_t g_at_response_mapping[] = {
  { "AT+CGATT="   ,   at_attach_or_detach          },
  { "AT+CGATT?"   ,   at_read_attach_status        },
  { "AT+CGDCONT=" ,   at_set_pdcp_context          },
  { "AT+CGDCONT?" ,   at_read_pdp_context          },
  { "AT+CGACT?"   ,   at_read_act_status           },
  { "AT+CGACT="   ,   at_activate_pdp_context      },
  { "AT+CGPADDR=" ,   at_read_pdp_add              },
  { "AT+CGEQOS="  ,   at_show_eps_qos              },
  { "AT+CGETFADS=",   at_set_tfad_context          },
  { "AT+CGETFADR=",   at_read_tfad_context         },
  { "AT+CGEBRMOD=",   at_mod_bearer_req            },
  { "AT+CGEBRMOD?",   at_read_dedicated_bearer_cid },
  { "null"        ,   NULL}
};
//-----------------------------------------------------------------------------
size_t at_grab_string(const unsigned char *input, unsigned char **output)
//-----------------------------------------------------------------------------
{
  size_t size = 4, position = 0;
  int inside_quotes = 0;

  /* Allocate initial buffer in case string is empty */
  *output = (unsigned char *)malloc(size);

  if (*output == NULL) {
    printf("PARSER_LOG:Ran out of memory!\n");
    return 0;
  }

  while (inside_quotes ||
         (  *input != ','
            && *input != ')'
            && *input != 0x0d
            && *input != 0x0a
            && *input != 0x00)) {

    /* Check for quotes */
    if (*input == '"') {
      inside_quotes = ! inside_quotes;
    }

    /* We also allocate space for traling zero */
    if (position + 2 > size) {
      size += 10;
      *output = (unsigned char *)realloc(*output, size);

      if (*output == NULL) {
        printf("PARSER_LOG:Ran out of memory!\n");
        return 0;
      }
    }

    /* Copy to output */
    (*output)[position] = *input;
    position++;
    input   ++;
  }

  (*output)[position] = 0;

  /* Strip quotes */
  if ((*output)[0] == '"' && (*output)[position - 1]) {
    memmove(*output, (*output) + 1, position - 2);
    (*output)[position - 2] = 0;
  }

  printf("PARSER_LOG:Grabbed string from reply: \"%s\" (parsed %Id  bytes)\n", *output, position);
  return position;
}
//-----------------------------------------------------------------------------
int at_parse_command(const unsigned char *input, const char *format, ...)
//-----------------------------------------------------------------------------
{
  const char *fmt = format;
  const char *inp = (const char *)input;
  char *endptr;
  unsigned char *out_us;
  unsigned char *buffer;
  size_t length;
  size_t storage_size;
  int *out_i;
  long int *out_l;
  va_list ap;
  int error = 1;

  printf("PARSER_LOG:Parsing %s\n with %s\n", input, format);

  va_start(ap, format);

  while (*fmt) {
    switch(*fmt++) {
    case '@':
      if (*fmt == 0) {
        printf("PARSER_LOG:Invalid format string: %s\n", format);
        error = 0;
        goto end;
      }

      switch(*fmt++) {
      case 'i':
        out_i = va_arg(ap, int *);
        *out_i = strtol(inp, &endptr, 10);

        if (endptr == inp) {
          error = 0;
          goto end;
        }

        printf("PARSER_LOG:Parsed int %d\n", *out_i);
        inp = endptr;
        break;

      case 'I':
        out_i = va_arg(ap, int *);
        *out_i = strtol(inp, &endptr, 10);

        if (endptr == inp) {
          printf("PARSER_LOG:Number empty\n");
          *out_i = 0;
        } else {
          printf("PARSER_LOG:Parsed int %d\n", *out_i);
          inp = endptr;
        }

        break;

      case 'l':
        out_l = va_arg(ap, long int *);
        *out_l = strtol(inp, &endptr, 10);

        if (endptr == inp) {
          error = 0;
          goto end;
        }

        printf("PARSER_LOG:Parsed long int %ld\n", *out_l);
        inp = endptr;
        break;

      case 'r':
        out_us = va_arg(ap, unsigned char *);
        storage_size = va_arg(ap, size_t);
        length = at_grab_string(inp, &buffer);
        printf("PARSER_LOG:Parsed raw string \"%s\"\n", buffer);

        if (strlen(buffer) > storage_size) {
          free(buffer);
          error = 0;
          goto end;
        }

        strcpy(out_us, buffer);
        free(buffer);
        inp += length;
        break;

      case '@':
        if (*inp++ != '@') {
          error = 0;
          goto end;
        }

        break;

      case '0':
        /* Just skip the rest */
        goto end;

      default:
        printf("PARSER_LOG:Invalid format string (@%c): %s\n", *(fmt - 1), format);
        error = 0;
        goto end;
      }

      break;

    case ' ':
      while (isspace(*inp)) inp++;

      break;

    default:
      if (*inp++ != *(fmt - 1)) {
        error = 0;
        goto end;
      }

      break;
    }
  }

  /* Ignore trailing spaces */
  while (isspace(*inp)) inp++;

  if (*inp != 0) {
    printf("PARSER_LOG:String do not end same!\n");
    error = 0;
    goto end;
  }

end:
  va_end(ap);
  return error;
}
//-----------------------------------------------------------------------------
void at_attach_or_detach(char *string,int position)
//-----------------------------------------------------------------------------
{
  // From 3GPP TS 27.007 V10.4.0 (2011-06):
  //---------------------+--------------------------------------------
  // Command             |   Possible Response(s)
  //---------------------+--------------------------------------------
  // +CGATT=[<state>]    |    OK
  //                     |    ERROR
  //---------------------+--------------------------------------------
  // +CGATT?             |    +CGATT: <state>
  //---------------------+--------------------------------------------
  // +CGATT=?            |    +CGATT: (list of supported <state>s)
  //---------------------+--------------------------------------------
  // Description
  //      The execution command is used to attach the MT to, or detach the MT from, the Packet Domain service. After the
  //      command has completed, the MT remains in V.250 command state. If the MT is already in the requested state, the
  //      command is ignored and the OK response is returned. If the requested state cannot be achieved, an ERROR or
  //      +CME ERROR response is returned. Extended error responses are enabled by the +CMEE command.
  //      Any active PDP contexts will be automatically deactivated when the attachment state changes to detached.
  //      The read command returns the current Packet Domain service state.
  //      The test command is used for requesting information on the supported Packet Domain service states.
  //      NOTE:
  //          This command has the characteristics of both the V.250 action and parameter commands. Hence it has the
  //          read form in addition to the execution/set and test forms.
  //
  // Defined Values
  //      <state>: : a numeric parameter that indicates the state of PS attachment
  //          0 detached
  //          1 attached
  // Other values are reserved and will result in an ERROR response to the execution command.


  char *temp;
  char *str="+CGATT: (0,1)\r\nOK\r\n";
  int Error = -1,attach_status = -1;

  if(string[position + 1] == '?') {
    printf("AT PARSER sending: %s\n",str);
    at_response(str,strlen(str));
    return;
  }

  temp = string + position +1;
  Error = at_parse_command(temp," @i",&attach_status);

  if(Error == 1) {
    switch(attach_status) {
    case 1:
      printf("AT PARSER: Attach command received\n");

      if (mt_attach_request() == 1) {
        at_response("OK\r\n",strlen("OK\r\n"));

      } else {
        at_response("ERROR\r\n",strlen("ERROR\r\n"));

      }

      break;

    case 0:
      printf("AT PARSER: Detach command received\n");

      if (mt_detach_request() == 1) {
        at_response("OK\r\n",strlen("OK\r\n"));

      } else {
        at_response("ERROR\r\n",strlen("ERROR\r\n"));

      }

      break;

    default:
      printf("AT PARSER: Invalid value %d\n",attach_status);
      memset(at_error_log,'\0',sizeof(at_error_log));
      sprintf(at_error_log,"Invalid value %d",attach_status);
      at_response(at_error_log,strlen(at_error_log));
      break;
    }
  } else {
    printf("AT PARSER: Parse failed\n");
    memset(at_error_log,'\0',sizeof(at_error_log));
    sprintf(at_error_log,"ERROR\r\n");
    at_response(at_error_log,strlen(at_error_log));
  }
}
//-----------------------------------------------------------------------------
void at_read_attach_status(char *string,int position)
//-----------------------------------------------------------------------------
{
  if (mt_attach_state_request() == 1) {
    printf( "\n PARSER_LOG: UE is Attached\n");
    at_response("+CGATT: 1\r\nOK\r\n",strlen("+CGATT: 1\r\nOK\r\n"));
  } else {
    printf( "\n PARSER_LOG: UE is Detached\n");
    at_response("+CGATT: 0\r\nOK\r\n",strlen("+CGATT: 0\r\nOK\r\n"));
  }
}
//-----------------------------------------------------------------------------
void at_set_pdcp_context(char *string,int position)
//-----------------------------------------------------------------------------
{

}
//-----------------------------------------------------------------------------
void at_activate_pdp_context(char *string,int position)
//-----------------------------------------------------------------------------
{
}
//-----------------------------------------------------------------------------
void at_read_pdp_add(char *string,int position)
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
void at_read_pdp_context(char *string,int position)
//-----------------------------------------------------------------------------
{
}
//-----------------------------------------------------------------------------
void at_read_act_status(char *string,int position)
//-----------------------------------------------------------------------------
{
}
//-----------------------------------------------------------------------------
void at_show_eps_qos(char *string,int position)
//-----------------------------------------------------------------------------
{
}
//-----------------------------------------------------------------------------
void at_set_tfad_context(char *string,int position)
//-----------------------------------------------------------------------------
{
}
//-----------------------------------------------------------------------------
void at_read_tfad_context(char *string,int position)
//-----------------------------------------------------------------------------
{
}
//-----------------------------------------------------------------------------
void at_mod_bearer_req(char *string,int position)
//-----------------------------------------------------------------------------
{
}
//-----------------------------------------------------------------------------
void at_read_dedicated_bearer_cid(char *string,int position)
//-----------------------------------------------------------------------------
{
}
//-----------------------------------------------------------------------------
int at_convert_hex_to_dec(char *temp,int size)
//-----------------------------------------------------------------------------
{
  int res = 0,i =0,hex_value = 0,Power = 1,j;

  for(i = 0; i < size; i++) {
    switch(temp[size - i - 1]) {
    case 'A':
    case 'a':
      hex_value = 10;
      break;

    case 'B':
    case 'b':
      hex_value = 11;
      break;

    case 'C':
    case 'c':
      hex_value = 12;
      break;

    case 'D':
    case 'd':
      hex_value = 13;
      break;

    case 'E':
    case 'e':
      hex_value = 14;
      break;

    case 'F':
    case 'f':
      hex_value = 15;
      break;

    default:
      hex_value = temp[size - i - 1] - '0';
    }

    for(j = 1,Power = 1; j <= i ; j++)
      Power = Power * 16;

    res = res + (hex_value * Power);
  }

  return res;
}
//-----------------------------------------------------------------------------
void at_parse_string(char * at_strP)
//-----------------------------------------------------------------------------
{
  char *buffer = NULL;
  char format[4] = "=?\n";
  char * ptr;
  int i=0,pos = -1,flag = 0;

  // was /r
  if((buffer = strchr(at_strP,'\r'))!=NULL) {
    memset(at_command, 0, AT_CMD_BUFFSIZE);
    memcpy(at_command, at_strP, buffer-at_strP);
    //*(buffer + 1) = '\0';
    printf("AT PARSER:received = %s\n",at_command);
  } else {
    printf("PARSER_LOG:invalid command : Enter Again\n");
    memset(at_error_log,'\0',sizeof(at_error_log));
    sprintf(at_error_log,"invalid command : Enter Again");
    at_response(at_error_log,strlen(at_error_log));
    return;
  }

  buffer = at_command;
  ptr = strpbrk(buffer,format);
  pos = ptr - buffer;

  while(strcmp(g_at_response_mapping[i].command,"null")!=0) {
    if(strncmp(buffer,g_at_response_mapping[i].command,pos+1)== 0) {
      flag=1;
      break;
    }

    i++;
  }

  if(flag == 0 ) {
    printf("PARSER_LOG:unknown command \n");
    memset(at_error_log,'\0',sizeof(at_error_log));
    sprintf(at_error_log,"unknown command");
    printf("IFM<-UeMgr [ label = \"Unknown AT CMD\" ] ;");
    at_response(at_error_log,strlen(at_error_log));
    return;
  } else {
    g_at_response_mapping[i].ResponseFunction(at_command,pos);
  }
}
//-----------------------------------------------------------------------------
void at_parse_init(void (*at_response_handler)(char*, int),
                   int (*mt_attach_state_request_handler)(void),
                   int (*mt_attach_request_handler)(void),
                   int (*mt_detach_request_handler)(void))
//-----------------------------------------------------------------------------
{
  at_response             = at_response_handler;
  mt_attach_state_request = mt_attach_state_request_handler;
  mt_attach_request       = mt_attach_request_handler;
  mt_detach_request       = mt_detach_request_handler;
}
//-----------------------------------------------------------------------------
void at_null_callback_handler(char* string, int size)
//-----------------------------------------------------------------------------
{
  char at_response[AT_CMD_BUFFSIZE];

  assert(size > 0);
  assert(size < AT_CMD_BUFFSIZE);
  strncpy(at_response, string, size);
  at_response[size] = 0;
  printf("AT DEFAULT CALLBACK HANDLER RECEIVED RESPONSE: %s\n", at_response);
}
//-----------------------------------------------------------------------------
int mt_null_attach_state_request_handler(void)
//-----------------------------------------------------------------------------
{
  int n;
  n=rand() %2;  /* n is random number in range of 0 - 1 */
  return(n);
}
//-----------------------------------------------------------------------------
int mt_null_attach_request_handler(void)
//-----------------------------------------------------------------------------
{
  int n;
  n=rand() %2;  /* n is random number in range of 0 - 1 */
  return(n);
}
//-----------------------------------------------------------------------------
int mt_null_detach_request_handler(void)
//-----------------------------------------------------------------------------
{
  int n;
  n=rand() %2;  /* n is random number in range of 0 - 1 */
  return(n);
}
