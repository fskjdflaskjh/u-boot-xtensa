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


#include <asm/asmmacro.h>
#include <asm/regs.h>

/*
 * Offsets into the the pt_regs struture.
 * Make sure these always match with the structure defined in ptrace.h!
 */

#define PT_PC		0
#define PT_PS		4
#define PT_DEPC		8
#define PT_EXCCAUSE	12
#define PT_EXCVADDR	16
#define PT_DEBUGCAUSE	20
#define PT_WMASK	24
#define PT_LBEG		28
#define PT_LEND		32
#define PT_LCOUNT	36
#define PT_SAR		40
#define PT_WINDOWBASE	44
#define PT_WINDOWSTART	48
#define PT_SYSCALL	52
#define PT_ICOUNTLEVEL	56
#define PT_RESERVED	60
#define PT_AREG		64
#define PT_SIZE		(64 + 64)

/*
 * Reset vector.
 *  Only a trampoline to jump to _start  
 */

	.section .ResetVector.text, "ax"
	.global _ResetVector
_ResetVector:

	j	1f
	.align 4
2:	.long	_start

1:	l32r	a2, 2b
	jx	a2


/*
 * Processor initialization. We still run in rom space.
 */

	.section .reset.text, "ax"
	.global _start
	.align	16
_start:

	/* Keep a0 = 0 for various initializations. */

	movi	a0, 0

	/* Disable dbreak debug exceptions. */

#if XCHAL_HAVE_DEBUG && XCHAL_NUM_DBREAK > 0
	.set	_index, 0
	.rept	XCHAL_NUM_DBREAK - 1
	wsr	a0, DBREAKC + _index
	.set	_index, _index + 1
	.endr
#endif

	/* Reset windowbase and windowstart. */

#if XCHAL_HAVE_WINDOWED
	movi	a3, 1
	wsr	a0, WINDOWBASE
	wsr	a3, WINDOWSTART
#endif

	/* Disable loops. */

	wsr	a0, LCOUNT

	/* Set PS.INTLEVEL = 1, PS.WOE = 0, PS.EXCM = 0 */

	movi	a2, 1
	wsr	a2, PS
	rsync

	/* Unlock and invalidate caches. */

        movi    a2, 0
	__loopi	a2, a3, XCHAL_DCACHE_SIZE, 4 << XCHAL_DCACHE_LINEWIDTH
#if XCHAL_DCACHE_LINE_LOCKABLE
	diu	a2, 0 << XCHAL_DCACHE_LINEWIDTH
	diu	a2, 1 << XCHAL_DCACHE_LINEWIDTH
	diu	a2, 2 << XCHAL_DCACHE_LINEWIDTH
	diu	a2, 3 << XCHAL_DCACHE_LINEWIDTH
#endif
	dii	a2, 0 << XCHAL_DCACHE_LINEWIDTH
	dii	a2, 1 << XCHAL_DCACHE_LINEWIDTH
	dii	a2, 2 << XCHAL_DCACHE_LINEWIDTH
	dii	a2, 3 << XCHAL_DCACHE_LINEWIDTH
	__endla	a2, a3, 4 << XCHAL_DCACHE_LINEWIDTH

        movi    a2, 0
	__loopi	a2, a3, XCHAL_ICACHE_SIZE, 4 << XCHAL_ICACHE_LINEWIDTH
#if XCHAL_ICACHE_LINE_LOCKABLE
	iiu	a2, 0 << XCHAL_ICACHE_LINEWIDTH
	iiu	a2, 1 << XCHAL_ICACHE_LINEWIDTH
	iiu	a2, 2 << XCHAL_ICACHE_LINEWIDTH
	iiu	a2, 3 << XCHAL_ICACHE_LINEWIDTH
#endif
	iii	a2, 0 << XCHAL_ICACHE_LINEWIDTH
	iii	a2, 1 << XCHAL_ICACHE_LINEWIDTH
	iii	a2, 2 << XCHAL_ICACHE_LINEWIDTH
	iii	a2, 3 << XCHAL_ICACHE_LINEWIDTH
	__endla	a2, a3, 4 << XCHAL_ICACHE_LINEWIDTH

	isync

	/* Unpack data sections. */

	movi	a2, __boot_reloc_table_start
	movi	a3, __boot_reloc_table_end

1:	beq	a2, a3, 3f	# no more entries?
	l32i	a4, a2, 0	# start destination (in RAM)
	l32i	a5, a2, 4	# end destination (in RAM)
	l32i	a6, a2, 8	# star source (in ROM)
	addi	a2, a2, 12	# next entry
	beq	a4, a5, 1b	# skip, empty entry
	beq	a4, a6, 1b	# skip, source and destination are the same

2:	l32i	a7, a6, 0
	addi	a6, a6, 4
	s32i	a7, a4, 0
	addi	a4, a4, 4
	bltu	a4, a5, 2b
	j	1b

3:	/* All code and initalized data segments have been copied. */

	/* Clear BSS */

	movi	a2, _bss_start
	movi	a3, _bss_end

	__loopt	a2, a3, a4, 2
	s32i	a0, a2, 0
	__endla	a2, a4, 4

	/* Writeback */

        movi    a2, 0
	__loopi	a2, a3, XCHAL_DCACHE_SIZE, 4 << XCHAL_DCACHE_LINEWIDTH
	diwb	a2, 0 << XCHAL_DCACHE_LINEWIDTH
	diwb	a2, 1 << XCHAL_DCACHE_LINEWIDTH
	diwb	a2, 2 << XCHAL_DCACHE_LINEWIDTH
	diwb	a2, 3 << XCHAL_DCACHE_LINEWIDTH
	__endla	a2, a3, 4 << XCHAL_DCACHE_LINEWIDTH

	/* Setup stack and enable window exceptions. */

	movi	a1, stack_end

	movi	a2, 0x00040001		# WOE = 1, INTLEVEL = 1, UM = 0
	wsr	a2, PS
	rsync

	movi	a4, board_init_f
1:	callx4	a4
	j	1b

/*
 * Exception vectors.
 *
 *  Verious notes:
 *   - We currently don't use the user exception vector (PS.UM is always 0),
 *     but do define such a vector, just in case. They both jump to the
 *     same exception handler, though.
 *   - The double exception vector currently also jumps to the same
 *     common exception handler. (should probably do something different)
 *   - DEPC is set to 0 when coming from the kernel (or user) exception, 
 *     but retains the correct value when coming from the double exception
 *     vector. This can be used to distinguish between double and the other
 *     exceptions.
 *   - We currently only save the bare minimum number of registers:
 *     a0...a15, sar, loop-registers, exception register (epc1, excvaddr,
 *     exccause, depc)
 *   - WINDOWSTART is only saved to identify if registers have been spilled
 *     to the wrong stack (exception stack) while executing the exception
 *     handler.
 */

	.section .KernelExceptionVector.text, "ax"

	.global _KernelExceptionVector
_KernelExceptionVector:

	wsr	a2, EXCSAVE1
	movi	a0, 0
	wsr	a0, DEPC
	movi	a2, ExceptionHandler
	jx	a2

	.global _UserExceptionVector
_UserExceptionVector:

	wsr	a2, EXCSAVE1
	movi	a0, 0
	wsr	a0, DEPC
	movi	a2, ExceptionHandler
	jx	a2

#if XCHAL_HAVE_XEA2
	.global _DoubleExceptionVector
_DoubleExceptionVector:

	wsr	a2, EXCSAVE1
	movi	a2, ExceptionHandler
	jx	a2
#endif


	.text
	.align 4
ExceptionHandler:

	rsr	a2, EXCCAUSE		# find handler

	/* Special case for alloca handler. */

	bnei	a2, 5, 1f		# jump if not alloca exception

	addi	a1, a1, -16 - 4		# create a small stack frame
	s32i	a3, a1, 0		# and save a3 (a2 still in excsave1)
	movi	a2, fast_alloca_exception
	jx	a2			# jump to fast_alloca_exception

	/* All other exceptions go here: */

	/* Create ptrace stack and save a0...a3 */

1:	addi	a2, a1, - PT_SIZE - 16
	s32i	a0, a2, PT_AREG + 0 * 4
	s32i	a1, a2, PT_AREG + 1 * 4
	s32i	a3, a2, PT_AREG + 3 * 4
	rsr	a3, EXCSAVE1
	s32i	a3, a2, PT_AREG + 2 * 4
	mov	a1, a2

	/* Save remaining AR registers. */

	s32i	a4, a1, PT_AREG + 4 * 4
	s32i	a5, a1, PT_AREG + 5 * 4
	s32i	a6, a1, PT_AREG + 6 * 4
	s32i	a7, a1, PT_AREG + 7 * 4
	s32i	a8, a1, PT_AREG + 8 * 4
	s32i	a9, a1, PT_AREG + 9 * 4
	s32i	a10, a1, PT_AREG + 10 * 4
	s32i	a11, a1, PT_AREG + 11 * 4
	s32i	a12, a1, PT_AREG + 12 * 4
	s32i	a13, a1, PT_AREG + 13 * 4
	s32i	a14, a1, PT_AREG + 14 * 4
	s32i	a15, a1, PT_AREG + 15 * 4

	/* Save SRs */

	rsr	a2, WINDOWSTART
	s32i	a2, a1, PT_WINDOWSTART
	rsr	a2, DEPC
	s32i	a2, a1, PT_DEPC

	rsr	a2, SAR
	rsr	a3, EPC1
	s32i	a2, a1, PT_SAR
	s32i	a3, a1, PT_PC

#if XCHAL_HAVE_LOOPS
	movi	a2, 0
	rsr	a3, LBEG
	xsr	a2, LCOUNT
	s32i	a3, a1, PT_LBEG
	rsr	a3, LEND
	s32i	a2, a1, PT_LCOUNT
	s32i	a3, a1, PT_LEND
#endif

	/* Jump to registered handler. */

	rsr	a2, EXCCAUSE
	movi	a3, (1 << PS_WOE_BIT)|1	# disable interrupts
	xsr	a3, PS
	rsync
	s32i	a2, a1, PT_EXCCAUSE
	s32i	a3, a1, PT_PS

	movi	a4, exc_table
	addx4	a4, a2, a4
	l32i	a4, a4, 0
	mov	a6, a1			# Provide stack frame as only argument
	callx4	a4

	/* Go to exception mode (PS.EXCM=1) */

	wsr	a3, PS

	/* Restore SR registers */

#if XCHAL_HAVE_LOOPS
	l32i	a2, a1, PT_LBEG
	l32i	a3, a1, PT_LEND
	l32i	a4, a1, PT_LCOUNT
	wsr	a2, LBEG
	wsr	a3, LEND
	wsr	a4, LCOUNT
#endif

	l32i	a2, a1, PT_SAR
	l32i	a3, a1, PT_PC
	wsr	a2, SAR
	wsr	a3, EPC1

	/* Do we need to simulate a MOVSP? */

	l32i	a2, a1, PT_WINDOWSTART
	addi	a3, a2, -1
	and	a2, a2, a3
	beqz	a2, 1f			# Skip if regs were spilled before exc.

	rsr	a2, WINDOWSTART
	addi	a3, a2, -1
	and	a2, a2, a3
	bnez	a2, 1f			# Skip if registers aren't spilled now

	addi	a2, a1, -16
	l32i	a4, a2, 0
	l32i	a5, a2, 4
	s32i	a4, a1, PT_SIZE + 0
	s32i	a5, a1, PT_SIZE + 4
	l32i	a4, a2, 8
	l32i	a5, a2, 12
	s32i	a4, a1, PT_SIZE + 8
	s32i	a5, a1, PT_SIZE + 12

	/* Restore address register. */

1:	l32i	a15, a1, PT_AREG + 15 * 4
	l32i	a14, a1, PT_AREG + 14 * 4
	l32i	a13, a1, PT_AREG + 13 * 4
	l32i	a12, a1, PT_AREG + 12 * 4
	l32i	a11, a1, PT_AREG + 11 * 4
	l32i	a10, a1, PT_AREG + 10 * 4
	l32i	a9, a1, PT_AREG + 9 * 4
	l32i	a8, a1, PT_AREG + 8 * 4
	l32i	a7, a1, PT_AREG + 7 * 4
	l32i	a6, a1, PT_AREG + 6 * 4
	l32i	a5, a1, PT_AREG + 5 * 4
	l32i	a4, a1, PT_AREG + 4 * 4
	l32i	a3, a1, PT_AREG + 3 * 4
	l32i	a2, a1, PT_AREG + 2 * 4

	/* Check if we came from a double exception. */

	l32i	a0, a1, PT_DEPC
	bnez	a0, 1f

	l32i	a0, a1, PT_AREG + 0 * 4
	l32i	a1, a1, PT_AREG + 1 * 4

	rfe

1:	wsr	a0, DEPC
	l32i	a0, a1, PT_AREG + 0 * 4
	l32i	a1, a1, PT_AREG + 1 * 4

	rfde



/*
 * Window overflow and underflow handlers.
 * The handlers must be 64 bytes apart, first starting with the underflow
 * handlers underflow-4 to underflow-12, then the overflow handlers
 * overflow-4 to overflow-12.
 *
 * Note: We rerun the underflow handlers if we hit an exception, so
 *	 we try to access any page that would cause a page fault early.
 */

	.section .WindowVectors.text, "ax"

/* 4-Register Window Overflow Vector (Handler) */

	.align 64
.global _WindowOverflow4
_WindowOverflow4:
	s32e	a0, a5, -16
	s32e	a1, a5, -12
	s32e	a2, a5,  -8
	s32e	a3, a5,  -4
	rfwo


/* 4-Register Window Underflow Vector (Handler) */

	.align 64
.global _WindowUnderflow4
_WindowUnderflow4:
	l32e	a0, a5, -16
	l32e	a1, a5, -12
	l32e	a2, a5,  -8
	l32e	a3, a5,  -4
	rfwu

/*
 * a0:	a0
 * a1:	new stack pointer = a1 - 16 - 4
 * a2:	available, saved in excsave1
 * a3:	available, saved on stack *a1
 */

/* 15*/	.byte	0xff

fast_alloca_exception:	/* must be at _WindowUnderflow4 + 16

/* 16*/	rsr	a2, PS
/* 19*/	rsr	a3, WINDOWBASE
/* 22*/	extui	a2, a2, PS_OWB_SHIFT, PS_OWB_SHIFT
/* 25*/	xor	a2, a2, a3
/* 28*/	rsr	a3, PS
/* 31*/	slli	a2, a2, PS_OWB_SHIFT
/* 34*/	xor	a2, a3, a2
/* 37*/	wsr	a2, PS

/* 40*/	_l32i	a3, a1, 0
/* 43*/	addi	a1, a1, 16 + 4
/* 46*/	rsr	a2, EXCSAVE1

/* 49*/	rotw	-1
/* 52*/	_bbci.l	a4, 31, _WindowUnderflow4	/* 0x: call4 */
/* 55*/	rotw	-1
/* 58*/	_bbci.l	a8, 30, _WindowUnderflow8	/* 10: call8 */
/* 61*/ _j	__WindowUnderflow12		/* 11: call12 */
/* 64*/

/* 8-Register Window Overflow Vector (Handler) */

	.align 64
.global _WindowOverflow8
_WindowOverflow8:
	s32e	a0, a9, -16
	l32e	a0, a1, -12
	s32e	a2, a9,  -8
	s32e	a1, a9, -12
	s32e	a3, a9,  -4
	s32e	a4, a0, -32
	s32e	a5, a0, -28
	s32e	a6, a0, -24
	s32e	a7, a0, -20
	rfwo

/* 8-Register Window Underflow Vector (Handler) */

	.align 64
.global _WindowUnderflow8
_WindowUnderflow8:
	l32e	a1, a9, -12
	l32e	a0, a9, -16
	l32e	a7, a1, -12
	l32e	a2, a9,  -8
	l32e	a4, a7, -32
	l32e	a3, a9,  -4
	l32e	a5, a7, -28
	l32e	a6, a7, -24
	l32e	a7, a7, -20
	rfwu

/* 12-Register Window Overflow Vector (Handler) */

	.align 64
.global _WindowOverflow12
_WindowOverflow12:
	s32e	a0,  a13, -16
	l32e	a0,  a1,  -12
	s32e	a1,  a13, -12
	s32e	a2,  a13,  -8
	s32e	a3,  a13,  -4
	s32e	a4,  a0,  -48
	s32e	a5,  a0,  -44
	s32e	a6,  a0,  -40
	s32e	a7,  a0,  -36
	s32e	a8,  a0,  -32
	s32e	a9,  a0,  -28
	s32e	a10, a0,  -24
	s32e	a11, a0,  -20
	rfwo

/* 12-Register Window Underflow Vector (Handler) */

	.org _WindowOverflow12 + 64 - 3
__WindowUnderflow12:
	rotw	-1
.global _WindowUnderflow12
_WindowUnderflow12:
	l32e	a1,  a13, -12
	l32e	a0,  a13, -16
	l32e	a11, a1,  -12
	l32e	a2,  a13,  -8
	l32e	a4,  a11, -48
	l32e	a8,  a11, -32
	l32e	a3,  a13,  -4
	l32e	a5,  a11, -44
	l32e	a6,  a11, -40
	l32e	a7,  a11, -36
	l32e	a9,  a11, -28
	l32e	a10, a11, -24
	l32e	a11, a11, -20
	rfwu

.section ".bss"
	.global	stack
	.global stack_end
	.align	16
stack:	.fill	0x2000, 1, 0
stack_end:


