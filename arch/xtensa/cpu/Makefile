#
# (C) Copyright 2007 - 2013 Tensilica, Inc.
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

obj-y = cpu.o exceptions.o
extra-y = start.o u-boot.lds

$(obj)u-boot.lds: u-boot.lds.S ../include/asm/ldscript.h
	$(CPP) -P -C $(CPPFLAGS) -U$(ARCH) $< >$@
$(obj).depend.%:	%.lds.S
	$(CC) -M $(CPPFLAGS) $(EXTRA_CPPFLAGS_DEP) \
	-MQ $(obj)$(basename $<) $< >$@

