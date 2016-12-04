#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.0  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */

export PROCESS = UE

ifndef PROJDIR
PROJDIR = $(PWD)/..
endif

include $(PROJDIR)/Makerules
include $(PROJDIR)/Makefile.inc
include $(PROJDIR)/../Makefile.tools

export LD_RUN_PATH = $(LIBDIR):$(LIBPROCESS)

LIBS		= -luenas.a -lrt 
INCLUDES	= -I. -I$(INCDIR) -I$(UTILDIR) -I$(USIMAPIDIR) -I$(EMMDIR) -I$(ESMDIR) -I$(IESDIR)

#LIBSUTIL	= $(LIBDIR)/$(LIBUTIL).a $(LIBDIR)/$(LIBUTIL).so

USIM_OBJ	= usim_data.o
UE_OBJ		= ue_data.o

USIM_TARGET	= usim_data
UE_TARGET	= ue_data

TARGETS = $(USIM_TARGET) $(UE_TARGET)

all: $(TARGETS)

#-DIMSI_USA_MNC_3DIGITS
%.o: %.c Makefile
	$(CC) $(CFLAGS)   -c $< -o $@

$(USIM_TARGET): $(USIM_OBJ) $(LIBSUTIL)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)  -lnettle -lcrypto -lm
	@echo Replacing $@ to $(BINDIR)
	@$(RM) $(BINDIR)/$@
	@$(CP) $@ $(BINDIR)

$(UE_TARGET): $(UE_OBJ) $(LIBSUTIL)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)  -lnettle -lcrypto -lm
	@echo Replacing $@ to $(BINDIR)
	@$(RM) $(BINDIR)/$@
	@$(CP) $@ $(BINDIR)

clean:
	$(RM) $(OBJS) *.bak *~

veryclean: clean
	$(RM) $(TARGETS)

veryveryclean: veryclean
	$(RM) -Rf *.o $(PROJDIR)
	$(RM) -Rf *.a $(PROJDIR)

depend:
	makedepend -- ${CFLAGS} -- ${SRCS}

# DO NOT DELETE THIS LINE -- make depend depends on it.

