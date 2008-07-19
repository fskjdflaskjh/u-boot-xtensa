/*
 * Copyright (C) 2007-2008 Tensilica, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <asm/addrspace.h>

/*=====================*/
/* Board and Processor */
/*=====================*/
#define CONFIG_XTAV60				/* Avnet LX60 board + Xtensa */
#define CONFIG_SYS_CLK_FREQ	(gd->cpu_clk)	/* FPGA CPU freq after init */
#define CONFIG_TIMER_INDEX	0		/* Which (core) timer to use */
#define CFG_HZ			1000		/* Timer ticks per second */

/*=========================================*/
/* FPGA Registers (board info and control) */
/*=========================================*/
/*
 * These assume FPGA bitstreams from Tensilica release RB and up. Earlier
 * releases may not provide any/all of these registers or at these offsets.
 * Some of the FPGA registers are broken down into bitfields described by
 * SHIFT left amount and field WIDTH (bits), and also by a bitMASK.
 */

/* Date of FPGA bitstream build in binary coded decimal (BCD) */
#define CFG_FPGAREG_DATE	IOADDR(0x0D020000)
#define FPGAREG_MTH_SHIFT	24		/* BCD month 1..12 */
#define FPGAREG_MTH_WIDTH	8
#define FPGAREG_MTH_MASK	0xFF000000
#define FPGAREG_DAY_SHIFT	16		/* BCD day 1..31 */
#define FPGAREG_DAY_WIDTH	8
#define FPGAREG_DAY_MASK	0x00FF0000
#define FPGAREG_YEAR_SHIFT	0		/* BCD year 2001..9999 */
#define FPGAREG_YEAR_WIDTH	16
#define FPGAREG_YEAR_MASK	0x0000FFFF

/* FPGA core clock frequency in Hz (also input to UART) */
#define CFG_FPGAREG_FREQ	IOADDR(0x0D020004)	/* CPU clock frequency*/

/*
 * DIP switch (left=sw1=lsb=bit0, right=sw8=msb=bit7; off=0, on=1):
 *   Bits 0..5 set the lower 6 bits of the default ethernet MAC.
 *   Bit 6 is reserved for future use by Tensilica.
 *   Bit 7 maps the first 128KB of ROM address space at CFG_ROM_BASE to 
 *   the base of flash * (when on/1) or to the base of RAM (when off/0).
 */
#define CFG_FPGAREG_DIPSW	IOADDR(0x0D02000C)	/* DIP switch (8 bit) */
#define FPGAREG_MAC_SHIFT	0		/* Ethernet MAC bits 0..5 */
#define FPGAREG_MAC_WIDTH	6
#define FPGAREG_MAC_MASK	0x3f
#define FPGAREG_BOOT_SHIFT	7		/* Boot ROM addr mapping */
#define FPGAREG_BOOT_WIDTH	1
#define FPGAREG_BOOT_MASK	0x80
#define FPGAREG_BOOT_RAM	0			/* boot from RAM */
#define FPGAREG_BOOT_FLASH	(1<<FPGAREG_BOOT_SHIFT)	/* boot from flash */

/* Force hard reset of board by writing a code to this register */
#define CFG_FPGAREG_RESET	IOADDR(0x0D020010)	/* Hard reset board */
#define CFG_FPGAREG_RESET_CODE	0x0000DEAD	/*   by writing this code */

/*====================*/
/* ASCII Display Info */
/*====================*/
#define CFG_ASCDISP		1		/* Board has ASCII display */
#define CFG_ASCDISP_ADDR	IOADDR(0x0D040000) /* Display controller regs */
#define CFG_ASCDISP_LINES	2		/* Display number of lines */
#define CFG_ASCDISP_CHARS	16		/* Display chars per line */
#define CFG_ASCDISP_BUFSZ	64		/* Display printf buffer size */

/*====================*/
/* Serial Driver Info */
/*====================*/
#define CFG_NS16550
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE	4		/* NS16550 register size */
#define CFG_NS16550_COM1	IOADDR(0x0D050000) /* Base address of UART0 */
/* Input clk to NS16550 (in Hz; the SYS_CLK_FREQ is in kHz) */
#define CFG_NS16550_CLK		CONFIG_SYS_CLK_FREQ
#define CONFIG_CONS_INDEX	1		/* use UART0 for console */
#define CONFIG_BAUDRATE		38400		/* Default baud rate */
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/*=============*/
/* Ethernet Driver Info */
/*=============*/
#define CONFIG_DRIVER_OPEN_ETH
#define CONFIG_ETHBASE		00:50:C2:13:6f:00 /* Default MAC base (last  */
						  /* byte comes from DIP sw) */
#define CONFIG_ENV_OVERWRITE			/* Allow MAC address changes */
#define CFG_OETH_REG_BASE	IOADDR(0x0D030000) /* Register base */
#define CFG_OETH_BD_BASE	IOADDR(0x0D030400) /* Buffer descriptors base */
#define CFG_OETH_BUFF_ADDR	IOADDR(0x0D800000) /* Buffer base */

/*=====================*/
/* Flash & Environment */
/*=====================*/
/* XT-AV60 Flash has bottom block subdivided into 8x8KB parameter blocks */
#define CFG_FLASH_CFI				/* flash has a CFI interface */
#define CFG_FLASH_CFI_DRIVER			/* use generic CFI driver */
#define CFG_FLASH_CFI_WIDTH	FLASH_CFI_16BIT	/* bus interface width */
#define CFG_MAX_FLASH_BANKS	1		/* max number of flash banks */
#define CFG_FLASH_SIZE		0x400000	/* flash size 4MB */
#define CFG_FLASH_SECT_SZ	0x10000		/* primary   block size 64KB */
#define CFG_FLASH_PARMSECT_SZ	0x2000		/* parameter block size  8KB */
#define CFG_MAX_FLASH_SECT 	(CFG_FLASH_SECT_SZ/CFG_FLASH_PARMSECT_SZ + \
				 CFG_FLASH_SIZE/CFG_FLASH_SECT_SZ - 1)
#define CFG_FLASH_PROTECTION			/* hardware flash protection */
#define CFG_FLASH_BASE		IOADDR(0x08000000)	/* flash uncached base addr */
#define CFG_FLASH_TOP		(CFG_FLASH_BASE + CFG_FLASH_SIZE)
/* U-Boot monitor lives at the base of flash where it's mapped to ROM area */
#define CFG_MONITOR_BASE	CFG_FLASH_BASE	/* Image lives at flash base */
#define CFG_MONITOR_LEN		(128 << 10)	/* must fit in SRAM for OCD */
/* Put env. in top block (allows simple driver and avoids mangled memory map) */
/* Another option would be to put env. in 2nd param block offs 8KB, size 8KB */
#define CFG_ENV_IS_IN_FLASH			/* environment is in Flash */
#define CFG_ENV_OFFSET		(CFG_FLASH_SIZE - CFG_FLASH_SECT_SZ) /* top */
#define CFG_ENV_SECT_SIZE	CFG_FLASH_SECT_SZ /* env occupies 1 block */

/*===================*/
/* RAM Memory Layout */
/*===================*/
/*
 * Layout of the system RAM, top to bottom:
 *	stack (grows down toward heap)
 *	  (note: the boundary between stack and heap is fluid)
 *	malloc heap (grows up toward stack)
 *	global data and board info
 *	u-boot code (and unused space above)
 *	[linux boot parameters area (only if booting linux kernel)]
 *	free to load kernel or application (bulk of RAM)
 *	RAM vectors (most core configurations).
 */
#define CONFIG_STACKSIZE	(128 << 10)	/* stack size 128KB */
#define CFG_MALLOC_LEN		(256 << 10)	/* heap  size 256KB */
#define CFG_GBL_DATA_SIZE	128		/* global & board data size */

/* U-Boot malloc heap and stack area (grow together at top of RAM) */
#define CFG_HEAP_BASE  (CFG_MEMORY_TOP - CONFIG_STACKSIZE - CFG_MALLOC_LEN)

/* U-Boot area in RAM after unpacking ('make' symbol TEXT_BASE points here) */
#define CFG_TEXT_LEN		(CFG_MONITOR_LEN - CFG_GBL_DATA_SIZE)
#define CFG_TEXT_BASE		(CFG_HEAP_BASE - CFG_MONITOR_LEN)

/* Linux boot params area in RAM (used only when booting linux) */
#define CFG_BOOTPARAMS_LEN	(64  << 10)	/* linux boot param area size */
#define CFG_BOOTPARAMS_BASE	(CFG_TEXT_BASE - CFG_BOOTPARAMS_LEN)

/* U-Boot initial RAM area before it unpacks itself (not used in this port). */

/* Memory test is destructive so default must not overlap vectors or U-Boot */
#define CFG_MEMTEST_START	MEMADDR(0x001000000)	/* memtest start addr */
#define CFG_MEMTEST_END		MEMADDR(0x002000000)	/* memtest end addr+1 */

#define CFG_MEMORY_ADDR		MEMADDR(0x00000000)
#define CFG_MEMORY_SIZE		0x04000000
#define CFG_LOAD_ADDR		MEMADDR(0x02000000)
#define CFG_MEMORY_TOP		(CFG_MEMORY_BASE + CFG_MEMORY_SIZE)

/*==============================*/
/* U-Boot general configuration */
/*==============================*/
#undef	CONFIG_USE_IRQ				/* Keep it simple, poll only */
#define CONFIG_MISC_INIT_R
#undef	CONFIG_BOOTDELAY
#define CONFIG_BOOTFILE		"uImage"	/* Boot file name */
#define CFG_PROMPT		"U-Boot> "	/* Monitor Command Prompt */
#define CFG_CBSIZE		1024		/* Console I/O Buffer Size  */
#define CFG_PBSIZE		(CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Prt buf */
#define CFG_MAXARGS		16		/* max number of command args */
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size */
						
#define CONFIG_VERSION_VARIABLE
#define CONFIG_AUTO_COMPLETE			/* Support tab autocompletion */
#if 0						/* Leave hush out for now */
#undef CONFIG_AUTO_COMPLETE			/* Not implemented for hush */
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2	"> "
#endif
#define CONFIG_CMDLINE_EDITING
#define CFG_LONGHELP
#define CONFIG_CRC32_VERIFY
#define CONFIG_MX_CYCLIC
#define CONFIG_SHOW_BOOT_PROGRESS

#ifdef DEBUG
#define CONFIG_PANIC_HANG       1               /* Require manual reboot */
#endif

/*=================*/
/* U-Boot commands */
/*=================*/
#include <config_cmd_default.h>
#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_DIAG
#define CONFIG_CMD_PING
#define CONFIG_CMD_SAVES
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_SETGETDCR
#undef CONFIG_CMD_XIMG

#endif /* __CONFIG_H */
