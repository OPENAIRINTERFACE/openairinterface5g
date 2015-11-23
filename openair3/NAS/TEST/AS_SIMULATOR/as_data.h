/*****************************************************************************
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    as_data.h

Version   0.1

Date    2013/04/11

Product   Access-Stratum sublayer simulator

Subsystem Access-Stratum data

Author    Frederic Maurel

Description Defines constants and functions used by the AS simulator
    process.

*****************************************************************************/

#ifndef __AS_DATA_H__
#define __AS_DATA_H__

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * Constansts used for MSCGEN (Message Sequence Chart Generator)
 */
#define MSCGEN(args...) fprintf(stderr, ##args);
#define _ue_id    "UE"
#define _mme_id   "MME"
#define _as_id    "AS"

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

const char* getTime(void);

const char* rrcCause(char cause);
const char* rrcType(char type);
const char* rrcErrCode(char code);
const char* rrcReleaseCause(char cause);

#endif // __AS_DATA_H__
