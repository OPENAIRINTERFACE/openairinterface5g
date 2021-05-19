#ifndef OPENAIRINTERFACE5G_LIMITS_H_
#define OPENAIRINTERFACE5G_LIMITS_H_

#if 1 /*defined(CBMIMO1) || defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)*/
#        define NUMBER_OF_eNB_MAX 1
#        define NUMBER_OF_gNB_MAX 1
#        define NUMBER_OF_RU_MAX 2
#        define NUMBER_OF_NR_RU_MAX 2
#        define NUMBER_OF_UCI_MAX 16
#        define NUMBER_OF_ULSCH_MAX 8
#        define NUMBER_OF_DLSCH_MAX 8 
#        define NUMBER_OF_SRS_MAX 16
#        define NUMBER_OF_NR_ULSCH_MAX 8
#        define NUMBER_OF_NR_DLSCH_MAX 8
#        define NUMBER_OF_NR_UCI_MAX 16
#        define nUMBER_OF_NR_SRS_MAX 16
#        define NUMBER_OF_SCH_STATS_MAX 16

#        define NUMBER_OF_NR_SCH_STATS_MAX 16

#        define NUMBER_OF_NR_PUCCH_MAX 16
#        define NUMBER_OF_NR_SR_MAX 16
#        define NUMBER_OF_NR_PDCCH_MAX 16

#define MAX_MANAGED_ENB_PER_MOBILE  2
#define MAX_MANAGED_GNB_PER_MOBILE  2

#        ifndef PHYSIM
#            ifndef UE_EXPANSION
#                    define NUMBER_OF_UE_MAX 4
#                    define NUMBER_OF_NR_UE_MAX 4
#                    define NUMBER_OF_CONNECTED_eNB_MAX 1
#                    define NUMBER_OF_CONNECTED_gNB_MAX 1
#            else
#                    define NUMBER_OF_UE_MAX 256
#                    define NUMBER_OF_CONNECTED_eNB_MAX 1
#                    define NUMBER_OF_CONNECTED_gNB_MAX 1
#            endif
#        else
#                    define NUMBER_OF_UE_MAX 4
#                    define NUMBER_OF_NR_UE_MAX 4
#                    define NUMBER_OF_CONNECTED_eNB_MAX 1
#                    define NUMBER_OF_CONNECTED_gNB_MAX 1
#        endif
#else
#        define NUMBER_OF_eNB_MAX 7
#        define NUMBER_OF_gNB_MAX 7
#        define NUMBER_OF_RU_MAX 32
#        define NUMBER_OF_NR_RU_MAX 32
#        ifndef UE_EXPANSION
/* if the value of MAX_MOBILES_PER_ENB and NUMBER_OF_UE_MAX is different,
eNB process will exit because unexpected access happens.
Now some parts are using NUMBER_OF_UE_MAX
and the other are using MAX_MOBILES_PER_ENB in for-loop.
*/
#                define NUMBER_OF_UE_MAX 16
#                define NUMBER_OF_UCI_VARS_MAX 56
#                define NUMBER_OF_CONNECTED_eNB_MAX 3
#                define NUMBER_OF_CONNECTED_gNB_MAX 3
#        else
#                define NUMBER_OF_UE_MAX 256
#                define NUMBER_OF_UCI_VARS_MAX 256
#                define NUMBER_OF_CONNECTED_eNB_MAX 1
#                define NUMBER_OF_CONNECTED_gNB_MAX 1
#        endif
#        if defined(STANDALONE) && STANDALONE==1
#            undef  NUMBER_OF_eNB_MAX
#            undef  NUMBER_OF_gNB_MAX

#            undef  NUMBER_OF_UE_MAX

#            undef  NUMBER_OF_RU_MAX
#            undef  NUMBER_OF_NR_RU_MAX

#            define NUMBER_OF_eNB_MAX 3
#            define NUMBER_OF_gNB_MAX 3

#            define NUMBER_OF_UE_MAX 3

#            define NUMBER_OF_RU_MAX 3
#            define NUMBER_OF_NR_RU_MAX 3
#        endif
#        if defined(LARGE_SCALE) && LARGE_SCALE
#            undef  NUMBER_OF_eNB_MAX
#            undef  NUMBER_OF_gNB_MAX

#            undef  NUMBER_OF_UE_MAX

#            undef  NUMBER_OF_CONNECTED_eNB_MAX
#            undef  NUMBER_OF_CONNECTED_gNB_MAX

#            undef  NUMBER_OF_RU_MAX
#            undef  NUMBER_OF_NR_RU_MAX

#            define NUMBER_OF_eNB_MAX 2
#            define NUMBER_OF_gNB_MAX 2

#            define NUMBER_OF_UE_MAX 120

#            define NUMBER_OF_RU_MAX 16
#            define NUMBER_OF_NR_RU_MAX 16

#            define NUMBER_OF_CONNECTED_eNB_MAX 1 // to save some memory
#            define NUMBER_OF_CONNECTED_gNB_MAX 1
#        endif
#endif

#endif /* OPENAIRINTERFACE5G_LIMITS_H_ */
