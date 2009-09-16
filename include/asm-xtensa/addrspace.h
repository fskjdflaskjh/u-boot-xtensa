/*
 * include/asm-xtensa/addrspace.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2008 Tensilica Inc.
 */

#include <asm/variant/core.h>
#ifndef _XTENSA_ADDRSPACE_H
#define _XTENSA_ADDRSPACE_H

/*
 * MMU Memory Map
 *
 * IO (uncached)	f0000000..ffffffff	-> f000000
 * IO (cached)		e0000000..efffffff	-> f000000
 * MEM (uncached)	d8000000..dfffffff	-> 0000000
 * MEM (cached)		d0000000..d7ffffff	-> 0000000
 *
 */

#define CONFIG_SYS_IO_BASE		0xf0000000
#define CONFIG_SYS_MEMORY_BASE		0xd0000000

#define IOADDR(x)		(CONFIG_SYS_IO_BASE + (x))
#define MEMADDR(x)		(CONFIG_SYS_MEMORY_BASE + (x))
#define PHYSADDR(x)		((x) - (CONFIG_SYS_MEMORY_BASE))

#endif	/* _XTENSA_ADDRSPACE_H */
