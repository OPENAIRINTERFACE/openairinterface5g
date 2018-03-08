#ifndef OPENAIRINTERFACE5G_LIMITS_H_
#define OPENAIRINTERFACE5G_LIMITS_H_

#if defined(CBMIMO1) || defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_LMSSDR)
#        define NUMBER_OF_eNB_MAX 1
#        define NUMBER_OF_RU_MAX 2
#        define NUMBER_OF_UE_MAX 16
#        define NUMBER_OF_CONNECTED_eNB_MAX 3
#else
#        define NUMBER_OF_eNB_MAX 7
#        define NUMBER_OF_RU_MAX 32
#        define NUMBER_OF_UE_MAX 20
#        define NUMBER_OF_CONNECTED_eNB_MAX 3
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
