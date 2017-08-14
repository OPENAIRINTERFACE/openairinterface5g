ENB_APP_DIR = $(OPENAIR2_TOP)/ENB_APP

ENB_APP_OBJS =  $(ENB_APP_DIR)/enb_app.o \
                $(ENB_APP_DIR)/enb_config.o 
ENB_APP_incl = \
    -I$(ENB_APP_DIR) -I$(OPENAIR2_TOP)
    
