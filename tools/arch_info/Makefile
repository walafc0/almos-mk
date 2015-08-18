#
# This file is part of AlmOS.
#
# AlmOS is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# AlmOS is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with AlmOS; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#
# UPMC / LIP6 / SOC (c) 2009
# Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
#

#=====================================================================
# AlmOS Boot Information Block Tools (bib2info & info2bib)
#=====================================================================

CC=gcc
CFLAGS=-W -Wall -Werror
BIN=$(ALMOS_TOP)/tools/bin
RM=rm -f
MK=mkdir -p

BINS=$(BIN)/bib2info $(BIN)/info2bib

all: $(BIN) $(BINS)

$(BIN):
	@echo '   [ MKDIR ]       '$@
	@$(MK) $@

$(BIN)/bib2info: bib2info.c devdb.c
	@echo '   [  CC  ]        '$^
	@$(CC) $(CFLAGS) $^ -o $@

$(BIN)/info2bib : info2bib.c devdb.c
	@echo '   [  CC  ]        '$^
	@$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean realclean

clean:
	@echo '   [  RM  ]        '*~
	@$(RM) *~

realclean: clean
	@echo '   [  RM  ]        '$(BINS)
	@$(RM) $(BINS)
	@echo '   [  RM  ]        '$(BIN)
	$(RM) -r $(BIN)

#=====================================================================
