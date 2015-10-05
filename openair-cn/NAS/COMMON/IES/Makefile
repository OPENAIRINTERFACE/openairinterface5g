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
#  OpenAirInterface Dev  : openair4g-devel@eurecom.fr
#
#  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.
#
################################################################################
ifndef PROJDIR
PROJDIR = $(PWD)/../..
endif

include $(PROJDIR)/Makerules
include $(PROJDIR)/Makefile.inc

INCLUDES = -I. -I$(UTILDIR)

#DEFINES	 += -DNAS_DEBUG

TARGET = $(LIBIES)
TARGETS = $(TARGET).a $(TARGET).so

all: $(TARGETS)

%.o: %.c %.h Makefile
	@echo Compiling $<
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).a: $(OBJS)
	@$(RM) $@
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo Replacing $@ to $(LIBDIR)
	@$(RM) $(LIBDIR)/$@
	@$(CP) $@ $(LIBDIR)

$(TARGET).so: $(OBJS)
	@$(LD) -G -o $@ $(OBJS)
	@echo Replacing $@ to $(LIBDIR)
	@$(RM) $(LIBDIR)/$@
	@$(CP) $@ $(LIBDIR)

clean:
	$(RM) $(OBJS) *.bak *~

veryclean: clean
	$(RM) $(TARGETS)

depend:
	makedepend -- ${CFLAGS} -- ${SRCS}

# DO NOT DELETE THIS LINE -- make depend depends on it.

