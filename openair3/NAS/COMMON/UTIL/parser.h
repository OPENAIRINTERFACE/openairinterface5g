/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
/*****************************************************************************
Source    parser.h

Version   0.1

Date    2012/02/27

Product   NAS stack

Subsystem Utilities

Author    Frederic Maurel

Description Usefull command line parser

*****************************************************************************/
#ifndef __PARSER_H__
#define __PARSER_H__

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/* -----------
 * Option type
 * -----------
 *  An option is defined with a name, an argument following the option's
 *  name, the usage message and a value
 */
typedef struct {
  const char* name;     /* Option name         */
  const char* argument;   /* Argument following the option   */
  const char* usage;      /* Option and Argument usage     */
#define PARSER_OPTION_VALUE_SIZE  32
  char value[PARSER_OPTION_VALUE_SIZE]; /* Option value      */
  char* pvalue;
} parser_option_t;

/* -----------------
 * Command line type
 * -----------------
 *  An command line is defined with a name, the number of options and the
 *  list of command's options
 */
typedef struct {
#define PARSER_COMMAND_NAME_SIZE  32
  char name[PARSER_COMMAND_NAME_SIZE];  /* Command name      */
  const int nb_options;     /* Number of options     */
  parser_option_t options[];      /* Command line options    */
} parser_command_line_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void parser_print_usage(const parser_command_line_t* commamd_line);
int  parser_get_options(int argc, const char** argv,
                        parser_command_line_t* commamd_line);

#endif /* __PARSER_H__*/
