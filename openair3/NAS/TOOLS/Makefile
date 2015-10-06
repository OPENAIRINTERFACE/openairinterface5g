################################################################################
#   OpenAirInterface
#   Copyright(c) 1999 - 2014 Eurecom
#
#    OpenAirInterface is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#
#    OpenAirInterface is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with OpenAirInterface.The full GNU General Public License is
#    included in this distribution in the file called "COPYING". If not,
#    see <http://www.gnu.org/licenses/>.
#
#  Contact Information
#  OpenAirInterface Admin: openair_admin@eurecom.fr
#  OpenAirInterface Tech : openair_tech@eurecom.fr
#  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr
#
#  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.
#
################################################################################
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

