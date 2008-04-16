
#include <asm/asmmacro.h>
#include <asm/regs.h>


	.section .ResetVector.text, "ax"
	.global _ResetVector
_ResetVector:

#if XCHAL_HAVE_DEBUG && XCHAL_NUM_DBREAK > 0

	movi	a0, 0
	.set	_index, 0
	.rept	XCHAL_NUM_DBREAK - 1
	wsr	a0, DBREAKC + _index
	.set	_index, _index + 1
	.endr

#endif

	j	1f
	.align 4
2:	.long	_start

1:	l32r	a2, 2b
	jx	a2



	.section .reset.text, "ax"
	.global _start
	.align	16
_start:

	/* Keep a0 = 0 for various initializations. */

	movi	a0, 0

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
 * Kernel exception vector. (Exceptions with PS.UM == 0, PS.EXCM == 0)
 *
 * We get this exception when we were already in kernel space.
 * We decrement the current stack pointer (kernel) by PT_SIZE and
 * jump to the first-level handler associated with the exception cause.
 *
 */

	.section .KernelExceptionVector.text, "ax"

	.global _KernelExceptionVector
_KernelExceptionVector:

	wsr	a2, DEPC		# copy a2 to DEPC
	addi	a2, a1, -16-64		# get new stack frame
	s32i	a0, a2, 0 * 4		# save a0 and a3 to stack
	s32i	a3, a2, 3 * 4
	rsr	a0, EXCCAUSE		# find handler
	movi	a3, exc_table_fast_kernel
	addx4	a3, a0, a3
	l32i	a3, a3, 0
	jx	a3			# jump to handler

/* Window overflow and underflow handlers.
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
 * a0:	available, original saved on stack (_PT_AREG0)
 * a1:	a1
 * a2:	new stack pointer, original in DEPC
 * a3:	available, original saved on stack (_PT_AREG3)
 */

/* 15*/	.byte	0xff

fast_alloca_exception:	/* must be at _WindowUnderflow4 + 16

/* 16*/	rsr	a0, PS
/* 19*/	rsr	a3, WINDOWBASE
/* 22*/	extui	a0, a0, PS_OWB_SHIFT, PS_OWB_SHIFT
/* 25*/	xor	a0, a0, a3
/* 28*/	rsr	a3, PS
/* 31*/	slli	a0, a0, PS_OWB_SHIFT
/* 34*/	xor	a0, a3, a0
/* 37*/	wsr	a0, PS

/* 40*/	_l32i	a0, a2, 0 * 4
/* 43*/	_l32i	a3, a2, 3 * 4
/* 46*/	rsr	a2, DEPC

	/* return address:
	 * 01	call4
	 * 10	call8 (most common)
	 * 11	call12
	 */

/* 49*/	rotw	-1
/* 52*/	_bbci.l	a4, 31, _WindowUnderflow4
/* 55*/	rotw	-1
/* 58*/	_bbci.l	a8, 30, _WindowUnderflow8
/* 61*/ _j	__WindowUnderflow12
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

	.data
	.global exc_table_fast_kernel
	.align 16
exc_table_fast_kernel:
	.long	0			/*  0 illegal */
	.long	0			/*  1 syscall */
	.long	0			/*  2 instr. fetch */
	.long	0			/*  3 load store */
	.long	0			/*  4 level1 intr */
	.long	fast_alloca_exception	/*  5 alloca */
	.long	0			/*  6 divide by 0 */
	.long	0			/*  7 speculation */
	.long	0			/*  8 privileged */
	.long	0			/*  9 unaligned */
	.long	0			/* 10 unused */
	.long	0			/* 11 unused */
	.long	0			/* 12 unused */
	.long	0			/* 13 unused */
	.long	0			/* 14 unused */
	.long	0			/* 15 unused */
	.long	0			/* 16 itlb miss */
	.long	0			/* 17 itlb multihit */
	.long	0			/* 18 itlb privilege */
	.long	0			/* 19 itlb size restriction */
	.long	0			/* 20 fetch cache attribute */
	.long	0			/* 21 unused */
	.long	0			/* 22 unused */
	.long	0			/* 23 unused */
	.long	0			/* 24 dtlb miss */
	.long	0			/* 25 dtlb multihit */
	.long	0			/* 26 dtlb privilege */
	.long	0			/* 27 dtlb size restriction */
	.long	0			/* 28 load cache attribute */
	.long	0			/* 29 store cache attribute */
	.long	0			/* 30 unused */
	.long	0			/* 31 unused */
	.long	0			/* 32 coprocessor 0 */
	.long	0			/* 33 coprocessor 1 */
	.long	0			/* 34 coprocessor 2 */
	.long	0			/* 35 coprocessor 3 */
	.long	0			/* 36 coprocessor 4 */
	.long	0			/* 37 coprocessor 5 */
	.long	0			/* 38 coprocessor 6 */
	.long	0			/* 39 coprocessor 7 */

.section ".bss"
	.global	stack
	.global stack_end
	.align	16
stack:	.fill	0x2000, 1, 0
stack_end:


