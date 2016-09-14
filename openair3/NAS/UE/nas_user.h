/*****************************************************************************
Source      nas_user.h

Version     0.1

Date        2012/03/09

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Maurel

Description NAS procedure functions triggered by the user

*****************************************************************************/
#ifndef __NAS_USER_H__
#define __NAS_USER_H__

#include "commonDef.h"
#include "networkDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void nas_user_initialize(emm_indication_callback_t emm_cb,
                         esm_indication_callback_t esm_cb, const char *version);

int nas_user_receive_and_process(int * fd, char *message);

int nas_user_process_data(const void *data);

const void *nas_user_get_data(void);

#endif /* __NAS_USER_H__*/
