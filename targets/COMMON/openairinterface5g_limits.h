#ifndef OPENAIRINTERFACE5G_LIMITS_H_
#define OPENAIRINTERFACE5G_LIMITS_H_

#if defined(CBMIMO1) || defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_LMSSDR)
#        define NUMBER_OF_eNB_MAX 1
#        define NUMBER_OF_RU_MAX 2
#        ifndef UE_EXPANSION
// TODO:L2 FAPI simulator.
// UESIM_EXPANSION is used to be same value of NUMBER_OF_UE_MAX
// in eNB and UE.
// now , if we use --mu option in UE, compiling error will occur.
// This problem will be fixed in the future.
#            ifndef UESIM_EXPANSION
#                define NUMBER_OF_UE_MAX 16
#                define NUMBER_OF_UCI_VARS_MAX 56
#                define NUMBER_OF_CONNECTED_eNB_MAX 3
#            else
#                define NUMBER_OF_UE_MAX 256
#                define NUMBER_OF_UCI_VARS_MAX 256
#                define NUMBER_OF_CONNECTED_eNB_MAX 1
#            endif
#        else
#                define NUMBER_OF_UE_MAX 256
#                define NUMBER_OF_UCI_VARS_MAX 256
#                define NUMBER_OF_CONNECTED_eNB_MAX 1
#        endif
#else
#        define NUMBER_OF_eNB_MAX 7
#        define NUMBER_OF_RU_MAX 32
#        ifndef UE_EXPANSION
/* if the value of MAX_MOBILES_PER_ENB and NUMBER_OF_UE_MAX is different,
eNB process will exit because unexpected access happens.
Now some parts are using NUMBER_OF_UE_MAX
and the other are using MAX_MOBILES_PER_ENB in for-loop.
*/
#            ifndef UESIM_EXPANSION
#                define NUMBER_OF_UE_MAX 16
#                define NUMBER_OF_UCI_VARS_MAX 56
#                define NUMBER_OF_CONNECTED_eNB_MAX 3
#            else
#                define NUMBER_OF_UE_MAX 256
#                define NUMBER_OF_UCI_VARS_MAX 256
#                define NUMBER_OF_CONNECTED_eNB_MAX 1
#            endif
#        else
#                define NUMBER_OF_UE_MAX 256
#                define NUMBER_OF_UCI_VARS_MAX 256
#                define NUMBER_OF_CONNECTED_eNB_MAX 1
#        endif
#        if defined(STANDALONE) && STANDALONE==1
#                undef  NUMBER_OF_eNB_MAX
#                undef  NUMBER_OF_UE_MAX
#                undef  NUMBER_OF_RU_MAX
#                define NUMBER_OF_eNB_MAX 3
#                define NUMBER_OF_UE_MAX 3
#                define NUMBER_OF_RU_MAX 3
#        endif
#        if defined(LARGE_SCALE) && LARGE_SCALE
#                undef  NUMBER_OF_eNB_MAX
#                undef  NUMBER_OF_UE_MAX
#                undef  NUMBER_OF_CONNECTED_eNB_MAX
#                undef  NUMBER_OF_RU_MAX
#                define NUMBER_OF_eNB_MAX 2
#                define NUMBER_OF_UE_MAX 120
#                define NUMBER_OF_RU_MAX 16
#                define NUMBER_OF_CONNECTED_eNB_MAX 1 // to save some memory
#        endif
#endif

#endif /* OPENAIRINTERFACE5G_LIMITS_H_ */
