/*
 * (C) Copyright 2007, Tensilica Inc.
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

#ifndef	_XTENSA_GBL_DATA_H
#define _XTENSA_GBL_DATA_H
/*
 * The following data structure is placed in some memory wich is
 * available very early after boot (like DPRAM on MPC8xx/MPC82xx, or
 * some locked parts of the data cache) to allow for a minimum set of
 * global variables during system initialization (until we have set
 * up the memory controller so that we can use RAM).
 *
 * Keep it *SMALL* and remember to set CFG_GBL_DATA_SIZE > sizeof(gd_t)
 */

typedef	struct	global_data {
	bd_t		*bd;		/* Board data for arch port */
	unsigned long	flags;		/* Flags (bitmasks below) */
	unsigned long	baudrate;	/* Serial port baud rate */
	unsigned long	cpu_clk;	/* CPU clock in Hz! */
	unsigned long	have_console;	/* console_init_f() was called */
	unsigned long	ram_size;	/* RAM size */
	unsigned long	reloc_off;	/* Relocation Offset (always 0) */
	unsigned long	env_addr;	/* Address  of Environment struct */
	unsigned long	env_valid;	/* Checksum of Environment valid? */
	void		**jt;		/* jump table */
} gd_t;

/*
 * Global Data Flags
 */
#define	GD_FLG_RELOC	0x00001		/* Code was relocated to RAM		*/
#define	GD_FLG_DEVINIT	0x00002		/* Devices have been initialized	*/
#define	GD_FLG_SILENT	0x00004		/* Silent mode				*/

/*
 * Reserve a global "register" as a global data ptr.
 * Should be callee-saved. This is only possible with Call0 ABI.
 * With Windowed ABI it is necessary to use a global variable (as for i386).
 */
#ifdef __XTENSA_CALL0_ABI__
  #define DECLARE_GLOBAL_DATA_PTR     register volatile gd_t *gd __asm__ ("a14")
#else
  #define DECLARE_GLOBAL_DATA_PTR     extern gd_t *gd
#endif

/*
 * We need to do this here to ensure it's declared in *all* C sources that
 * call functions (directly or indirectly) that reference global data ptr.
 * In particular print_size in lib_generic/display_options.c calls printf
 * which calls puts which references gd, but because gd is not declared in
 * display_options.c, the compiler (saves and) uses the register containing
 * gd, which is then corrupt when puts is called.
 * XT-FIXME: How do other archs deal with this? Do they just get lucky?
 */
DECLARE_GLOBAL_DATA_PTR;

#endif	/* _XTENSA_GBL_DATA_H */
