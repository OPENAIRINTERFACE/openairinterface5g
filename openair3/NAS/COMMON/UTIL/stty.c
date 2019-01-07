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
Source    stty.c

Version   0.1

Date    2012/11/29

Product   NAS stack

Subsystem Utilities

Author    Frederic Maurel

Description Terminal I/O device control setup

*****************************************************************************/

#include "commonDef.h"

#include <termios.h>  // tcgetattr, tcsetattr
#include <unistd.h>
#include <string.h> // strdup, strtok, strcmp
#include <stdlib.h> // free

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* Terminal I/O setting flags */
#define CFLG  0 /* control modes  */
#define IFLG  1 /* input modes    */
#define OFLG  2 /* output modes   */
#define LFLG  3 /* local modes    */
#define RFLG  4 /* control chars  */
#define BFLG  5 /* baud speed modes */

/* Terminal I/O setting parameters */
typedef struct sttyset_s {
  char *name;
  int which;
  int mask;
  int value;
} sttyset_t;

/* Table of terminal I/O parameters */
static sttyset_t stty_params[] = {
  { "0",  BFLG, 0,    B0  },
  { "50", BFLG, 0,    B50 },
  { "75", BFLG, 0,    B75 },
  { "110",  BFLG, 0,    B110  },
  { "134",  BFLG, 0,    B134  },
  { "150",  BFLG, 0,    B150  },
  { "200",  BFLG, 0,    B200  },
  { "300",  BFLG, 0,    B300  },
  { "600",  BFLG, 0,    B600  },
  { "1200", BFLG, 0,    B1200 },
  { "1800", BFLG, 0,    B1800 },
  { "2400", BFLG, 0,    B2400 },
  { "4800", BFLG, 0,    B4800 },
  { "9600", BFLG, 0,    B9600 },
  { "19200",  BFLG, 0,    B19200  },
  { "38400",  BFLG, 0,    B38400  },
#ifdef B57600
  { "57600",  BFLG, 0,    B57600  },
#endif
#ifdef B115200
  { "115200", BFLG, 0,    B115200 },
#endif
#ifdef B230400
  { "230400", BFLG, 0,    B230400 },
#endif
  { "cs7",  CFLG, CSIZE,    CS7 },
  { "cs8",  CFLG, CSIZE,    CS8 },
  { "cstopb", CFLG, CSTOPB,   CSTOPB  },
  { "cread",  CFLG, CREAD,    CREAD },
  { "parenb", CFLG, PARENB,   PARENB  },
  { "parodd", CFLG, PARODD,   PARODD  },
  { "hubcl",  CFLG, HUPCL,    HUPCL },
  { "clocal", CFLG, CLOCAL,   CLOCAL  },
#ifdef CRTSCTS
  { "crtscts",CFLG, CRTSCTS,  CRTSCTS },
#endif
#ifdef ORTSFL
  { "ortsfl", CFLG, ORTSFL,   ORTSFL  },
#endif
#ifdef CTSFLOW
  { "ctsflow",CFLG, CTSFLOW,  CTSFLOW },
#endif
#ifdef RTSFLOW
  { "rtsflow",CFLG, RTSFLOW,  RTSFLOW },
#endif
  { "ignbrk", IFLG, IGNBRK,   IGNBRK  },
  { "brkint", IFLG, BRKINT,   BRKINT  },
  { "ignpar", IFLG, IGNPAR,   IGNPAR  },
  { "parmrk", IFLG, PARMRK,   PARMRK  },
  { "inpck",  IFLG, INPCK,    INPCK },
  { "istrip", IFLG, ISTRIP,   ISTRIP  },
  { "inlcr",  IFLG, INLCR,    INLCR },
  { "igncr",  IFLG, IGNCR,    IGNCR },
  { "icrnl",  IFLG, ICRNL,    ICRNL },
#ifdef IUCLC  // Missing on OSX, FreeBSD
  { "iuclc",  IFLG, IUCLC,    IUCLC },
#endif
  { "ixon", IFLG, IXON,   IXON  },
  { "ixany",  IFLG, IXANY,    IXANY },
  { "ixoff",  IFLG, IXOFF,    IXOFF },
#ifdef IMAXBEL
  { "imaxbel",IFLG, IMAXBEL,  IMAXBEL },
#endif
  { "opost",  OFLG, OPOST,    OPOST },
#ifdef ILCUC  // Missing on OSX, FreeBSD
  { "olcuc",  OFLG, OLCUC,    OLCUC },
#endif
  { "onlcr",  OFLG, ONLCR,    ONLCR },
  { "ocrnl",  OFLG, OCRNL,    OCRNL },
  { "onocr",  OFLG, ONOCR,    ONOCR },
  { "onlret", OFLG, ONLRET,   ONLRET  },
  { "ofil", OFLG, OFILL,    OFILL },
  { "ofdel",  OFLG, OFDEL,    OFDEL },
  { "nl0",  OFLG, NLDLY,    NL0 },
  { "nl1",  OFLG, NLDLY,    NL1 },
  { "cr0",  OFLG, CRDLY,    CR0 },
  { "cr1",  OFLG, CRDLY,    CR1 },
  { "cr2",  OFLG, CRDLY,    CR2 },
  { "cr3",  OFLG, CRDLY,    CR3 },
  { "tab0", OFLG, TABDLY,   TAB0  },
  { "tab1", OFLG, TABDLY,   TAB1  },
  { "tab2", OFLG, TABDLY,   TAB2  },
  { "tab3", OFLG, TABDLY,   TAB3  },
  { "bs0",  OFLG, BSDLY,    BS0 },
  { "bs1",  OFLG, BSDLY,    BS1 },
  { "vt0",  OFLG, VTDLY,    VT0 },
  { "vt1",  OFLG, VTDLY,    VT1 },
  { "ff0",  OFLG, FFDLY,    FF0 },
  { "ff1",  OFLG, FFDLY,    FF1 },
  { "isig", LFLG, ISIG,   ISIG  },
  { "icanon", LFLG, ICANON,   ICANON  },
#ifdef XCASE  // Missing on OSX, FreeBSD
  { "xcase",  LFLG, XCASE,    XCASE },
#endif
  { "echo", LFLG, ECHO,   ECHO  },
  { "echoe",  LFLG, ECHOE,    ECHOE },
  { "echok",  LFLG, ECHOK,    ECHOK },
  { "echonl", LFLG, ECHONL,   ECHONL  },
  { "noflsh", LFLG, NOFLSH,   NOFLSH  },
  { "tostop", LFLG, TOSTOP,   TOSTOP  },
#ifdef ECHOCTL
  { "echoctl",LFLG, ECHOCTL,  ECHOCTL },
#endif
#ifdef ECHOPRT
  { "echoprt",LFLG, ECHOPRT,  ECHOPRT },
#endif
#ifdef ECHOKE
  { "echoke", LFLG, ECHOKE,   ECHOKE  },
#endif
#ifdef FLUSHO
  { "flusho", LFLG, FLUSHO,   FLUSHO  },
#endif
#ifdef PENDIN
  { "pendin", LFLG, PENDIN,   PENDIN  },
#endif
  { "iexten", LFLG, IEXTEN,   IEXTEN  },
#ifdef TOSTOP
  { "tostop", LFLG, TOSTOP,   TOSTOP  },
#endif
  { "raw",  RFLG, 0,    0 },
  { NULL, 0,  0,    0 }
};

static int _stty_set_this(struct termios *term, const sttyset_t *p, int turnon);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  stty_set()                                                **
 **                                                                        **
 ** Description: Set the parameters associated with the terminal referred  **
 **    by the given descriptor                                   **
 **                                                                        **
 ** Inputs:  p:   The TTY parameters                         **
 **    turnon:  Indicate whether the parameter should be   **
 **                             turned on (TRUE) or turned off (FALSE)     **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   term:    The termios structure to fill              **
 **    Return:  RETURNok, RETURNerror                      **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int stty_set(int fd, const char *params)
{
  int rc = RETURNok;

  register char *p;
  register char *s;
  struct termios term;
  register int i;
  int mode;

  /* Get current tty attributes */
  if ( tcgetattr(fd, &term) < 0 ) {
    return RETURNerror;
  }

  s = strdup(params);
  p = strtok(s," \t\n");

  while (p && (rc != RETURNerror)) {
    mode = 1;

    if ( *p == '-' ) {
      mode = 0;
      p++;
    }

    for ( i=0; stty_params[i].name; i++ ) {
      if ( !strcmp(p, stty_params[i].name) ) {
        rc = _stty_set_this(&term, &stty_params[i], mode);
        break;
      }
    }

    p = strtok(NULL," \t\n");
  }

  free(s);

  /* Apply new tty settings immediatly */
  if (rc != RETURNerror) {
    if ( tcsetattr(fd, TCSANOW, &term) < 0 ) {
      rc = RETURNerror;
    }
  }

  return (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  _stty_set_this()                                          **
 **                                                                        **
 ** Description: Fills the termios structure with the given TTY parameters **
 **                                                                        **
 ** Inputs:  p:   The TTY parameters                         **
 **    turnon:  Indicate whether the parameter should be   **
 **                             turned on (TRUE) or turned off (FALSE)     **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   term:    The termios structure to fill              **
 **    Return:  RETURNok, RETURNerror                      **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _stty_set_this(struct termios *term, const sttyset_t *p, int turnon)
{
  int rc = RETURNok;

  switch ( p->which ) {
  case CFLG:
    term->c_cflag &= ~(p->mask);

    if ( turnon )
      term->c_cflag |= p->value;

    break;

  case IFLG:
    term->c_iflag &= ~(p->mask);

    if ( turnon )
      term->c_iflag |= p->value;

    break;

  case OFLG:
    term->c_oflag &= ~(p->mask);

    if ( turnon )
      term->c_oflag |= p->value;

    break;

  case LFLG:
    term->c_lflag &= ~(p->mask);

    if ( turnon )
      term->c_lflag |= p->value;

    break;

  case RFLG:
    term->c_iflag = 0;
    term->c_oflag = 0;
    term->c_lflag = 0;
    term->c_cc[VMIN] = 1;
    term->c_cc[VTIME] = 0;
    break;

  case BFLG:
    if (cfsetispeed(term, p->value) < 0) {
      rc = RETURNerror;
    } else if (cfsetospeed(term, p->value) < 0) {
      rc = RETURNerror;
    }

    break;

  default:
    rc = RETURNerror;
  }

  return (rc);
}

