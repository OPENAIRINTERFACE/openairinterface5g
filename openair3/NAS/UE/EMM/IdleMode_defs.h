#ifndef _IDLEMODE_DEFS_H
#define _IDLEMODE_DEFS_H

/*
 * A list of PLMN identities in priority order is maintained locally
 * to perform the PLMN selection procedure.
 *
 * In automatic mode of operation, this list is used for PLMN selection when
 * the UE is switched on, or upon recovery from lack of coverage, or when the
 * user requests the UE to initiate PLMN reselection, and registration.
 * In manual mode of operation, this list is displayed to the user that may
 * select an available PLMN and initiate registration.
 *
 * The list may contain PLMN identifiers in the following order:
 * - The last registered PLMN or each equivalent PLMN present in the list of
 *   "equivalent PLMNs" (EPLMN_MAX), when UE is switched on or following
 *   recovery from lack of coverage;
 * - The highest priority PLMN in the list of "equivalent HPLMNs" or the
 *   HPLMN derived from the IMSI (1)
 * - Each PLMN/access technology combination in the "User Controlled PLMN
 *   Selector with Access Technology" (PLMN_MAX)
 * - Each PLMN/access technology combination in the "Operator Controlled PLMN
 *   Selector with Access Technology" (OPLMN_MAX)
 * - Other PLMN/access technology combinations with received high quality
 *   signal in random order (TODO)
 * - Other PLMN/access technology combinations in order of decreasing signal
 *   quality (TODO)
 * - The last selected PLMN again (1)
 */
typedef struct {
  int n_plmns;
#define EMM_PLMN_LIST_SIZE (EMM_DATA_EPLMN_MAX + EMM_DATA_PLMN_MAX +    \
                            EMM_DATA_OPLMN_MAX + 2)
  plmn_t *plmn[EMM_PLMN_LIST_SIZE];
  int index;    /* Index of the PLMN for which selection is ongoing        */
  int hplmn;    /* Index of the home PLMN or the highest priority
           * equivalent home PLMN                    */
  int fplmn;    /* Index of the first forbidden PLMN               */
  int splmn;    /* Index of the currently selected PLMN            */
  int rplmn;    /* Index of the currently registered PLMN          */
  struct plmn_param_t {
    char fullname[NET_FORMAT_LONG_SIZE+1];   /* PLMN full identifier     */
    char shortname[NET_FORMAT_SHORT_SIZE+1]; /* PLMN short identifier    */
    char num[NET_FORMAT_NUM_SIZE+1];     /* PLMN numeric identifier  */
    int stat; /* Indication of the PLMN availability             */
    int tac;  /* Location/Tracking Area Code                 */
    int ci;   /* Serving cell identifier                     */
    int rat;  /* Radio Access Technology supported by the serving cell   */
  } param[EMM_PLMN_LIST_SIZE];
} emm_plmn_list_t;

#endif
