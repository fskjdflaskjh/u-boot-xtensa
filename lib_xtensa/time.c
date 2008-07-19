/*
 * (C) Copyright 2008, Tensilica Inc.
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

#include <common.h>
#include <asm/stringify.h>


#if XCHAL_HAVE_CCOUNT
static ulong get_ccount(void)
{
	ulong ccount;
	asm volatile ("rsr %0,"__stringify(CCOUNT) : "=a" (ccount));
	return ccount;
}
#endif

/*
 * Delay (busy-wait) for a number of microseconds.
 */

void udelay(unsigned long usec)
{
	ulong start, timer;

#if !XCHAL_HAVE_CCOUNT
	/*
	 * Configs without timer option have to fake a timer for get_timer().
	 * Timing will not be accurate, but hopefully nothing will hang.
	 */
#warning "Without Xtensa timer option, timing will not be accurate."
#endif

	start = get_ccount();
	timer = (usec * (CONFIG_SYS_CLK_FREQ / 1000)) / 1000;
	while (get_ccount() - start < timer)
		;
}


/*
 * Return the elapsed time (ticks) since 'base'.
 */

ulong get_timer(ulong base)
{
	/* Don't tie up a timer; use cycle counter if available (or fake it). */
#if XCHAL_HAVE_CCOUNT
	register ulong ccount;
	__asm__ volatile ("rsr %0, CCOUNT" : "=a"(ccount));
	return ccount / (CONFIG_SYS_CLK_FREQ / CFG_HZ) - base;
#else
	/*
	 * Add at least the overhead of this call (in cycles).
	 * Avoids hanging in case caller doesn't use udelay().
	 */
	fake_ccount += 20;
	return fake_ccount - base;
#endif
}

