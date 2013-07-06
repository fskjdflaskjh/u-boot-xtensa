#
# (C) Copyright 2007 - 2014 Tensilica, Inc.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

CROSS_COMPILE ?= xtensa-linux-
PLATFORM_CPPFLAGS += -DCONFIG_XTENSA -D__XTENSA__

LOAD_ADDR_SRC := "\#include <asm/addrspace.h>\n"${CONFIG_STANDALONE_LOAD_ADDR}
LOAD_ADDR = $(shell echo -e ${LOAD_ADDR_SRC} | $(CC) $(CFLAGS) -E -P - )
CONFIG_STANDALONE_LOAD_ADDR = $(shell printf "0x%08x" `expr $$[$(LOAD_ADDR)]`)

__HAVE_ARCH_GENERIC_BOARD := y

