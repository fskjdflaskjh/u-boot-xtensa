/*
 * (C) Copyright 2008 Tensilica Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Exception handling.
 *  We currently don't handle any exception and force a reset.
 *  (Note that alloca is a special case and handled in start.S)
 */

#include <common.h>
#include <command.h>
#include <asm/string.h>
#include <asm/regs.h>

void unhandled_exception(struct pt_regs*);

typedef void (*handler_t)(struct pt_regs*);

handler_t exc_table[EXCCAUSE_LAST] = { 
	[0 ... EXCCAUSE_LAST-1] unhandled_exception, 
};

void unhandled_exception(struct pt_regs *regs)
{
	printf("Unhandled Exception: EXCCAUSE = %d\n", regs->exccause);
	panic("*** PANIC\n");
}

#ifdef CONFIG_USE_IRQ
#error "Use of interrupts is not supported in Avnet/Xtensa ports"
#else
void enable_interrupts (void)
{
	return;
}
int disable_interrupts (void)
{
	return 0;
}
#endif

