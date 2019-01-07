#ifndef _ESM_PT_DEFS_H
#define _ESM_PT_DEFS_H

#include "UTIL/nas_timer.h"
#include "IES/ProcedureTransactionIdentity.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * Minimal and maximal value of a procedure transaction identity:
 * The Procedure Transaction Identity (PTI) identifies bi-directional
 * messages flows
 */
#define ESM_PTI_MIN     (PROCEDURE_TRANSACTION_IDENTITY_FIRST)
#define ESM_PTI_MAX     (PROCEDURE_TRANSACTION_IDENTITY_LAST)

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/* Procedure transaction states */
typedef enum {
  ESM_PT_INACTIVE,    /* No procedure transaction exists      */
  ESM_PT_PENDING, /* The UE has initiated a procedure transaction
             * towards the network              */
  ESM_PT_STATE_MAX
} esm_pt_state;

/* ESM message timer retransmission data */
typedef struct {
  unsigned char pti;      /* Procedure transaction identity   */
  unsigned int count;     /* Retransmission counter       */
  OctetString msg;        /* Encoded ESM message to re-transmit   */
  void *user;             /* user reference - void to avoid cyclic dependency */
} esm_pt_timer_data_t;

/*
 * --------------------------
 * Procedure transaction data
 * --------------------------
 */
typedef struct {
  unsigned char pti;      /* Procedure transaction identity   */
  esm_pt_state status;    /* Procedure transaction status     */
  struct nas_timer_t timer;   /* Retransmission timer         */
  esm_pt_timer_data_t *args;  /* Retransmission timer parameters data */
} esm_pt_context_t;

/*
 * ------------------------------
 * List of procedure transactions
 * ------------------------------
 */
typedef struct {
  unsigned char index;    /* Index of the next procedure transaction
                 * identity to be used */
#define ESM_PT_DATA_SIZE (ESM_PTI_MAX - ESM_PTI_MIN + 1)
  esm_pt_context_t *context[ESM_PT_DATA_SIZE + 1];
} esm_pt_data_t;

#endif
