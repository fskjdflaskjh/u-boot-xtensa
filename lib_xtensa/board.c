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
#include <devices.h>
#include <version.h>
#include <net.h>
#include <config.h>
#include <xtensa.h>
#include <asm/stringify.h>		// XT-FIXME: move to generic
#include <asm/addrspace.h>
#include <asm/bootparam.h>

/* Import linker-defined symbols (see u-boot.lds) */
extern void *__monitor_start, *__monitor_end;

DECLARE_GLOBAL_DATA_PTR;

#ifndef __XTENSA_CALL0_ABI__
gd_t *gd;
#endif

#ifndef CONFIG_IDENT_STRING
#define CONFIG_IDENT_STRING ""
#endif

ulong monitor_flash_len;	/* size of U-Boot packed image in flash */

const char version_string[] =
	U_BOOT_VERSION" (" __DATE__ " - " __TIME__ ")"CONFIG_IDENT_STRING;

/*
 * Heap initialization for malloc(), and sbrk() definition.
 */

static ulong mem_malloc_start = 0;
static ulong mem_malloc_end = 0;
static ulong mem_malloc_brk = 0;

static
void mem_malloc_init(ulong dest_addr)
{
	mem_malloc_start = ALIGNUP(4, dest_addr);
	mem_malloc_end = dest_addr + CFG_MALLOC_LEN;
	mem_malloc_brk = mem_malloc_start;

	memset ((void *) mem_malloc_start, 0,
			mem_malloc_end - mem_malloc_start);
}

void *sbrk(ptrdiff_t increment)
{
	ulong old = mem_malloc_brk;
	ulong new = old + increment;

	if ((new < mem_malloc_start) || (new > mem_malloc_end)) {
		return (NULL);
	}
	mem_malloc_brk = new;

	return ((void *) old);
}

/*
 * Produce a floating point string of a frequency in MHz to 2 decimals.
 */
char *strmhz (char *buf, long hz)
{
	ulong l, m, n;

	n = hz / 1000000UL;
	l = sprintf (buf, "%lu", n);
	m = (hz % 1000000UL) / 100UL;
	sprintf (buf + l, ".%02lu", m);
	return (buf);
}

/*
 * Initialize baudrate settings in global and board data structures.
 * This does not actually initialize the UART or set its baudrate.
 * Return 0 on success (OK to continue), else non-zero (hang). 
 */
static int init_baudrate(void)
{
	char tmp[64];	/* long enough for environment variables */
	int i = getenv_r ("baudrate", tmp, sizeof (tmp));

	gd->baudrate = (i > 0)
			? (int) simple_strtoul (tmp, NULL, 10)
			: CONFIG_BAUDRATE;
	gd->bd->bi_baudrate = gd->baudrate;

	return 0;
}

/*
 * Initialize System RAM parameters and display RAM size.
 * Install dummy exception handler that ignores load/store exceptions.
 * Return 0 on success (OK to continue), else non-zero (hang). 
 */
static int init_sysram(void)
{
	gd->ram_size = CFG_MEMORY_SIZE;
	gd->bd->bi_memstart = CFG_MEMORY_BASE;
	gd->bd->bi_memsize  = CFG_MEMORY_SIZE;

	puts("SysRAM: ");
	print_size(gd->bd->bi_memsize, "\n");

	return 0;
}


/*
 * Various helper functions for the boot process.
 */

static int relocate (void)
{
	/* Relocate environment function pointers etc. */
	env_relocate();

	/* Initialize the jump table for applications */
	jumptable_init();

	return 0;
}
	
#ifndef CFG_NO_FLASH
/*
 * Initialize Flash memory and display Flash size.
 * Return 0 on success (OK to continue), else non-zero (hang). 
 */
static int init_flash(void)
{
	bd_t *bd = gd->bd;

	/* Must set monitor_flash_len before calling flash_init() */
	monitor_flash_len = (ulong)&__monitor_end - (ulong)&__monitor_start;

	bd->bi_flashstart = CFG_FLASH_BASE;
	bd->bi_flashsize = flash_init();

#if CFG_MONITOR_BASE == CFG_FLASH_BASE
	bd->bi_flashoffset = monitor_flash_len; /* skip U-Boot sectors */
#else
	bd->bi_flashoffset = 0;
#endif
	puts("Flash: ");
	print_size(bd->bi_flashsize, "\n");

	return 0;
}
#endif /* CFG_NO_FLASH */

#ifdef CONFIG_CMD_NET
/*
 * Initialize ethernet environment variables and board info.
 * The 'board' initializer should have set the 'ethaddr' environment variable
 * to the correct value if nothing was specified. Otherwise, we use the
 * invalid ethernet address: 00-00-00-00-00-00.
 */
static int ethernet_setup(void)
{
	bd_t *bd = gd->bd;
	char *s;

	s = getenv("ethaddr");
	if (s == 0) {
		s = "00:00:00:00:00:00";
		setenv("ethaddr", s);
	}

	puts("MAC:    ");
	puts(s);
	putc('\n');

	s = getenv("ipaddr");
	bd->bi_ip_addr = string_to_ip(s);
	if (s != 0) {
		puts("IP:     ");
		puts(s);
		putc('\n');
	}
	return 0;
}
#endif /* CONFIG_CMD_NET */

/*
 * Breath some life into the board...
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 *
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependend #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

init_fnc_t *init_sequence[] = {
	env_init,
	misc_init_f,
	init_baudrate,
	serial_init,
	console_init_f,
	display_options,
	checkcpu,
	checkboard,
	init_sysram,
	relocate,
	misc_init_r,	/* Make sure we run this before any device init. */
#ifndef CFG_NO_FLASH
	init_flash,
#endif
	devices_init,	/* Initialize devices */
	console_init_r, /* Initialize the console */
#ifdef CONFIG_CMD_NET
	ethernet_setup, /* Initialize ethernet */
#endif
	NULL,
};


/* 
 * Initialize board.
 */

void board_init_f(ulong data)
{
	/* 
	 * All RAM sections have been unpacked to RAM (relocated) 
	 * and the board has been initialized.
	 */
	init_fnc_t **init_fnc_ptr;
	bd_t *bd;
	ulong heap = CFG_HEAP_BASE;
	char mhz[8];

	gd = (gd_t *)(heap - sizeof(gd_t));
	memset((void *)gd, 0, sizeof(gd_t));
	gd->flags = GD_FLG_RELOC;	/* relocation is already done */
	bd = (bd_t *)((ulong)gd - sizeof(bd_t));
	memset((void *)bd, 0, sizeof(bd_t));
	gd->bd = bd;

	/* Reserve memory for passing linux boot parameters to kernel */
	bd->bi_boot_params = CFG_BOOTPARAMS_BASE;

	/* Initialize heap */
	mem_malloc_init(heap);

	/* Perform initialization sequence */
	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}

	/* Announce our arrival and clock frequency */
	display_printf("U-Boot %5s MHz", strmhz(mhz, gd->cpu_clk));

	while(1) 
		main_loop();

	/* NOT REACHED */
}


/*
 * I've fallen and I can't get up! Inform user and ask for reset, then wait.
 */

void hang(void)
{
	show_boot_progress(-30); /* -30 == "fatal error, hang the system" */

	puts ("### ERROR ### Please RESET the board ###\n");

	udelay(10000000);	/* 10s for user to read any existing message */
	lcd_disp_at_pos("Hung! Pls reset", 0);	/* low level so usually works */

	for (;;);
}
