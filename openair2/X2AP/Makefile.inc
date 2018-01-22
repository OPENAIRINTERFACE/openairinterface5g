
ASN1MESSAGESDIR=$(X2AP_DIR)/MESSAGES
ASN1DIR=$(ASN1MESSAGESDIR)/ASN1

#//ifeq ($(USE_MME), R10)
#//	ASN1RELDIR=R10.7
#//else
#//ASN1RELDIR=R11.2
#//endif

# TEMP
ASN1RELDIR=R11.2


include $(ASN1DIR)/$(ASN1RELDIR)/Makefile.inc

libx2ap_OBJECTS = \
	x2ap.o        \
	x2ap_common.o \
	$(addprefix MESSAGES/ASN1/$(ASN1RELDIR)/, $(X2AP_ASN_MODULE_SOURCES))



# pull in dependency info for *existing* .o files
-include $(OUTDIR)/*.d

CFLAGS =            \
	-Wall           \
	-DENB_MODE      \
	-DENABLE_USE_MME    \
	-I.        \
	-I$(ASN1MESSAGESDIR)/ASN1/$(ASN1RELDIR) \
	-I$(ASN1MESSAGESDIR) \
	-I$(OUTDIR)     \
	-I../UTILS      \
	$(ADD_CFLAGS)       \
	$(X2AP_CFLAGS)      \
	-Wuninitialized     \
	-Werror=implicit-function-declaration

X2AP_GENERATED =    \
	$(OUTDIR)/x2ap_encoder.o    \
	$(OUTDIR)/x2ap_decoder.o    \
	$(OUTDIR)/x2ap_xer_print.o

$(OUTDIR)/%.o : %.c
	@echo "Compiling $<"
	@echo "Compiling $<  with CFLAGS= $(CFLAGS)"
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi;
	@$(CC) -c $(CFLAGS) -o $@ $<
	@$(CC) -MM $(CFLAGS) $< > $(basename $@).d
	@mv -f $(basename $@).d $(basename $@).d.tmp
	@sed -e 's|.*:|$@:|' < $(basename $@).d.tmp > $(basename $@).d
	@sed -e 's/.*://' -e 's/\\$$//' < $(basename $@).d.tmp | fmt -1 | \
	sed -e 's/^ *//' -e 's/$$/:/' >> $(basename $@).d
	@rm -f $(basename $@).d.tmp

$(X2AP_GENERATED): %.o : %.c
	@echo "Compiling $<"
	@echo "Compiling $<  with CFLAGS= $(CFLAGS)"
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi;
	@$(CC) -c $(CFLAGS) -o $@ $<
	@$(CC) -MM $(CFLAGS) $< > $(basename $@).d
	@mv -f $(basename $@).d $(basename $@).d.tmp
	@sed -e 's|.*:|$@:|' < $(basename $@).d.tmp > $(basename $@).d
	@sed -e 's/.*://' -e 's/\\$$//' < $(basename $@).d.tmp | fmt -1 | \
	sed -e 's/^ *//' -e 's/$$/:/' >> $(basename $@).d
	@rm -f $(basename $@).d.tmp


$(OUTDIR)/x2ap_ieregen.stamp: $(ASN1DIR)/$(ASN1RELDIR)/X2AP-PDU-Contents.asn $(ASN1DIR)/asn1tostruct.py
	@if [ ! -d $(OUTDIR) ]; then mkdir -p $(OUTDIR); fi;
	@python $(ASN1DIR)/asn1tostruct.py -f$< -o$(OUTDIR) 
	@echo Timestamp > $@

#ProtocolExtensionContainer
#$(ASN1DIR)/$(ASN1RELDIR)/X2AP-PDU.asn 

$(OUTDIR)/x2ap_asn1regen.stamp:  $(ASN1DIR)/$(ASN1RELDIR)/X2AP-CommonDataTypes.asn  $(ASN1DIR)/$(ASN1RELDIR)/X2AP-Constants.asn $(ASN1DIR)/$(ASN1RELDIR)/X2AP-IEs.asn  $(ASN1DIR)/$(ASN1RELDIR)/X2AP-PDU.asn   
	@echo "Timestamp DIR " $(ASN1DIR) " DIRREL " $(ASN1RELDIR)
#	(cd $(ASN1DIR)/$(ASN1RELDIR) && asn1c -fhave_native64 -gen-PER -fcompound-names $^)
	(cd $(ASN1MESSAGESDIR) && asn1c -fhave_native64 -gen-PER -fcompound-names -fskeletons-copy $^)
	@echo "Timestamp X2AP_CFLAGS: " $(X2AP_CFLAGS)
	@echo Timestamp > $@	

$(OUTDIR)/libx2ap.a: $(OUTDIR)/x2ap_ieregen.stamp $(OUTDIR)/x2ap_asn1regen.stamp $(X2AP_GENERATED) $(addprefix $(OUTDIR)/,$(libx2ap_OBJECTS))
	@echo Creating X2AP archive
	@echo libx2ap_OBJECTS $(libx2ap_OBJECTS)
	@$(AR) rcs $@ $(X2AP_GENERATED) $(addprefix $(OUTDIR)/,$(libx2ap_OBJECTS))

clean:
	@$(RM_F_V) $(OUTDIR)/*.o
	@$(RM_F_V) $(OUTDIR)/*.d
	@$(RM_F_V) $(addprefix $(OUTDIR)/MESSAGES/ASN1/$(ASN1RELDIR), $(X2AP_ASN_MODULE_SOURCES))
	@$(RM_F_V) $(addprefix $(OUTDIR)/MESSAGES/ASN1/$(ASN1RELDIR), $(X2AP_ASN_MODULE_SOURCES:.o=.d))
	@$(RM_F_V) $(OUTDIR)/libx2ap.a
	@$(RM_F_V) $(OUTDIR)/x2ap_asn1regen.stamp
	@$(RM_F_V) $(OUTDIR)/x2ap_ieregen.stamp
	@$(RM_F_V) $(OUTDIR)/x2ap_decoder.c $(OUTDIR)/x2ap_encoder.c
	@$(RM_F_V) $(OUTDIR)/x2ap_xer_print.c $(OUTDIR)/x2ap_ies_defs.h

cleanall: clean
	@$(RM_F_V) $(addprefix $(OUTDIR)/MESSAGES/ASN1/$(ASN1RELDIR), $(X2AP_ASN_MODULE_SOURCES:.o=.c))
	@$(RM_F_V) $(addprefix $(OUTDIR)/MESSAGES/ASN1/$(ASN1RELDIR), $(X2AP_ASN_MODULE_SOURCES:.o=.h))

showcflags:
	@echo x2ap cflags: $(CFLAGS)
