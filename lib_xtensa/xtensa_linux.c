/*
 * (C) Copyright 2008 Tensilica Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <command.h>
#include <image.h>
#include <zlib.h>
#include <asm/byteorder.h>
#include <asm/bootparam.h>
#include <asm/cache.h>

DECLARE_GLOBAL_DATA_PTR;

extern image_header_t header;           /* from cmd_bootm.c */

extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);


/*
 * Setup boot-parameters.
 */

static struct bp_tag *setup_first_tag(struct bp_tag *params)
{
	params->id = BP_TAG_FIRST;
	params->size = sizeof(long);
	*(unsigned long*)&params->data = BP_VERSION;

	return bp_tag_next(params);
}

static struct bp_tag *setup_last_tag(struct bp_tag *params)
{
	params->id = BP_TAG_LAST;
	params->size = 0;

	return bp_tag_next(params);
}

static struct bp_tag *setup_memory_tag(struct bp_tag *params)
{
	struct bd_info *bd = gd->bd;
	struct bp_memory *mem;

	params->id = BP_TAG_MEMORY;
	params->size = sizeof(struct bp_memory) + sizeof(struct bp_memory_bank);
	mem = (struct bp_memory*) params->data;
	mem->nr_banks = 1;
	mem->bank[0].type = BP_MEM_TYPE_DEFAULT;
	mem->bank[0].start = bd->bi_memstart;
	mem->bank[0].end = bd->bi_memstart + bd->bi_memsize;

	return bp_tag_next(params);
}

static struct bp_tag *setup_commandline_tag(struct bp_tag *params,char *cmdline)
{
	int len;

	if (!cmdline)
		return params;

	len = strlen(cmdline);

	params->id = BP_TAG_COMMAND_LINE;
	params->size = (len + 3) & -4;
	strcpy ((char*) params->data, cmdline);

	return bp_tag_next(params);
}

static struct bp_tag *setup_ramdisk_tag(struct bp_tag *params,
					unsigned long rd_start,
					unsigned long rd_end)
{
	struct bp_memory *mem;

	if (rd_start == rd_end)
		return params;

	/* Add a single banked memory. */

	params->id = BP_TAG_INITRD;
	params->size = sizeof(struct bp_memory) + sizeof(struct bp_memory_bank);

	mem = (struct bp_memory*) params->data;
	mem->nr_banks = 1;
	mem->bank[0].type = BP_MEM_TYPE_DEFAULT;
	mem->bank[0].start = PHYSADDR(rd_start);
	mem->bank[0].end = PHYSADDR(rd_end);

	return bp_tag_next(params);
}

static struct bp_tag *setup_serial_tag(struct bp_tag *params)
{
#if 0
	params->id = BP_TAG_SERIAL_PORT;
	params->size = sizeof (unsigned long);
	params->data[0] = gd->serial_port;

	params = bp_tag_next(params);
#endif

	params->id = BP_TAG_SERIAL_BAUDRATE;
	params->size = sizeof (unsigned long);
	params->data[0] = gd->baudrate;

	return bp_tag_next(params);
}


/*
 * Boot Linux.
 */

void do_bootm_linux (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[],
		     ulong addr, ulong * len_ptr, int verify)
{
	struct bp_tag *params, *params_start;
	image_header_t *hdr = &header;
	ulong initrd_start, initrd_end;
	ulong len = 0, checksum;
	ulong data;
	char *commandline = getenv ("bootargs");
	void (*theKernel) (struct bp_tag*);

	theKernel = (void*) ntohl(hdr->ih_ep);

	/*
	 * Check if there is an initrd image
	 */

	if (argc >= 3) {
		show_boot_progress (9);

		addr = simple_strtoul(argv[2], NULL, 16);

		printf ("## Loading Ramdisk Image at %08lx ...\n", addr);

		/* Copy header so we can blank CRC field for re-calculation */
		memcpy (&header, (char *) addr, sizeof (image_header_t));

		if (ntohl (hdr->ih_magic) != IH_MAGIC) {
			printf ("Bad Magic Number\n");
			show_boot_progress (-10);
			do_reset (cmdtp, flag, argc, argv);
		}

		data = (ulong) & header;
		len = sizeof (image_header_t);

		checksum = ntohl (hdr->ih_hcrc);
		hdr->ih_hcrc = 0;

		if (crc32 (0, (uchar *) data, len) != checksum) {
			printf ("Bad Header Checksum\n");
			show_boot_progress (-11);
			do_reset (cmdtp, flag, argc, argv);
		}

		show_boot_progress (10);

		print_image_hdr (hdr);

		data = addr + sizeof (image_header_t);
		len = ntohl (hdr->ih_size);

		if (verify) {
			ulong csum = 0;

			printf ("   Verifying Checksum ... ");
			csum = crc32 (0, (uchar *) data, len);
			if (csum != ntohl (hdr->ih_dcrc)) {
				printf ("Bad Data CRC\n");
				show_boot_progress (-12);
				do_reset (cmdtp, flag, argc, argv);
			}
			printf ("OK\n");
		}

		show_boot_progress (11);

		if ((hdr->ih_os != IH_OS_LINUX) ||
		    (hdr->ih_arch != IH_CPU_XTENSA) ||
		    (hdr->ih_type != IH_TYPE_RAMDISK)) {
			printf ("No Linux Xtensa Ramdisk Image\n");
			show_boot_progress (-13);
			do_reset (cmdtp, flag, argc, argv);
		}

		/*
		 * Now check if we have a multifile image
		 */
	} else if ((hdr->ih_type == IH_TYPE_MULTI) && (len_ptr[1])) {
		ulong tail = ntohl (len_ptr[0]) % 4;
		int i;

		show_boot_progress (13);

		/* skip kernel length and terminator */
		data = (ulong) (&len_ptr[2]);
		/* skip any additional image length fields */
		for (i = 1; len_ptr[i]; ++i)
			data += 4;
		/* add kernel length, and align */
		data += ntohl (len_ptr[0]);
		if (tail) {
			data += 4 - tail;
		}

		len = ntohl (len_ptr[1]);

	} else {
		/* no initrd image */
		show_boot_progress (14);
		len = data = 0;
	}

#ifdef	DEBUG
	if (!data) {
		printf ("No initrd\n");
	}
#endif

	if (data) {
		initrd_start = data;
		initrd_end = initrd_start + len;
	} else {
		initrd_start = 0;
		initrd_end = 0;
	}


	params = params_start = (struct bp_tag*)gd->bd->bi_boot_params;
	params = setup_first_tag(params);
	params = setup_memory_tag(params);
	params = setup_commandline_tag(params, commandline);
	params = setup_serial_tag(params);

	if (initrd_start)
		params = setup_ramdisk_tag(params, initrd_start, initrd_end);


	params = setup_last_tag(params);

	/*
	 * The Xtensa Linux kernel image bundles everything and passes
	 * pointers (to initrd image, etc.) via the bootparams area which 
	 * is packaged after the kernel in a multifile U-Boot image.
	 * The kernel expects only the bootargs address as its first arg.
	 * U-Boot's "bootargs" environment variable is ignored.
	 */
	if (hdr->ih_type == IH_TYPE_MULTI && len_ptr[1] != 0) {
		int i;
		int len = ntohl(len_ptr[0]);

		show_boot_progress (13);

		/* count images (at least 2) and ignore any extra */
		for (i = 2; len_ptr[i] != 0; ++i);

		/* find the boot params which is the second image */
		gd->bd->bi_boot_params = 
			(ulong)&len_ptr[i+1] + len + 4 - (len % 4);
	}
	else {
		printf ("Not a Linux Xtensa multifile image\n");
		//show_boot_progress (-13);
		//do_reset (cmdtp, flag, argc, argv);
	}
	
	show_boot_progress (15);

#ifdef DEBUG
	printf ("## Transferring control to Linux (at address %08lx) ...\n",
		(ulong) theKernel);
#endif

	printf ("\nStarting kernel ...\n\n");

	flush_cache_all();

	asm volatile (
"		mov	a2, %0		\n\t"
"		mov	a3, %1		\n\t"
"		jx	%2		\n\t"
		: : "a" (params_start), "a" (params_start), "a" (theKernel)
		: "memory");
}

