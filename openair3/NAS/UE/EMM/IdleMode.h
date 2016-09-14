/*****************************************************************************
Source      IdleMode.h

Version     0.1

Date        2012/10/23

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the functions used to get information from the list
        of available PLMNs locally maintained when the UE is in
        idle mode.

*****************************************************************************/
#ifndef __IDLEMODE_H__
#define __IDLEMODE_H__

#include "commonDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

typedef int (*IdleMode_callback_t) (int);

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void IdleMode_initialize(IdleMode_callback_t cb);

int IdleMode_get_nb_plmns(void);
int IdleMode_get_hplmn_index(void);
int IdleMode_get_rplmn_index(void);
int IdleMode_get_splmn_index(void);

int IdleMode_update_plmn_list(int index);

const char *IdleMode_get_plmn_fullname(const plmn_t *plmn, int index,
                                       size_t *len);
const char *IdleMode_get_plmn_shortname(const plmn_t *plmn, int index,
                                        size_t *len);
const char *IdleMode_get_plmn_id(const plmn_t *plmn, int index, size_t *len);

int IdleMode_get_plmn_fullname_index(const char *plmn);
int IdleMode_get_plmn_shortname_index(const char *plmn);
int IdleMode_get_plmn_id_index(const char *plmn);

#endif /* __IDLEMODE_H__*/
