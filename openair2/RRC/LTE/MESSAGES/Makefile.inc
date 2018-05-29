
current_release_asn1_module=$(strip $(notdir $(wildcard $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/.lock-*)))

ifdef CBA
    COMMON_CFLAGS += -DRel10
    COMMON_CFLAGS += -DCBA
    ifneq ($(current_release_asn1_module), .lock-rel10-cba)
        tmp3:=$(shell cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ;\
                rm -f ${ASN_MODULE_SOURCES} $(ASN_MODULE_HEADERS) *.o *.d ;\
                cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/ ;\
                rm EUTRA-RRC-Definitions.asn ;\
                ln -s EUTRA-RRC-Definitions-a20-lola.asn EUTRA-RRC-Definitions.asn ; \
                cd $(OPENAIR_TARGETS)/SIMU/USER/ ;\
                rm -f $(ASN1_MSG_OBJS1) ;\
                rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated ;\
                rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.am.sample ;\
                rm -f $(L2_OBJS) ;\
                rm -f *.o ;\
                rm -f oaisim ;\
                rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/.lock-* ;\
                touch $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/.lock-rel10-cba ;\
                /bin/bash $(OPENAIR_TARGETS)/SCRIPTS/asn1_generate_rrc_messaging_c_code.bash ;\
                cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ;\
                awk '/converter-sample.c/ {exit} // {print}' Makefile.am.sample > Makefile.inc.generated  )
    endif

else ifdef Rel10
   COMMON_CFLAGS += -DRel10
   ifneq  ($(current_release_asn1_module), .lock-rel10)
      tmp3:=$(shell cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ;\
              rm -f ${ASN_MODULE_SOURCES} $(ASN_MODULE_HEADERS) *.o *.d ;\
              cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/ ;\
              rm EUTRA-RRC-Definitions.asn ;\
              ln -s EUTRA-RRC-Definitions-a20.asn EUTRA-RRC-Definitions.asn ;\
              cd $(OPENAIR_TARGETS)/SIMU/USER/ ;\
              rm -f $(ASN1_MSG_OBJS1) ;\
              rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated ;\
              rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.am.sample ;\
              rm -f $(L2_OBJS) ;\
              rm -f *.o ;\
              rm -f oaisim ;\
              rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/.lock-* ;\
              touch $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/.lock-rel10 ;\
              /bin/bash $(OPENAIR_TARGETS)/SCRIPTS/asn1_generate_rrc_messaging_c_code.bash ;\
              cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ;\
              awk '/converter-sample.c/ {exit} // {print}' Makefile.am.sample > Makefile.inc.generated  )
   endif

else # default is rel 8
   COMMON_CFLAGS += -DRel8
   ifneq  ($(current_release_asn1_module), .lock-rel8)
       tmp3:=$(shell cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ;\
               rm -f ${ASN_MODULE_SOURCES} $(ASN_MODULE_HEADERS) *.o *.d ;\
               cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/ ;\
               rm EUTRA-RRC-Definitions.asn ;\
               ln -s EUTRA-RRC-Definitions-86.asn EUTRA-RRC-Definitions.asn ;\
               cd $(OPENAIR_TARGETS)/SIMU/USER/ ;\
               rm -f $(ASN1_MSG_OBJS1) ;\
               rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated ;\
               rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.am.sample ;\
               rm -f $(L2_OBJS);\
               rm -f *.o ;\
               rm -f oaisim ;\
               rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/.lock-* ;\
               touch $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/.lock-rel8 ;\
               /bin/bash $(OPENAIR_TARGETS)/SCRIPTS/asn1_generate_rrc_messaging_c_code.bash ;\
               cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ;\
               awk '/converter-sample.c/ {exit} // {print}' Makefile.am.sample > Makefile.inc.generated  )
    endif

endif

tmp:=$(shell cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ;\
       if [ ! -s Makefile.am.sample ] ; then \
           /bin/bash $(OPENAIR_TARGETS)/SCRIPTS/asn1_generate_rrc_messaging_c_code.bash ;\
       fi ;\
       if [ ! -s Makefile.inc.generated ] ; then  \
           awk '/converter-sample.c/ {exit} // {print}' Makefile.am.sample > Makefile.inc.generated ;\
       fi )

include $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated

ASN_CONVERTER_SOURCES+=asn1_msg.c
 
#COMMON_CFLAGS += -DEMIT_ASN_DEBUG=1

EXTRA_CFLAGS += -I. -I$(OPENAIR2_DIR)/RRC/LITE/MESSAGES 

ASN1_MODULE_OBJS=${ASN_MODULE_SOURCES:.c=.o}
ASN1_CONVERTER_OBJS=${ASN_CONVERTER_SOURCES:.c=.o} 
ASN1_MSG_OBJS=$(ASN1_MODULE_OBJS) $(ASN1_CONVERTER_OBJS)
