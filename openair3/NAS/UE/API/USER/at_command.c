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

/*****************************************************************************

Source    at_command.c

Version   0.1

Date    2012/03/07

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel, Baris Demiray

Description Defines the ATtention (AT) command set supported by the NAS
    sublayer protocol

*****************************************************************************/

#include "at_command.h"
#include "commonDef.h"
#include "nas_log.h"

#include "at_error.h"

#include <ctype.h>  // isspace, isdigit
#include <stdlib.h> // strtol, malloc, free
#include <string.h> // strncpy, strstr, strchr, strcmp, strncasecmp
// strlen, memmove, strdup, strtok
#include <stdarg.h> // va_start, va_arg, va_end

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

// FIXME Put this in .h
extern int at_response_format_v1;
extern int at_error_code_suppression_q1;
extern at_error_format_t at_error_format;

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* Handlers used to parse AT commands */
static int parse_e0(const char*, int, at_command_t*);
static int parse_e1(const char*, int, at_command_t*);
static int parse_q0(const char*, int, at_command_t*);
static int parse_q1(const char*, int, at_command_t*);
static int parse_v0(const char*, int, at_command_t*);
static int parse_v1(const char*, int, at_command_t*);
static int parse_cgsn(const char*, int, at_command_t*);
static int parse_cgsn_test(const char*, int, at_command_t*);
static int parse_cgmi(const char*, int, at_command_t*);
static int parse_cgmi_test(const char*, int, at_command_t*);
static int parse_cgmm(const char*, int, at_command_t*);
static int parse_cgmm_test(const char*, int, at_command_t*);
static int parse_cgmr(const char*, int, at_command_t*);
static int parse_cgmr_test(const char*, int, at_command_t*);
static int parse_cimi(const char*, int, at_command_t*);
static int parse_cimi_test(const char*, int, at_command_t*);
static int parse_cfun_set(const char*, int, at_command_t*);
static int parse_cfun_get(const char*, int, at_command_t*);
static int parse_cfun_test(const char*, int, at_command_t*);
static int parse_cpin_set(const char*, int, at_command_t*);
static int parse_cpin_get(const char*, int, at_command_t*);
static int parse_cpin_test(const char*, int, at_command_t*);
static int parse_csq(const char*, int, at_command_t*);
static int parse_csq_test(const char*, int, at_command_t*);
static int parse_cesq(const char*, int, at_command_t*);
static int parse_cesq_test(const char*, int, at_command_t*);
static int parse_clac(const char*, int, at_command_t*);
static int parse_clac_test(const char*, int, at_command_t*);
static int parse_cmee_set(const char*, int, at_command_t*);
static int parse_cmee_get(const char*, int, at_command_t*);
static int parse_cmee_test(const char*, int, at_command_t*);
static int parse_cnum(const char*, int, at_command_t*);
static int parse_cnum_test(const char*, int, at_command_t*);
static int parse_clck_set(const char*, int, at_command_t*);
static int parse_clck_test(const char*, int, at_command_t*);
static int parse_cops_set(const char*, int, at_command_t*);
static int parse_cops_get(const char*, int, at_command_t*);
static int parse_cops_test(const char*, int, at_command_t*);
static int parse_creg_set(const char*, int, at_command_t*);
static int parse_creg_get(const char*, int, at_command_t*);
static int parse_creg_test(const char*, int, at_command_t*);
static int parse_cgatt_set(const char*, int, at_command_t*);
static int parse_cgatt_get(const char*, int, at_command_t*);
static int parse_cgatt_test(const char*, int, at_command_t*);
static int parse_cgreg_set(const char*, int, at_command_t*);
static int parse_cgreg_get(const char*, int, at_command_t*);
static int parse_cgreg_test(const char*, int, at_command_t*);
static int parse_cereg_set(const char*, int, at_command_t*);
static int parse_cereg_get(const char*, int, at_command_t*);
static int parse_cereg_test(const char*, int, at_command_t*);
static int parse_cgdcont_set(const char*, int, at_command_t*);
static int parse_cgdcont_get(const char*, int, at_command_t*);
static int parse_cgdcont_test(const char*, int, at_command_t*);
static int parse_cgact_set(const char*, int, at_command_t*);
static int parse_cgact_get(const char*, int, at_command_t*);
static int parse_cgact_test(const char*, int, at_command_t*);
static int parse_cgpaddr_set(const char*, int, at_command_t*);
static int parse_cgpaddr_test(const char*, int, at_command_t*);

/* List of AT command handlers */
typedef struct {
  char *command;
  int (*handler)(const char *, int, at_command_t*);
  at_command_type_t type;
} at_command_handler_t;

static const at_command_handler_t _at_command_handler[] = {
  /* TE-TA interface commands         */
  { "E0",   parse_e0,   AT_COMMAND_ACT },
  { "E1",   parse_e1,   AT_COMMAND_ACT },
  { "Q0",   parse_q0,   AT_COMMAND_ACT },
  { "Q1",   parse_q1,   AT_COMMAND_ACT },
  { "V0",   parse_v0,   AT_COMMAND_ACT },
  { "V1",   parse_v1,   AT_COMMAND_ACT },
  { "E0Q0V0",   parse_v0,   AT_COMMAND_ACT },
  { "E0Q0V1",   parse_v1,   AT_COMMAND_ACT },
  { "E1Q0V0",   parse_v0,   AT_COMMAND_ACT },
  { "E1Q0V1",   parse_v1,   AT_COMMAND_ACT },
  /* Request Product Serial Number identification (IMEI)  */
  { "+CGSN",    parse_cgsn,   AT_COMMAND_ACT },
  { "+CGSN=?",  parse_cgsn_test,  AT_COMMAND_TST },
  /* Request manufacturer identification      */
  { "+CGMI",    parse_cgmi,     AT_COMMAND_ACT },
  { "+CGMI=?",  parse_cgmi_test,  AT_COMMAND_TST },
  /* Request model identification       */
  { "+CGMM",    parse_cgmm,     AT_COMMAND_ACT },
  { "+CGMM=?",  parse_cgmm_test,  AT_COMMAND_TST },
  /* Request revision identification        */
  { "+CGMR",    parse_cgmr,     AT_COMMAND_ACT },
  { "+CGMR=?",  parse_cgmr_test,  AT_COMMAND_TST },
  /* Request International Mobile Subscriber Identity   */
  { "+CIMI",    parse_cimi,     AT_COMMAND_ACT },
  { "+CIMI=?",  parse_cimi_test,  AT_COMMAND_TST },
  /* Set phone functionality          */
  { "+CFUN=",   parse_cfun_set,   AT_COMMAND_SET },
  { "+CFUN?",   parse_cfun_get,   AT_COMMAND_GET },
  { "+CFUN=?",  parse_cfun_test,  AT_COMMAND_TST },
  /* Enter PIN            */
  { "+CPIN=",   parse_cpin_set,   AT_COMMAND_SET },
  { "+CPIN?",   parse_cpin_get,   AT_COMMAND_GET },
  { "+CPIN=?",  parse_cpin_test,  AT_COMMAND_TST },
  /* Signal quality           */
  { "+CSQ",     parse_csq,    AT_COMMAND_ACT },
  { "+CSQ=?",   parse_csq_test,   AT_COMMAND_TST },
  /* Extended signal quality          */
  { "+CESQ",    parse_cesq,     AT_COMMAND_ACT },
  { "+CESQ=?",  parse_cesq_test,  AT_COMMAND_TST },
  /* List all available AT commands       */
  { "+CLAC",    parse_clac,     AT_COMMAND_ACT },
  { "+CLAC=?",  parse_clac_test,  AT_COMMAND_TST },
  /*  Report mobile termination error       */
  { "+CMEE=",   parse_cmee_set,   AT_COMMAND_SET },
  { "+CMEE?",   parse_cmee_get,   AT_COMMAND_GET },
  { "+CMEE=?",  parse_cmee_test,  AT_COMMAND_TST },
  /* Subscriber number          */
  { "+CNUM",    parse_cnum,     AT_COMMAND_ACT },
  { "+CNUM=?",  parse_cnum_test,  AT_COMMAND_TST },
  /* Facility lock            */
  { "+CLCK=",   parse_clck_set,   AT_COMMAND_SET },
  { "+CLCK=?",  parse_clck_test,  AT_COMMAND_TST },
  /* PLMN selection             */
  { "+COPS=",   parse_cops_set,   AT_COMMAND_SET },
  { "+COPS?",   parse_cops_get,   AT_COMMAND_GET },
  { "+COPS=?",  parse_cops_test,  AT_COMMAND_TST },
  /* Network registration         */
  { "+CREG=",   parse_creg_set,   AT_COMMAND_SET },
  { "+CREG?",   parse_creg_get,   AT_COMMAND_GET },
  { "+CREG=?",  parse_creg_test,  AT_COMMAND_TST },
  /* GPRS service attach/detach       */
  { "+CGATT=",  parse_cgatt_set,  AT_COMMAND_SET },
  { "+CGATT?",  parse_cgatt_get,  AT_COMMAND_GET },
  { "+CGATT=?",   parse_cgatt_test,   AT_COMMAND_TST },
  /* GPRS network registration status       */
  { "+CGREG=",  parse_cgreg_set,  AT_COMMAND_SET },
  { "+CGREG?",  parse_cgreg_get,  AT_COMMAND_GET },
  { "+CGREG=?",   parse_cgreg_test,   AT_COMMAND_TST },
  /* EPS network registration status        */
  { "+CEREG=",  parse_cereg_set,  AT_COMMAND_SET },
  { "+CEREG?",  parse_cereg_get,  AT_COMMAND_GET },
  { "+CEREG=?",   parse_cereg_test,   AT_COMMAND_TST },
  /* Define PDP Context         */
  { "+CGDCONT=",  parse_cgdcont_set,  AT_COMMAND_SET },
  { "+CGDCONT?",  parse_cgdcont_get,  AT_COMMAND_GET },
  { "+CGDCONT=?",   parse_cgdcont_test,   AT_COMMAND_TST },
  /* PDP context activate or deactivate     */
  { "+CGACT=",  parse_cgact_set,  AT_COMMAND_SET },
  { "+CGACT?",  parse_cgact_get,  AT_COMMAND_GET },
  { "+CGACT=?",   parse_cgact_test,   AT_COMMAND_TST },
  /* Show PDP address(es)         */
  { "+CGPADDR=",  parse_cgpaddr_set,  AT_COMMAND_SET },
  { "+CGPADDR=?",   parse_cgpaddr_test,   AT_COMMAND_TST },

  { "null", NULL }
};

static int ParseString(char * at_str, at_command_t* at_command);
static int ParseCommand(const unsigned char *str, const char *format, ...);
static size_t GrabString(const unsigned char *str, char **output);
static unsigned int NumberOfParameters(const unsigned char* str);
static unsigned int IsNumeric(const char* str, size_t len);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  at_command_decode()                                       **
 **                                                                        **
 ** Description: Parses AT command line and accordingly fills data struc-  **
 **    ture. The main functions of the AT command parser are:    **
 **        - check the AT command syntax                         **
 **        - fill parameter values into the data structure     **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing AT com-   **
 **       mand line                                  **
 **      length:  Number of bytes that should be processed   **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   at_command:  AT command data structure to be filled     **
 **      Return:  The number of AT commands successfully     **
 **       decoded; RETURNerror if an error occurred  **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int at_command_decode(const char* buffer, int length, at_command_t* at_command)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  if (length < 3) {
    /* Minimum size to carry an AT command is 3 */
    LOG_TRACE(ERROR, "USR-API   - Input is short of size to carry an AT command!");
    return RETURNerror;
  } else if (strncasecmp(buffer, "AT", 2) != 0) {
    /* There must be AT initials in input... */
    LOG_TRACE(ERROR, "USR-API   - There is no proper initials (AT) in input!");
    return RETURNerror;
  } else if (buffer[length - 1] != '\r') {
    /* And there must be CR at the end */
    LOG_TRACE(ERROR, "USR-API   - There is no Carriage Return at the end!");
    return RETURNerror;
  } else if (strcasecmp(buffer, "AT\r") == 0) {
    /* AT command alone; just return OK */
    at_command->id = 0;
    at_command->mask = AT_COMMAND_NO_PARAM;
    return (1);
  }

  /* Parse the AT command line */
  int i;
  char* buf = strdup(buffer+2);
  char* cmd = strtok(buf, ";");

  // FIXME check overflow
  for (i=0; cmd && (rc != RETURNerror); i++) {
    rc = ParseString(cmd, &at_command[i]);
    cmd = strtok(NULL, ";");
  }

  free(buf);

  if (rc != RETURNerror) {
    LOG_FUNC_RETURN (i);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:  at_command_get_list()                                     **
 **                                                                        **
 ** Description: Returns the list of supported AT commands.                **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing the       **
 **       string representaton of the first AT com-  **
 **       mand that is supported by the NAS sublayer **
 **      n_max:   Maximum number of AT commands the buffer   **
 **       may contain                                **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:  The number of AT commands actually suppor- **
 **       ted by the NAS sublayer                    **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int at_command_get_list(const char** buffer, int n_max)
{
  LOG_FUNC_IN;

  int handler_index = 0;

  while ( (strcmp(_at_command_handler[handler_index].command, "null") != 0) &&
          (handler_index < n_max) ) {
    *buffer++ = _at_command_handler[handler_index++].command;
  }

  LOG_FUNC_RETURN (handler_index);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  ParseString()                                             **
 **                                                                        **
 ** Description: Calls the appropriate parsing handler after having        **
 **    successfully parsed the AT command prefix.                **
 **                                                                        **
 ** Inputs:  at_str:  Pointer to the buffer containing AT com-   **
 **       mand line                                  **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   at_command:  AT command structure to be filled          **
 **      Return:  The AT command identifier when the buffer  **
 **       has been successfully processed;           **
 **       RETURNerror otherwise.                     **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int ParseString(char * at_str, at_command_t* at_command)
{
  char* terminal_character_index = NULL;
  int num_of_chars_to_compare = -1;
  int handler_index;
  int handler_found = FALSE;
  int check_for_carriage_return_index = 0;

  /*
   * Find the number of characters we should be comparing against
   * the command list
   */
  if ((terminal_character_index = strstr(at_str, "=?")) != NULL) {
    /* `test' command */
    num_of_chars_to_compare = terminal_character_index - at_str + 2;
    check_for_carriage_return_index = 1;
    at_command->type = AT_COMMAND_TST;
  } else if ((terminal_character_index = strchr(at_str, '?')) != NULL) {
    /* `read' command */
    num_of_chars_to_compare = terminal_character_index - at_str + 1;
    check_for_carriage_return_index = 1;
    at_command->type = AT_COMMAND_GET;
  } else if ((terminal_character_index = strchr(at_str, '=')) != NULL) {
    /* `set' command */
    num_of_chars_to_compare = terminal_character_index - at_str + 1;
    at_command->type = AT_COMMAND_SET;
  } else if ((terminal_character_index = strchr(at_str, '\r')) != NULL) {
    /* Basic/action command */
    num_of_chars_to_compare = terminal_character_index - at_str;
    check_for_carriage_return_index = 1;
    at_command->type = AT_COMMAND_ACT;
  } else {
    /* One action command among many commands */
    num_of_chars_to_compare = strchr(at_str, '\0') - at_str;
    at_command->type = AT_COMMAND_ACT;
  }

  /*
   * The AT command strings in `Commands' list (that we compare the incoming
   * command against) don't have CR at the end. So we have to ensure that CR
   * comes just after characters like = and ?
   */
  if (check_for_carriage_return_index == 1) {
    if ( (at_str[num_of_chars_to_compare] != '\0') &&
         (at_str[num_of_chars_to_compare] != '\r') ) {
      LOG_TRACE(ERROR, "USR-API   - Carriage return is not at where it should be!");
      return RETURNerror;
    }
  }

  /* Check if this command has a relevant command handler */
  for (handler_index=0;
       strcmp(_at_command_handler[handler_index].command, "null") != 0;
       handler_index++) {
    /* First match the command types... */
    if (at_command->type != _at_command_handler[handler_index].type)
      continue;

    /* Match the command length... */
    if (strlen(_at_command_handler[handler_index].command) !=
        num_of_chars_to_compare) continue;

    /* Now do string comparison... */
    if (strncasecmp(at_str, _at_command_handler[handler_index].command,
                    num_of_chars_to_compare) != 0) continue;

    handler_found = TRUE;
    break;
  }

  /* Call the relevant handler or return with error */
  if (!handler_found) {
    LOG_TRACE(ERROR, "USR-API   - Unsupported or invalid command!");
    return RETURNerror;
  }

  return _at_command_handler[handler_index].handler(at_str,
         num_of_chars_to_compare,
         at_command);
}

/****************************************************************************
 **                                                                        **
 ** Name:  ParseCommand()                                            **
 **                                                                        **
 ** Description: Parses the input string on the basis of the format string **
 **    provided.                                                 **
 **                                                                        **
 ** Inputs:  str:   The string to be parsed                    **
 **    format:  The format of the input string             **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **      Return:  RETURNok, RETURNerror                      **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int ParseCommand(const unsigned char *str, const char *format, ...)
{
  const unsigned char *fmt = (unsigned char*) format;
  const char *inp = (char*) str;
  char *endptr;
  char *out_us;
  char *buffer;
  size_t length;
  size_t storage_size;
  int *out_i;
  long int *out_l;
  va_list ap;
  int error = RETURNok;

  va_start(ap, format);

  while (*fmt) {
    switch (*fmt++) {
    case '@':
      if (*fmt == 0) {
        /* Invalid format string */
        LOG_TRACE(ERROR, "USR-API   - Invalid format string!");
        error = RETURNerror;
        goto end;
      }

      switch (*fmt++) {
      case 'i':
        out_i = va_arg(ap, int *);
        *out_i = strtol(inp, &endptr, 10);

        if (endptr == inp) {
          error = RETURNerror;
          goto end;
        }

        inp = endptr;
        break;

      case 'I':
        out_i = va_arg(ap, int *);
        *out_i = strtol(inp, &endptr, 10);

        if (endptr == inp) {
          /* Number empty */
          /* optional parameter ??? */
          LOG_TRACE(WARNING, "USR-API   - Number empty!");
          *out_i = 0;
        } else {
          inp = endptr;
        }

        break;

      case 'l':
        out_l = va_arg(ap, long int *);
        *out_l = strtol(inp, &endptr, 10);

        if (endptr == inp) {
          error = RETURNerror;
          goto end;
        }

        inp = endptr;
        break;

      case 'r':
        out_us = va_arg(ap, char *);
        storage_size = va_arg(ap, size_t);
        length = GrabString((unsigned char*) inp, &buffer);

        if (strlen(buffer) > storage_size) {
          free(buffer);
          error = RETURNerror;
          goto end;
        }

        strcpy(out_us, buffer);
        free(buffer);
        inp += length;
        break;

      case '@':
        if (*inp++ != '@') {
          error = RETURNerror;
          goto end;
        }

        break;

      case '0':
        /* Just skip the rest */
        goto end;

      default:
        /* Invalid format string */
        error = RETURNerror;
        goto end;
      }

      break;

    case ' ':
      while (isspace(*inp))
        inp++;

      break;

    default:
      if (*inp++ != *(fmt - 1)) {
        error = RETURNerror;
        goto end;
      }

      break;
    }
  }

  /* Ignore trailing spaces */
  while (isspace(*inp))
    inp++;

  if (*inp != 0) {
    error = RETURNerror;
    goto end;
  }

end:
  va_end(ap);

  return error;
}

/****************************************************************************
 **                                                                        **
 ** Name:  GrabString()                                              **
 **                                                                        **
 ** Description: Grabs single string, removing possible quotes.            **
 **                                                                        **
 ** Inputs:  str:   The string to be parsed                    **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   output:  Allocated output string buffer             **
 **      Return:  Length of parsed string                    **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static size_t GrabString(const unsigned char *str, char **output)
{
  size_t size = 4, position = 0;
  int inside_quotes = 0;

  /* Allocate initial buffer in case string is empty */
  *output = malloc(size);

  if (*output == NULL) {
    return 0;
  }

  while (inside_quotes || (*str != ',' && *str != ')' &&
                           *str != 0x0d && *str != 0x0a && *str != 0x00)) {
    /* Check for quotes */
    if (*str == '"') {
      inside_quotes = !inside_quotes;
    }

    /* We also allocate space for trailing zero */
    if (position + 2 > size) {
      size += 10;
      *output = realloc(*output, size);

      if (*output == NULL) {
        return 0;
      }
    }

    /* Copy to output */
    (*output)[position] = *str;
    position++;
    str++;
  }

  (*output)[position] = 0;

  /* Strip quotes */
  if ((*output)[0] == '"' && (*output)[position - 1]) {
    memmove(*output, (*output) + 1, position - 2);
    (*output)[position - 2] = 0;
  }

  return position;
}

/****************************************************************************
 **                                                                        **
 ** Name:  NumberOfParameters()                                      **
 **                                                                        **
 ** Description: Returns the number of parameters found in the given AT    **
 **    command line.                                             **
 **                                                                        **
 ** Inputs:  str:   The string to be parsed                    **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **      Return:  The number of parameter delimiter (comma)  **
 **       found in the input string buffer           **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static unsigned int NumberOfParameters(const unsigned char* str)
{
  if (*str == '\r')
    return 0;

  int count = 1;

  while (*str) {
    if (*str++ == ',') {
      ++count;
    }
  }

  return count;
}

/****************************************************************************
 **                                                                        **
 ** Name:  IsNumeric()                                               **
 **                                                                        **
 ** Description: Checks whether the input string buffer has all digits     **
 **                                                                        **
 ** Inputs:  str:   The string to be parsed                    **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **      Return:  TRUE, FALSE                                **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static unsigned int IsNumeric(const char* str, size_t len)
{
  while (len-- > 0) {
    if (!isdigit(str[len])) {
      return FALSE;
    }
  }

  return TRUE;
}

/****************************************************************************/
/******************** Handlers used to parse AT commands ********************/
/****************************************************************************/

static int parse_e0(const char* string, int position, at_command_t* at_command)
{
  /* Command echo E0 */
  at_command->id = 0;
  at_command->mask = AT_COMMAND_NO_PARAM;
  return at_command->id;
}

static int parse_e1(const char* string, int position, at_command_t* at_command)
{
  /* Command echo E1 */
  at_command->id = 0;
  at_command->mask = AT_COMMAND_NO_PARAM;
  return at_command->id;
}

static int parse_q0(const char* string, int position, at_command_t* at_command)
{
  /* Result code suppression Q0 */
  at_error_code_suppression_q1 = FALSE;
  at_command->id = 0;
  at_command->mask = AT_COMMAND_NO_PARAM;
  return at_command->id;
}

static int parse_q1(const char* string, int position, at_command_t* at_command)
{
  /* Result code suppression Q1 */
  at_error_code_suppression_q1 = TRUE;
  at_command->id = 0;
  at_command->mask = AT_COMMAND_NO_PARAM;
  return at_command->id;
}

static int parse_v0(const char* string, int position, at_command_t* at_command)
{
  /* Response format V0 (numeric result code + limited header) */
  at_error_format = AT_ERROR_NUMERIC;
  at_response_format_v1 = FALSE;
  at_command->id = 0;
  at_command->mask = AT_COMMAND_NO_PARAM;
  return at_command->id;
}

static int parse_v1(const char* string, int position, at_command_t* at_command)
{
  /* Response format V1 (verbose result code + full header) */
  at_error_format = AT_ERROR_VERBOSE;
  at_response_format_v1 = TRUE;
  at_command->id = 0;
  at_command->mask = AT_COMMAND_NO_PARAM;
  return at_command->id;
}

static int parse_cgsn(const char* string, int position, at_command_t* at_command)
{
  /* CGSN action command - No parameter */
  at_command->id = AT_CGSN;
  at_command->mask = AT_COMMAND_CGSN_MASK;
  return at_command->id;
}

static int parse_cgsn_test(const char* string, int position, at_command_t* at_command)
{
  /* CGSN test command - No parameter */
  at_command->id = AT_CGSN;
  return at_command->id;
}

static int parse_cgmi(const char* string, int position, at_command_t* at_command)
{
  /* CGMI action command - No parameter */
  at_command->id = AT_CGMI;
  at_command->mask = AT_COMMAND_CGMI_MASK;
  return at_command->id;
}

static int parse_cgmi_test(const char* string, int position, at_command_t* at_command)
{
  /* CGMI test command - No parameter */
  at_command->id = AT_CGMI;
  return at_command->id;
}

static int parse_cgmm(const char* string, int position, at_command_t* at_command)
{
  /* CGMM action command - No parameter */
  at_command->id = AT_CGMM;
  at_command->mask = AT_COMMAND_CGMM_MASK;
  return at_command->id;
}

static int parse_cgmm_test(const char* string, int position, at_command_t* at_command)
{
  /* CGMM test command - No parameter */
  at_command->id = AT_CGMM;
  return at_command->id;
}

static int parse_cgmr(const char* string, int position, at_command_t* at_command)
{
  /* CGMR action command - No parameter */
  at_command->id = AT_CGMR;
  at_command->mask = AT_COMMAND_CGMR_MASK;
  return at_command->id;
}

static int parse_cgmr_test(const char* string, int position, at_command_t* at_command)
{
  /* CGMR test command - No parameter */
  at_command->id = AT_CGMR;
  return at_command->id;
}

static int parse_cimi(const char* string, int position, at_command_t* at_command)
{
  /* CIMI action command - No parameter */
  at_command->id = AT_CIMI;
  at_command->mask = AT_COMMAND_CIMI_MASK;
  return at_command->id;
}

static int parse_cimi_test(const char* string, int position, at_command_t* at_command)
{
  /* CIMI test command - No parameter */
  at_command->id = AT_CIMI;
  return at_command->id;
}

static int parse_cfun_set(const char* string, int position, at_command_t* at_command)
{
  /* CFUN parameter command - Parameters [<fun>[,<rst>]] */
  at_command->id = AT_CFUN;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  /* Parse the parameters */
  switch (number_of_parameters) {
  case 0: // No any parameter
    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CFUN= command succeed");
    at_command->mask = AT_COMMAND_NO_PARAM;
    break;

  case 1: // Only <fun> parameter is present
    if (ParseCommand(parameter_start_index, "@i",
                     &at_command->command.cfun.fun) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CFUN=<fun> command failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CFUN=<fun> command succeed (fun:%d)", at_command->command.cfun.fun);

    at_command->mask = AT_CFUN_FUN_MASK;
    break;

  case 2: // <fun>,<rst> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@i",
                     &at_command->command.cfun.fun,
                     &at_command->command.cfun.rst) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CFUN=<fun>,<rst> command failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CFUN=<fun>,<rst> command succeed (fun:%d, rst:%d)", at_command->command.cfun.fun, at_command->command.cfun.rst);

    at_command->mask = AT_CFUN_FUN_MASK | AT_CFUN_RST_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return RETURNerror;
  }

  return at_command->id;
}

static int parse_cfun_get(const char* string, int position, at_command_t* at_command)
{
  /* CFUN action command - No parameter */
  at_command->id = AT_CFUN;
  return at_command->id;
}

static int parse_cfun_test(const char* string, int position, at_command_t* at_command)
{
  /* CFUN test command - No parameter */
  at_command->id = AT_CFUN;
  return at_command->id;
}

static int parse_cpin_set(const char* string, int position, at_command_t* at_command)
{
  /* CPIN parameter command - Parameters <pin>[,<newpin>] */
  at_command->id = AT_CPIN;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  /* Parse the parameters */
  switch (number_of_parameters) {
  case 1: // Only <pin> parameter is present
    if (ParseCommand(parameter_start_index, "@r",
                     at_command->command.cpin.pin, AT_CPIN_SIZE) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CPIN=<pin> command failed");
      return RETURNerror;
    }

    if (!IsNumeric(at_command->command.cpin.pin, AT_CPIN_SIZE)) {
      LOG_TRACE(ERROR, "USR-API   - Incoming parameter is not a valid PIN code!");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CPIN=<pin> command succeed (pin:%s)", at_command->command.cpin.pin);

    at_command->mask = AT_COMMAND_NO_PARAM;
    break;

  case 2: // <pin>,<newpin> parameters are present
    if (ParseCommand(parameter_start_index, "@r,@r",
                     at_command->command.cpin.pin, AT_CPIN_SIZE,
                     at_command->command.cpin.newpin, AT_CPIN_SIZE) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CPIN=<pin>,<newpin> command failed");
      return RETURNerror;
    }

    if (!IsNumeric(at_command->command.cpin.pin, AT_CPIN_SIZE) ||
        !IsNumeric(at_command->command.cpin.newpin, AT_CPIN_SIZE)) {
      LOG_TRACE(ERROR, "USR-API   - Incoming parameter is not a valid PIN code!");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CPIN=<pin>,<newpin> command succeed (pin:%s, newpin:%s)", at_command->command.cpin.pin, at_command->command.cpin.newpin);

    at_command->mask = AT_CPIN_NEWPIN_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return RETURNerror;
  }

  return at_command->id;
}

static int parse_cpin_get(const char* string, int position, at_command_t* at_command)
{
  /* CPIN action command - No parameter */
  at_command->id = AT_CPIN;
  return at_command->id;
}

static int parse_cpin_test(const char* string, int position, at_command_t* at_command)
{
  /* CPIN test command - No parameter */
  at_command->id = AT_CPIN;
  return at_command->id;
}

static int parse_csq(const char* string, int position, at_command_t* at_command)
{
  /* CSQ action command - No parameter */
  at_command->id = AT_CSQ;
  at_command->mask = AT_COMMAND_CSQ_MASK;
  return at_command->id;
}

static int parse_csq_test(const char* string, int position, at_command_t* at_command)
{
  /* CSQ test command - No parameter */
  at_command->id = AT_CSQ;
  return at_command->id;
}

static int parse_cesq(const char* string, int position, at_command_t* at_command)
{
  /* CESQ action command - No parameter */
  at_command->id = AT_CESQ;
  at_command->mask = AT_COMMAND_CESQ_MASK;
  return at_command->id;
}

static int parse_cesq_test(const char* string, int position, at_command_t* at_command)
{
  /* CESQ test command - No parameter */
  at_command->id = AT_CESQ;
  return at_command->id;
}

static int parse_clac(const char* string, int position, at_command_t* at_command)
{
  /* CLAC action command - No parameter */
  at_command->id = AT_CLAC;
  at_command->mask = AT_COMMAND_CLAC_MASK;
  return at_command->id;
}

static int parse_clac_test(const char* string, int position, at_command_t* at_command)
{
  /* CLAC test command - No parameter */
  at_command->id = AT_CLAC;
  return at_command->id;
}

static int parse_cmee_set(const char* string, int position, at_command_t* at_command)
{
  /* CMEE parameter command - Parameter [<n>] */
  at_command->id = AT_CMEE;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  /* Parse the parameters */
  switch (number_of_parameters) {
  case 0: // No any parameter
    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CMEE= command succeed");
    at_command->mask = AT_COMMAND_NO_PARAM;
    break;

  case 1: // Only <n> parameter is present
    if (ParseCommand(parameter_start_index, "@i",
                     &at_command->command.cmee.n) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CMEE=<n> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CMEE=<n> command succeed (n:%d)", at_command->command.cmee.n);

    at_command->mask = AT_CMEE_N_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return RETURNerror;
  }

  return at_command->id;
}

static int parse_cmee_get(const char* string, int position, at_command_t* at_command)
{
  /* CMEE read command - No parameter */
  at_command->id = AT_CMEE;
  return at_command->id;
}

static int parse_cmee_test(const char* string, int position, at_command_t* at_command)
{
  /* CMEE test command - No parameter */
  at_command->id = AT_CMEE;
  return at_command->id;
}

static int parse_cnum(const char* string, int position, at_command_t* at_command)
{
  /* CNUM action command - No parameter */
  at_command->id = AT_CNUM;
  at_command->mask = AT_COMMAND_CNUM_MASK;
  return at_command->id;
}

static int parse_cnum_test(const char* string, int position, at_command_t* at_command)
{
  /* CNUM test command - No parameter */
  at_command->id = AT_CNUM;
  return at_command->id;
}

static int parse_clck_set(const char* string, int position, at_command_t* at_command)
{
  /* CLCK action command - Parameters <fac>,<mode>[,<passwd>] */
  at_command->id = AT_CLCK;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  switch (number_of_parameters) {
  case 2: // <fac>,<mode> parameters are present
    if (ParseCommand(parameter_start_index, "@r,@i",
                     &at_command->command.clck.fac, AT_CLCK_FAC_SIZE,
                     &at_command->command.clck.mode) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CLCK=<fac>,<mode> "
                "failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CLCK=<fac>,<mode> "
              "succeed (fac:%s, mode:%d)",
              at_command->command.clck.fac,
              at_command->command.clck.mode);
    at_command->mask = AT_COMMAND_NO_PARAM;
    break;

  case 3: // <fac>,<mode>,<passwd> parameters are present
    if (ParseCommand(parameter_start_index, "@r,@i,@r",
                     &at_command->command.clck.fac, AT_CLCK_FAC_SIZE,
                     &at_command->command.clck.mode,
                     &at_command->command.clck.passwd,
                     AT_CLCK_PASSWD_SIZE) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CLCK=<fac>,<mode>"
                "<passwd> failed");
      return RETURNerror;
    }

    if (!IsNumeric(at_command->command.clck.passwd,AT_CLCK_PASSWD_SIZE)) {
      LOG_TRACE(ERROR, "USR-API   - Incoming parameter is not a valid password!");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CLCK=<fac>,<mode>"
              "<passwd> succeed (fac:%s, mode:%d, passwd:%s)",
              at_command->command.clck.fac,
              at_command->command.clck.mode,
              at_command->command.clck.passwd);
    at_command->mask = AT_CLCK_PASSWD_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return -1;
  }

  return at_command->id;
}

static int parse_clck_test(const char* string, int position, at_command_t* at_command)
{
  /* CLCK test command - No parameter */
  at_command->id = AT_CLCK;
  return at_command->id;
}

static int parse_cops_set(const char* string, int position, at_command_t* at_command)
{
  /* COPS parameter command - Parameters [<mode>[,<format>[,<oper>[,<AcT>]]]] */
  at_command->id = AT_COPS;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  /* Parse the parameters */
  char oper_value[AT_COPS_NUM_SIZE];

  switch (number_of_parameters) {
  case 0: // No any parameter
    LOG_TRACE(INFO, "USR-API   - Parsing of AT+COPS= command succeed");
    at_command->mask = AT_COMMAND_NO_PARAM;
    break;

  case 1: // Only <mode> parameter is present
    if (ParseCommand(parameter_start_index, "@i",
                     &at_command->command.cops.mode) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+COPS=<mode> command failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+COPS=<mode> command succeed (mode:%d)", at_command->command.cops.mode);

    at_command->mask = AT_COPS_MODE_MASK;
    break;

  case 2: // <mode>,<format> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@i",
                     &at_command->command.cops.mode,
                     &at_command->command.cops.format) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+COPS=<mode>,<format> command failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+COPS=<mode>,<format> command succeed (mode:%d, format:%d)", at_command->command.cops.mode, at_command->command.cops.format);

    at_command->mask = AT_COPS_MODE_MASK | AT_COPS_FORMAT_MASK;
    break;

  case 3: // <mode>,<format>,<oper> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@i,@r",
                     &at_command->command.cops.mode,
                     &at_command->command.cops.format,
                     oper_value, NET_FORMAT_MAX_SIZE) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+COPS=<mode>,<format>,<oper> command failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+COPS=<mode>,<format>,<oper> command succeed (mode:%d, format:%d)", at_command->command.cops.mode, at_command->command.cops.format);

    /* Now, process <oper> parameter according to the specified <format> parameter */
    switch (at_command->command.cops.format) {
    case AT_COPS_LONG: /* long format alphanumeric <oper> */
      strncpy((char*) at_command->command.cops.plmn.id.alpha_long,
              oper_value, AT_COPS_LONG_SIZE);
      LOG_TRACE(INFO, "USR-API   - <oper> parameter's value is '%s'", at_command->command.cops.plmn.id.alpha_long);
      break;

    case AT_COPS_SHORT: /* short format alphanumeric <oper> */
      strncpy((char*) at_command->command.cops.plmn.id.alpha_short, oper_value, AT_COPS_SHORT_SIZE);
      LOG_TRACE(INFO, "USR-API   - <oper> parameter's value is '%s'", at_command->command.cops.plmn.id.alpha_short);
      break;

    case AT_COPS_NUM: /* numeric <oper> */
      strncpy((char*) at_command->command.cops.plmn.id.num,
              oper_value, AT_COPS_NUM_SIZE);
      LOG_TRACE(INFO, "USR-API   - <oper> parameter's value is '%s'", at_command->command.cops.plmn.id.num);
      break;

    default:
      LOG_TRACE(ERROR, "USR-API   - Invalid format value (%d)",
                at_command->command.cops.format);
      return RETURNerror;
    }

    at_command->mask = AT_COPS_MODE_MASK | AT_COPS_FORMAT_MASK | AT_COPS_OPER_MASK;
    break;

  case 4: // <mode>,<format>,<oper>,<AcT> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@i,@0",
                     &at_command->command.cops.mode,
                     &at_command->command.cops.format) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+COPS=<mode>,<format> command failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+COPS=<mode>,<format>,<oper> command succeed (mode:%d, format:%d)", at_command->command.cops.mode, at_command->command.cops.format);

    /* Now, process <oper> parameter according to the specified <format> parameter */
    switch (at_command->command.cops.format) {
    case AT_COPS_LONG: /* long format alphanumeric <oper> */
      if (ParseCommand(parameter_start_index, "@i,@i,@r,@i",
                       &at_command->command.cops.mode,
                       &at_command->command.cops.format,
                       oper_value, AT_COPS_LONG_SIZE,
                       &at_command->command.cops.AcT) != RETURNok) {
        LOG_TRACE(ERROR, "USR-API   - Parsing of AT+COPS=<mode>,<format>,<oper>,<AcT> command failed");
        return RETURNerror;
      }

      strncpy((char*) at_command->command.cops.plmn.id.alpha_long,
              oper_value, AT_COPS_LONG_SIZE);
      LOG_TRACE(INFO, "USR-API   - Parsing of AT+COPS=<mode>,<format>,<oper>,<AcT> command succeed (oper:%s, Act:%d)", at_command->command.cops.plmn.id.alpha_long, at_command->command.cops.AcT);
      break;

    case AT_COPS_SHORT: /* short format alphanumeric <oper> */
      if (ParseCommand(parameter_start_index, "@i,@i,@r,@i",
                       &at_command->command.cops.mode,
                       &at_command->command.cops.format,
                       oper_value, AT_COPS_SHORT_SIZE,
                       &at_command->command.cops.AcT) != RETURNok) {
        LOG_TRACE(ERROR, "USR-API   - Parsing of AT+COPS=<mode>,<format>,<oper>,<AcT> command failed");
        return RETURNerror;
      }

      strncpy((char*) at_command->command.cops.plmn.id.alpha_short,
              oper_value, AT_COPS_SHORT_SIZE);
      LOG_TRACE(INFO, "USR-API   - Parsing of AT+COPS=<mode>,<format>,<oper>,<AcT> command succeed (oper:%s, Act:%d)",
                at_command->command.cops.plmn.id.alpha_short,
                at_command->command.cops.AcT);
      break;

    case AT_COPS_NUM: /* numeric <oper> */
      if (ParseCommand(parameter_start_index, "@i,@i,@r,@i",
                       &at_command->command.cops.mode,
                       &at_command->command.cops.format,
                       oper_value, AT_COPS_NUM_SIZE,
                       &at_command->command.cops.AcT) != RETURNok) {
        LOG_TRACE(ERROR, "USR-API   - Parsing of AT+COPS=<mode>,<format>,<oper>,<AcT> command failed");
        return RETURNerror;
      }

      strncpy((char*) at_command->command.cops.plmn.id.num,
              oper_value, AT_COPS_NUM_SIZE);
      LOG_TRACE(INFO, "USR-API   - Parsing of AT+COPS=<mode>,<format>,<oper>,<AcT> command succeed (oper:%s, Act:%d)",
                at_command->command.cops.plmn.id.num,
                at_command->command.cops.AcT);
      break;

    default:
      LOG_TRACE(ERROR, "USR-API   - Invalid format value (%d)",
                at_command->command.cops.format);
      return RETURNerror;
    }

    at_command->mask = AT_COPS_MODE_MASK | AT_COPS_FORMAT_MASK | AT_COPS_OPER_MASK | AT_COPS_ACT_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return RETURNerror;
  }

  return at_command->id;
}

static int parse_cops_get(const char* string, int position, at_command_t* at_command)
{
  /* COPS test command - No parameter */
  at_command->id = AT_COPS;
  return at_command->id;
}

static int parse_cops_test(const char* string, int position, at_command_t* at_command)
{
  /* COPS test command - No parameter */
  at_command->id = AT_COPS;
  return at_command->id;
}

static int parse_creg_set(const char* string, int position, at_command_t* at_command)
{
  /* CREG parameter command - Parameter [<n>] */
  at_command->id = AT_CREG;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  /* Parse the parameters */
  switch (number_of_parameters) {
  case 0: // No any parameter
    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CREG= command succeed");
    at_command->mask = AT_COMMAND_NO_PARAM;
    break;

  case 1: // Only <n> parameter is present
    if (ParseCommand(parameter_start_index, "@i",
                     &at_command->command.creg.n) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CREG=<n> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CREG=<n> command succeed (n:%d)", at_command->command.creg.n);

    at_command->mask = AT_CREG_N_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return RETURNerror;
  }

  return at_command->id;
}

static int parse_creg_get(const char* string, int position, at_command_t* at_command)
{
  /* CREG read command - No parameter */
  at_command->id = AT_CREG;
  return at_command->id;
}

static int parse_creg_test(const char* string, int position, at_command_t* at_command)
{
  /* CREG test command - No parameter */
  at_command->id = AT_CREG;
  return at_command->id;
}

static int parse_cgatt_set(const char* string, int position, at_command_t* at_command)
{
  /* CGATT parameter command - Parameter [<state>] */
  at_command->id = AT_CGATT;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  /* Parse the parameters */
  switch (number_of_parameters) {
  case 0: // No any parameter
    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGATT= command succeed");
    at_command->mask = AT_COMMAND_NO_PARAM;
    break;

  case 1: // Only <state> parameter is present
    if (ParseCommand(parameter_start_index, "@i",
                     &at_command->command.cgatt.state) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGATT=<state> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGATT=<state> command succeed (state:%d)", at_command->command.cgatt.state);

    at_command->mask = AT_CGATT_STATE_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return RETURNerror;
  }

  return at_command->id;
}

static int parse_cgatt_get(const char* string, int position, at_command_t* at_command)
{
  /* CGATT read command - No parameter */
  at_command->id = AT_CGATT;
  return at_command->id;
}

static int parse_cgatt_test(const char* string, int position, at_command_t* at_command)
{
  /* CGATT test command - No parameter */
  at_command->id = AT_CGATT;
  return at_command->id;
}

static int parse_cgreg_set(const char* string, int position, at_command_t* at_command)
{
  /* CGREG parameter command - Parameter [<n>] */
  at_command->id = AT_CGREG;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  /* Parse the parameters */
  switch (number_of_parameters) {
  case 0: // No any parameter
    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGREG= command succeed");
    at_command->mask = AT_COMMAND_NO_PARAM;
    break;

  case 1: // Only <n> parameter is present
    if (ParseCommand(parameter_start_index, "@i",
                     &at_command->command.cgreg.n) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGREG=<n> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGREG=<n> command succeed (n:%d)", at_command->command.cgreg.n);

    at_command->mask = AT_CGREG_N_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return RETURNerror;
  }

  return at_command->id;
}

static int parse_cgreg_get(const char* string, int position, at_command_t* at_command)
{
  /* CGREG read command - No parameter */
  at_command->id = AT_CGREG;
  return at_command->id;
}

static int parse_cgreg_test(const char* string, int position, at_command_t* at_command)
{
  /* CGREG test command - No parameter */
  at_command->id = AT_CGREG;
  return at_command->id;
}

static int parse_cereg_set(const char* string, int position, at_command_t* at_command)
{
  /* CEREG parameter command - Parameter [<n>] */
  at_command->id = AT_CEREG;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  /* Parse the parameters */
  switch (number_of_parameters) {
  case 0: // No any parameter
    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CEREG= command succeed");
    at_command->mask = AT_COMMAND_NO_PARAM;
    break;

  case 1: // Only <n> parameter is present
    if (ParseCommand(parameter_start_index, "@i",
                     &at_command->command.cereg.n) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CEREG=<n> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CEREG=<n> command succeed (n:%d)", at_command->command.cereg.n);

    at_command->mask = AT_CEREG_N_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return RETURNerror;
  }

  return at_command->id;
}

static int parse_cereg_get(const char* string, int position, at_command_t* at_command)
{
  /* CEREG read command - No parameter */
  at_command->id = AT_CEREG;
  return at_command->id;
}

static int parse_cereg_test(const char* string, int position, at_command_t* at_command)
{
  /* CEREG test command - No parameter */
  at_command->id = AT_CEREG;
  return at_command->id;
}

static int parse_cgdcont_set(const char* string, int position, at_command_t* at_command)
{
  /* CGDCONT parameter command - Parameters [<cid>[,<PDP_type>[,<APN>[,<PDP_addr>[,<d_comp>[,<h_comp>[,<IPv4AddrAlloc>[,<emergency indication>[,<P-CSCF_discovery>[,<IM_CN_Signalling_Flag_Ind>]]]]]]]]]] */
  at_command->id = AT_CGDCONT;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  /* Parse the parameters */
  switch (number_of_parameters) {
  case 1: // Only <cid> parameter is present
    if (ParseCommand(parameter_start_index, "@i",
                     &at_command->command.cgdcont.cid) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGDCONT=<cid> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGDCONT=<cid> command succeed (cid:%d)", at_command->command.cgdcont.cid);

    at_command->mask = AT_CGDCONT_CID_MASK;
    break;

  case 2: // <cid>,<PDP_type> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@r",
                     &at_command->command.cgdcont.cid,
                     &at_command->command.cgdcont.PDP_type, AT_CGDCONT_PDP_SIZE) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type> succeed (cid:%d, pdp_type:%s)",
              at_command->command.cgdcont.cid,
              at_command->command.cgdcont.PDP_type);

    at_command->mask = AT_CGDCONT_CID_MASK | AT_CGDCONT_PDP_TYPE_MASK;
    break;

  case 3: // <cid>,<PDP_type>,<APN> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@r,@r",
                     &at_command->command.cgdcont.cid,
                     &at_command->command.cgdcont.PDP_type, AT_CGDCONT_PDP_SIZE,
                     &at_command->command.cgdcont.APN, AT_CGDCONT_APN_SIZE) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN> succeed (cid:%d, pdp_type:%s, apn:%s)",
              at_command->command.cgdcont.cid,
              at_command->command.cgdcont.PDP_type,
              at_command->command.cgdcont.APN);

    at_command->mask = AT_CGDCONT_CID_MASK | AT_CGDCONT_PDP_TYPE_MASK | AT_CGDCONT_APN_MASK;
    break;

  case 4: // <cid>,<PDP_type>,<APN>,<PDP_addr> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@r,@r,@r",
                     &at_command->command.cgdcont.cid,
                     &at_command->command.cgdcont.PDP_type, AT_CGDCONT_PDP_SIZE,
                     &at_command->command.cgdcont.APN, AT_CGDCONT_APN_SIZE,
                     &at_command->command.cgdcont.PDP_addr, AT_CGDCONT_ADDR_SIZE) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr> succeed (cid:%d, pdp_type:%s, apn:%s, pdp_addr:%s)",
              at_command->command.cgdcont.cid,
              at_command->command.cgdcont.PDP_type,
              at_command->command.cgdcont.APN,
              at_command->command.cgdcont.PDP_addr);

    at_command->mask = AT_CGDCONT_CID_MASK | AT_CGDCONT_PDP_TYPE_MASK | AT_CGDCONT_APN_MASK | AT_CGDCONT_PDP_ADDR_MASK;
    break;

  case 5: // <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@r,@r,@r,@i",
                     &at_command->command.cgdcont.cid,
                     &at_command->command.cgdcont.PDP_type, AT_CGDCONT_PDP_SIZE,
                     &at_command->command.cgdcont.APN, AT_CGDCONT_APN_SIZE,
                     &at_command->command.cgdcont.PDP_addr, AT_CGDCONT_ADDR_SIZE,
                     &at_command->command.cgdcont.d_comp) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp> succeed (cid:%d, pdp_type:%s, apn:%s, pdp_addr:%s, d_comp:%d)",
              at_command->command.cgdcont.cid,
              at_command->command.cgdcont.PDP_type,
              at_command->command.cgdcont.APN,
              at_command->command.cgdcont.PDP_addr,
              at_command->command.cgdcont.d_comp);

    at_command->mask = AT_CGDCONT_CID_MASK | AT_CGDCONT_PDP_TYPE_MASK | AT_CGDCONT_APN_MASK | AT_CGDCONT_PDP_ADDR_MASK | AT_CGDCONT_D_COMP_MASK;
    break;

  case 6: // <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@r,@r,@r,@i,@i",
                     &at_command->command.cgdcont.cid,
                     &at_command->command.cgdcont.PDP_type, AT_CGDCONT_PDP_SIZE,
                     &at_command->command.cgdcont.APN, AT_CGDCONT_APN_SIZE,
                     &at_command->command.cgdcont.PDP_addr, AT_CGDCONT_ADDR_SIZE,
                     &at_command->command.cgdcont.d_comp,
                     &at_command->command.cgdcont.h_comp) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp> succeed (cid:%d, pdp_type:%s, apn:%s, pdp_addr:%s, d_comp:%d, h_comp:%d)",
              at_command->command.cgdcont.cid,
              at_command->command.cgdcont.PDP_type,
              at_command->command.cgdcont.APN,
              at_command->command.cgdcont.PDP_addr,
              at_command->command.cgdcont.d_comp,
              at_command->command.cgdcont.h_comp);

    at_command->mask = AT_CGDCONT_CID_MASK | AT_CGDCONT_PDP_TYPE_MASK | AT_CGDCONT_APN_MASK | AT_CGDCONT_PDP_ADDR_MASK | AT_CGDCONT_D_COMP_MASK | AT_CGDCONT_H_COMP_MASK;
    break;

  case 7: // <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,<IPv4AddrAlloc> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@r,@r,@r,@i,@i,@i",
                     &at_command->command.cgdcont.cid,
                     &at_command->command.cgdcont.PDP_type, AT_CGDCONT_PDP_SIZE,
                     &at_command->command.cgdcont.APN, AT_CGDCONT_APN_SIZE,
                     &at_command->command.cgdcont.PDP_addr, AT_CGDCONT_ADDR_SIZE,
                     &at_command->command.cgdcont.d_comp,
                     &at_command->command.cgdcont.h_comp,
                     &at_command->command.cgdcont.IPv4AddrAlloc) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,<IPv4AddrAlloc> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO,
              "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,<IPv4AddrAlloc> succeed (cid:%d, pdp_type:%s, apn:%s, pdp_addr:%s, d_comp:%d, h_comp:%d, ipv4addralloc:%d)",
              at_command->command.cgdcont.cid,
              at_command->command.cgdcont.PDP_type,
              at_command->command.cgdcont.APN,
              at_command->command.cgdcont.PDP_addr,
              at_command->command.cgdcont.d_comp,
              at_command->command.cgdcont.h_comp,
              at_command->command.cgdcont.IPv4AddrAlloc);

    at_command->mask = AT_CGDCONT_CID_MASK | AT_CGDCONT_PDP_TYPE_MASK | AT_CGDCONT_APN_MASK | AT_CGDCONT_PDP_ADDR_MASK | AT_CGDCONT_D_COMP_MASK | AT_CGDCONT_H_COMP_MASK | AT_CGDCONT_IPV4ADDRALLOC_MASK;
    break;

  case 8: // <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,<IPv4AddrAlloc>,<emergency indication> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@r,@r,@r,@i,@i,@i,@i",
                     &at_command->command.cgdcont.cid,
                     &at_command->command.cgdcont.PDP_type, AT_CGDCONT_PDP_SIZE,
                     &at_command->command.cgdcont.APN, AT_CGDCONT_APN_SIZE,
                     &at_command->command.cgdcont.PDP_addr, AT_CGDCONT_ADDR_SIZE,
                     &at_command->command.cgdcont.d_comp,
                     &at_command->command.cgdcont.h_comp,
                     &at_command->command.cgdcont.IPv4AddrAlloc,
                     &at_command->command.cgdcont.emergency_indication) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,<IPv4AddrAlloc>,<emergency indication> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO,
              "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,<IPv4AddrAlloc>,<emergency indication> succeed (cid:%d, pdp_type:%s, apn:%s, pdp_addr:%s, d_comp:%d, h_comp:%d, ipv4addralloc:%d, emergency_indication:%d)",
              at_command->command.cgdcont.cid,
              at_command->command.cgdcont.PDP_type,
              at_command->command.cgdcont.APN,
              at_command->command.cgdcont.PDP_addr,
              at_command->command.cgdcont.d_comp,
              at_command->command.cgdcont.h_comp,
              at_command->command.cgdcont.IPv4AddrAlloc,
              at_command->command.cgdcont.emergency_indication);

    at_command->mask = AT_CGDCONT_CID_MASK | AT_CGDCONT_PDP_TYPE_MASK | AT_CGDCONT_APN_MASK | AT_CGDCONT_PDP_ADDR_MASK | AT_CGDCONT_D_COMP_MASK | AT_CGDCONT_H_COMP_MASK | AT_CGDCONT_IPV4ADDRALLOC_MASK |
                       AT_CGDCONT_EMERGECY_INDICATION_MASK;
    break;

  case 9: /* <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,
     * <IPv4AddrAlloc>,<emergency indication>,<P-CSCF_discovery>
     * parameters are present */
    if (ParseCommand(parameter_start_index, "@i,@r,@r,@r,@i,@i,@i,@i,@i",
                     &at_command->command.cgdcont.cid,
                     &at_command->command.cgdcont.PDP_type, AT_CGDCONT_PDP_SIZE,
                     &at_command->command.cgdcont.APN, AT_CGDCONT_APN_SIZE,
                     &at_command->command.cgdcont.PDP_addr, AT_CGDCONT_ADDR_SIZE,
                     &at_command->command.cgdcont.d_comp,
                     &at_command->command.cgdcont.h_comp,
                     &at_command->command.cgdcont.IPv4AddrAlloc,
                     &at_command->command.cgdcont.emergency_indication,
                     &at_command->command.cgdcont.P_CSCF_discovery) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,<IPv4AddrAlloc>,<emergency indication>,<P-CSCF_discovery> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO,
              "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,<IPv4AddrAlloc>,<emergency indication>,<P-CSCF_discovery> succeed (cid:%d, pdp_type:%s, apn:%s, pdp_addr:%s, d_comp:%d, h_comp:%d, ipv4addralloc:%d, emergency_indication:%d, p_cscf_discovery:%d)",
              at_command->command.cgdcont.cid,
              at_command->command.cgdcont.PDP_type,
              at_command->command.cgdcont.APN,
              at_command->command.cgdcont.PDP_addr,
              at_command->command.cgdcont.d_comp,
              at_command->command.cgdcont.h_comp,
              at_command->command.cgdcont.IPv4AddrAlloc,
              at_command->command.cgdcont.emergency_indication,
              at_command->command.cgdcont.P_CSCF_discovery);

    at_command->mask = AT_CGDCONT_CID_MASK | AT_CGDCONT_PDP_TYPE_MASK | AT_CGDCONT_APN_MASK | AT_CGDCONT_PDP_ADDR_MASK | AT_CGDCONT_D_COMP_MASK | AT_CGDCONT_H_COMP_MASK | AT_CGDCONT_IPV4ADDRALLOC_MASK |
                       AT_CGDCONT_EMERGECY_INDICATION_MASK | AT_CGDCONT_P_CSCF_DISCOVERY_MASK;
    break;

  case 10: /* <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,
     * <IPv4AddrAlloc>,<emergency indication>,<P-CSCF_discovery>,
     * <IM_CN_Signalling_Flag_Ind> parameters are present */
    if (ParseCommand(parameter_start_index, "@i,@r,@r,@r,@i,@i,@i,@i,@i,@i",
                     &at_command->command.cgdcont.cid,
                     &at_command->command.cgdcont.PDP_type, AT_CGDCONT_PDP_SIZE,
                     &at_command->command.cgdcont.APN, AT_CGDCONT_APN_SIZE,
                     &at_command->command.cgdcont.PDP_addr, AT_CGDCONT_ADDR_SIZE,
                     &at_command->command.cgdcont.d_comp,
                     &at_command->command.cgdcont.h_comp,
                     &at_command->command.cgdcont.IPv4AddrAlloc,
                     &at_command->command.cgdcont.emergency_indication,
                     &at_command->command.cgdcont.P_CSCF_discovery,
                     &at_command->command.cgdcont.IM_CN_Signalling_Flag_Ind) != RETURNok) {
      LOG_TRACE(ERROR,
                "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,<IPv4AddrAlloc>,<emergency indication>,<P-CSCF_discovery>, <IM_CN_Signalling_Flag_Ind> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO,
              "USR-API   - Parsing of AT+CGDCONT=<cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>,<IPv4AddrAlloc>,<emergency indication>,<P-CSCF_discovery>, <IM_CN_Signalling_Flag_Ind> succeed (cid:%d, pdp_type:%s, apn:%s, pdp_addr:%s, d_comp:%d, h_comp:%d, ipv4addralloc:%d, emergency_indication:%d, p_cscf_discovery:%d, im_cn_signalling_flag:%d)",
              at_command->command.cgdcont.cid,
              at_command->command.cgdcont.PDP_type,
              at_command->command.cgdcont.APN,
              at_command->command.cgdcont.PDP_addr,
              at_command->command.cgdcont.d_comp,
              at_command->command.cgdcont.h_comp,
              at_command->command.cgdcont.IPv4AddrAlloc,
              at_command->command.cgdcont.emergency_indication,
              at_command->command.cgdcont.P_CSCF_discovery,
              at_command->command.cgdcont.IM_CN_Signalling_Flag_Ind);

    at_command->mask = AT_CGDCONT_CID_MASK | AT_CGDCONT_PDP_TYPE_MASK | AT_CGDCONT_APN_MASK | AT_CGDCONT_PDP_ADDR_MASK | AT_CGDCONT_D_COMP_MASK | AT_CGDCONT_H_COMP_MASK | AT_CGDCONT_IPV4ADDRALLOC_MASK |
                       AT_CGDCONT_EMERGECY_INDICATION_MASK | AT_CGDCONT_P_CSCF_DISCOVERY_MASK | AT_CGDCONT_IM_CN_SIGNALLING_FLAG_IND_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return RETURNerror;
  }

  return at_command->id;
}

static int parse_cgdcont_get(const char* string, int position, at_command_t* at_command)
{
  /* CGDCONT read command - No parameter */
  at_command->id = AT_CGDCONT;
  return at_command->id;
}

static int parse_cgdcont_test(const char* string, int position, at_command_t* at_command)
{
  /* CGDCONT test command - No parameter */
  at_command->id = AT_CGDCONT;
  return at_command->id;
}

static int parse_cgact_set(const char* string, int position, at_command_t* at_command)
{
  /* CGACT parameter command - Parameters <state>[,<cid>] */
  at_command->id = AT_CGACT;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  switch (number_of_parameters) {
  case 1: // Only <state> parameter is present
    if (ParseCommand(parameter_start_index, "@i",
                     &at_command->command.cgact.state) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGACT=<state> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGACT=<state> succeed (state:%d)", at_command->command.cgact.state);
    at_command->mask = AT_CGACT_STATE_MASK;
    break;

  case 2: // <state>,<cid> parameters are present
    if (ParseCommand(parameter_start_index, "@i,@i",
                     &at_command->command.cgact.state,
                     &at_command->command.cgact.cid) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGACT=<state>,<cid> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGACT=<state>,<cid> succeed (state:%d, cid:%d)",
              at_command->command.cgact.state,
              at_command->command.cgact.cid);

    at_command->mask = AT_CGACT_STATE_MASK | AT_CGACT_CID_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return -1;
  }

  return at_command->id;
}

static int parse_cgact_get(const char* string, int position, at_command_t* at_command)
{
  /* CGACT read command - No parameter */
  at_command->id = AT_CGACT;
  return at_command->id;
}

static int parse_cgact_test(const char* string, int position, at_command_t* at_command)
{
  /* CGACT test command - No parameter */
  at_command->id = AT_CGACT;
  return at_command->id;
}

static int parse_cgpaddr_set(const char* string, int position, at_command_t* at_command)
{
  /* CGPADDR parameter command - Parameters [<cid>] */
  at_command->id = AT_CGPADDR;
  unsigned char* parameter_start_index = (unsigned char*) string + position;

  /* Check the number of parameters */
  unsigned number_of_parameters = NumberOfParameters(parameter_start_index);

  switch (number_of_parameters) {
  case 0: // No any parameter
    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGPADDR= command succeed");
    at_command->mask = AT_COMMAND_NO_PARAM;
    break;

  case 1: // Only <cid> parameter is present
    if (ParseCommand(parameter_start_index, "@i",
                     &at_command->command.cgpaddr.cid) != RETURNok) {
      LOG_TRACE(ERROR, "USR-API   - Parsing of AT+CGPADDR=<state> failed");
      return RETURNerror;
    }

    LOG_TRACE(INFO, "USR-API   - Parsing of AT+CGPADDR=<cid> succeed (cid:%d)", at_command->command.cgpaddr.cid);
    at_command->mask = AT_CGPADDR_CID_MASK;
    break;

  default:
    LOG_TRACE(ERROR, "USR-API   - Invalid number of parameters (%d)", number_of_parameters);
    return -1;
  }

  return at_command->id;
}

static int parse_cgpaddr_test(const char* string, int position, at_command_t* at_command)
{
  /* CGPADDR test command - No parameter */
  at_command->id = AT_CGPADDR;
  return at_command->id;
}
