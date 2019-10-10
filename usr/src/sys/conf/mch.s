// V7/x86 source code: see www.nordier.com/v7x86 for details.
// Copyright (c) 1999 Robert Nordier.  All rights reserved.

// machine language assist
// for x86

.set	PGSZ,0x1000
.set	PGSH,0xc

.set	TSS,0x08
.set	KCODE,0x10
.set	KDATA,0x18
.set	UCODE,0x23
.set	UDATA,0x2b

.set	KBASE,0x7fc00000
.set	KLOAD,0x10000
.set	KA,KBASE-KLOAD

.set	IPL0,0xffff
.set	IPL1,0xfdfb
.set	IPL4,0xfd41
.set	IPL5,0xbd01
.set	IPL6,0xbd00
.set	IPL7,0

.set	U,0x7ff9d000
.set	UTAB,U+0xe4
.set	RW,7

.text

.globl	_start
_start:

// We start off in real mode
.code16

	cli
	cs; lgdt gdtd
	mov	cr0,eax
	inc	ax
	mov	eax,cr0
	ljmpl	$KCODE,$1f-KA

// Now in 32-bit protected mode
.code32

1:
	mov	$KDATA,eax
	mov	ax,es
	mov	ax,ds
	mov	ax,ss
	mov	$_start-KA,esp
	push	$0x2
	popf

// Clear bss

	mov	$_edata-KA,edi
	mov	$_end-KA,ecx
	add	$PGSZ-1,ecx
	and	$![PGSZ-1],ecx
	sub	edi,ecx
	xor	eax,eax
	rep; stosb
	mov	edi,_kend-KA

// Clear some additional pages

	mov	$9*PGSZ,ecx
	rep; stosb

// Enable access to memory above one megabyte

1:
	inb	$0x64,al
	testb	$0x2,al
	jnz	1b
	movb	$0xd1,al		// Write output port
	outb	al,$0x64
1:
	inb	$0x64,al
	testb	$0x2,al
	jnz	1b
	movb	$0xdf,al		// Enable A20
	outb	al,$0x60

// Determine installed memory, taking first megabyte on trust
// We currently make use of only 16M

	mov	$0x100000,edi
	mov	$0x5aa5a55a,eax
	mov	$0xa55a5aa5,edx
	mov	$[14-1]*256,ecx 	// Reserving 2M
	push	 ecx
1:
	mov	(edi),ebx
	mov	eax,(edi)
	cmp	eax,(edi)
	jne	2f
	mov	edx,(edi)
	cmp	edx,(edi)
2:
	mov	ebx,(edi)
	jne	1f
	add	$PGSZ,edi
	loop	1b
1:
	pop	eax
	sub	ecx,eax
	add	$256,eax
	mov	eax,_phymem-KA

// Pages are allocated as follows:
//     0    page directory
//     1    user page table
//     2-5  core page tables
//     6    kernel page table
//     7    u. area
//     8    kernel stack

	mov	_kend-KA,ebx

// Set up page directory

	lea	0*PGSZ(ebx),edi
	lea	1*PGSZ(ebx),eax
	movb	$0x7,al
	mov	eax,(edi)
	lea	6*PGSZ(ebx),eax
	movb	$0x3,al
	mov	eax,511*4(edi)
	lea	256*4(ebx),edi
	lea	2*PGSZ(ebx),eax
	mov	$4,ecx
	movb	$0x3,al
1:
	stosl
	add	$PGSZ,eax
	loop	1b

// Set up local page table

	lea	1*PGSZ+[16*4](ebx),edi
	mov	$_start-KA,eax
	mov	$16,ecx
	movb	$0x3,al
1:
	stosl
	add	$PGSZ,eax
	loop	1b

// Set up core page tables

	lea	2*PGSZ(ebx),edi
	xor	eax,eax
	mov	$4096,ecx
	movb	$0x3,al
1:
	stosl
	add	$PGSZ,eax
	loop	1b

// Set up kernel page table

	lea	6*PGSZ(ebx),edi

	mov	$_start-KA,eax
	mov	$_etext-KA,ecx
	add	$PGSZ-1,ecx
	sub	eax,ecx
	shr	$PGSH,ecx
	movb	$0x1,al
1:
	stosl
	add	$PGSZ,eax
	loop	1b
	xorb	al,al

	lea	0*PGSZ(ebx),ecx
	sub	eax,ecx
	shr	$PGSH,ecx
	movb	$0x3,al
1:
	stosl
	add	$PGSZ,eax
	loop	1b

	lea	[922*4]+[6*PGSZ](ebx),edi
	lea	0*PGSZ(ebx),eax
	movb	$0x3,al
	stosl
	add	$PGSZ,eax
	stosl
	add	$5*PGSZ,eax
	stosl
	add	$PGSZ,eax
	stosl
	add	$4,edi
	add	$PGSZ,eax
	stosl

	mov	$0xa0000,eax
	mov	$96,ecx
	movb	$0x13,al
1:
	stosl
	add	$PGSZ,eax
	loop	1b

// Enable paging

	lea	0*PGSZ(ebx),eax
	mov	eax,cr3
	mov	cr0,eax
	or	$0x80000000,eax
	mov	eax,cr0
	ljmp	$KCODE,$1f

1:
	lgdt	gdtd2
	mov	$kstk,esp

// Create IDT

	mov	$ivt,edi
	mov	$idtctl,esi
1:
	lodsl
	test	eax,eax
	jz	1f
	mov	eax,edx
	lodsl
	mov	edx,ebx
	mov	$0x10,ecx
2:
	shl	ebx
	jnc	3f
	movw	ax,(edi)
	movb	$KCODE,0x2(edi)
	mov	eax,0x4(edi)
	movw	dx,0x4(edi)
	add	$0x4,eax
3:
	add	$0x8,edi
	loop	2b
	jmp	1b
1:
	movb	$0xee,ivt+[3*8]+5	// allow int3
	lidt	idtd

// Set IRQ offsets

	movb	$0x11,al		// ICW1
	outb	al,$0x20
	outb	al,$0xa0
	movb	$0x20,al		// ICW2
	outb	al,$0x21
	movb	$0x28,al
	outb	al,$0xa1
	movb	$0x4,al 		// ICW3
	outb	al,$0x21
	movb	$0x2,al
	outb	al,$0xa1
	movb	$0x1,al 		// ICW4
	outb	al,$0x21
	outb	al,$0xa1
	sti

// Set up TSS

	mov	esp,tss+0x4
	movl	$KDATA,tss+0x8

// Done

	call	_main

	mov	$TSS,eax
	ltr	ax
	mov	$UCODE,eax
	mov	ax,es
	mov	$UDATA,eax
	mov	ax,es
	mov	ax,ds
	push	eax
	pushl	$0x1000
	pushl	$0x202
	pushl	$UCODE
	pushl	$0
	iret

// Exception jump table

intx00:
	push	$0x0			// #DE
	jmp	trap0
	push	$0x1			// #DB
	jmp	trap0
	push	$0x2			// (NMI)
	jmp	trap0
	push	$0x3			// #BP
	jmp	trap0
	push	$0x4			// #OF
	jmp	trap0
	push	$0x5			// #BR
	jmp	trap0
	push	$0x6			// #UD
	jmp	trap0
	push	$0x7			// #NM
	jmp	trap0
	push	$0x8			// #DF
	jmp	trap
	push	$0x9			//
	jmp	trap0
	push	$0xa			// #TS
	jmp	trap
	push	$0xb			// #NP
	jmp	trap
	push	$0xc			// #SS
	jmp	trap
	push	$0xd			// #GP
	jmp	trap
	push	$0xe			// #PF
	jmp	trap
intx10:
	push	$0x10			// #MF
	jmp	trap0

// Hardware interrupt jump table

intx20:
	push	$0x20			// IRQ0
	jmp	call
	push	$0x21			// IRQ1
	jmp	call
	push	$0x22			// IRQ2
	jmp	call
	push	$0x23			// IRQ3
	jmp	call
	push	$0x24			// IRQ4
	jmp	call
	push	$0x25			// IRQ5
	jmp	call
	push	$0x26			// IRQ6
	jmp	call
	push	$0x27			// IRQ7
	jmp	call
	push	$0x28			// IRQ8
	jmp	call
	push	$0x29			// IRQ9
	jmp	call
	push	$0x2a			// IRQ10
	jmp	call
	push	$0x2b			// IRQ11
	jmp	call
	push	$0x2c			// IRQ12
	jmp	call
	push	$0x2d			// IRQ13
	jmp	call
	push	$0x2e			// IRQ14
	jmp	call
	push	$0x2f			// IRQ15
	jmp	call

intx30:
	push	$0x30			// System call

trap0:
	pushl	(esp)
	movb	$0,4(esp)

trap:
	ss
	cmpl	$0,nofault
	je	call1
	add	$0xc,esp
	ss
	pushl	nofault
	iret

call:
	pushl	(esp)
	movb	$0,4(esp)

call1:
	push	es
	push	ds
	push	eax
	push	ecx
	push	edx

	push	ebx
	push	esi
	push	edi

	mov	$KDATA,eax
	mov	eax,es
	mov	eax,ds

	mov	0x20(esp),edx
	mov	$0x10,ecx
	mov	edx,eax
	sub	$0x20,al
	jb	2f
	cmp	ecx,eax
	jae	2f
	mov	eax,ecx

	cmp	$8,al
	mov	$0x20,al
	jb	1f
	out	al,$0xa0
1:
	out	al,$0x20

	bt	ecx,pl
	jc	2f
	bts	ecx,iq
	jmp	3f

2:
	pushl	pl
	mov	_ipltbl(,ecx,4),eax
	mov	eax,pl
	sti
	push	edx
	call	*_inttbl(,ecx,4)

	cmpb	$KCODE,0x34(esp)
	je	1f
	cmpb	$0,_runrun
	je	1f
	movb	$0xf,(esp)
	call	_trap

1:
	pop	edx
	popl	pl

	call	unqint

3:
	pop	edi
	pop	esi
	pop	ebx

	pop	edx
	pop	ecx
	pop	eax
	pop	ds
	pop	es
	add	$0x8,esp
	iret

.globl	_spl0, _spl1, _spl4, _spl5, _spl6, _spl7, _splx
_spl0:
	mov	$IPL0,edx
	jmp	splx

_spl1:
	mov	$IPL1,edx
	jmp	splx

_spl4:
	mov	$IPL4,edx
	jmp	splx

_spl5:
	mov	$IPL5,edx
	jmp	splx

_spl6:
	mov	$IPL6,edx
	jmp	splx

_spl7:
	mov	$IPL7,edx
	jmp	splx

_splx:
	mov	4(esp),edx

splx:
	mov	pl,eax
	mov	edx,pl

unqint:
	pushf
	cli
	mov	iq,ecx
	and	pl,ecx
	jz	1f
	bsf	ecx,ecx
	btr	ecx,iq
	lea	intx20(,ecx,4),ecx
	push	cs
	call	*ecx
	jmp	unqint
1:
	popf
	ret

.globl	_savfp
_savfp:
	mov	0x4(esp),eax
	fnsave	(eax)
	ret

.globl	_restfp
_restfp:
	mov	0x4(esp),eax
	frstor	(eax)
	ret

.globl	_stst
_stst:
	mov	0x4(esp),eax
	fnstcw	(eax)
	ret

.globl	_addupc
_addupc:
	mov	8(esp),ebx
	mov	4(esp),eax
	sub	8(ebx),eax
	shr	$1,eax
	mov	12(ebx),edx
	shr	$1,edx
	mul	edx
	shrd	$14,edx,eax
	inc	eax
	and	$0xfffffffe,eax
	cmp	4(ebx),eax
	jae	1f
	add	(ebx),eax
	movw	12(esp),dx
	push	nofault
	movl	$2f,nofault
	addw	dx,(eax)
	jmp	3f
2:
	movl	$0,12(ebx)
3:
	pop	nofault
1:
	ret

.globl	_fubyte
_fubyte:
	mov	0x4(esp),edx
	mov	$0x1,ecx
	call	bwfsu
	xor	eax,eax
	movb	(edx),al
	jmp	2f

.globl	_fuword
_fuword:
	mov	0x4(esp),edx
	mov	$0x4,ecx
	call	bwfsu
	mov	(edx),eax
	jmp	2f

.globl	_subyte
_subyte:
	mov	0x4(esp),edx
	mov	$0x1,ecx
	call	bwssu
	movb	0x10(esp),al
	movb	al,(edx)
	jmp	1f

.globl	_suword
_suword:
	mov	0x4(esp),edx
	mov	$0x4,ecx
	call	bwssu
	mov	0x10(esp),eax
	mov	eax,(edx)

1:
	xor	eax,eax
2:
	popl	nofault
	popf
	ret

3:
	mov	$-1,eax
	jmp	2b

bwssu:
	call	ckw

bwfsu:
	call	ckr
	pop	eax
	pushf
	cli
	pushl	nofault
	movl	$3b,nofault
	jmp	*eax

.globl	_copyin
_copyin:
	mov	0x4(esp),edx
	call	cpisu
	mov	0x10(esp),ebx
1:
	mov	(edx),eax
	add	$4,edx
	mov	eax,(ebx)
	add	$4,ebx
	loop	1b
	jmp	2f

.globl	_copyout
_copyout:
	mov	0x8(esp),edx
	call	cposu
	mov	0xc(esp),ebx
1:
	mov	(ebx),eax
	add	$4,ebx
	mov	eax,(edx)
	add	$4,edx
	loop	1b

2:
	xor	eax,eax
3:
	popl	nofault
	pop	ebx
	ret

4:
	mov	$-1,eax
	jmp	3b

cposu:
	call	ckw

cpisu:
	mov	0x10(esp),ecx
	call	ckr
	shr	$2,ecx
	pop	eax
	push	ebx
	pushl	nofault
	movl	$4b,nofault
	jmp	*eax

ckw:
	cmpl	$RW,UTAB+[3*4]
	je	1f
	mov	UTAB,eax
	shl	$PGSH,eax
	cmp	eax,edx
	jb	2f
1:
	ret

ckr:
	mov	$0x400000,eax
	sub	edx,eax
	jb	2f
	cmp	ecx,eax
	jb	2f
	ret

2:
	add	$0x8,esp
	mov	$-1,eax
	ret

.globl	_idle
_idle:
	pushf
	push	pl
	movl	$IPL0,pl
	sti
	hlt
waitloc:
	pop	pl
	popf
	ret

.globl	 _save
_save:
	pop	edx
	mov	(esp),eax
	mov	ebx,(eax)
	mov	esp,0x4(eax)
	mov	ebp,0x8(eax)
	mov	esi,0xc(eax)
	mov	edi,0x10(eax)
	mov	edx,0x14(eax)
	xor	eax,eax
	jmp	*edx

.globl	_resume
_resume:
	mov	0x4(esp),eax
	mov	0x8(esp),edx
	shl	$0xc,eax
	movb	$0x7,al
	cli
	mov	eax,kptu
	add	$PGSZ,eax
	mov	eax,kptk
	mov	cr3,eax
	mov	eax,cr3
	mov	(edx),ebx
	mov	0x4(edx),esp
	mov	0x8(edx),ebp
	mov	0xc(edx),esi
	mov	0x10(edx),edi
	sti
	mov	$0x1,eax
	jmp	*0x14(edx)

.globl	_bcopy
_bcopy:
	push	esi
	push	edi
	mov	0x0c(esp),esi
	mov	0x10(esp),edi
	mov	0x14(esp),ecx
	cld
	shr	$0x2,ecx
	rep; movsl
	mov	0x14(esp),ecx
	and	$0x3,ecx
	rep; movsb
	pop	edi
	pop	esi
	ret

.globl	_bzero
_bzero:
	push	edi
	mov	0x8(esp),edi
	mov	0xc(esp),ecx
	xor	eax,eax
	cld
	shr	$0x2,ecx
	rep; stosl
	mov	0xc(esp),ecx
	and	$0x3,ecx
	rep; stosb
	pop	edi
	ret

.globl	_inb
_inb:
	mov	0x4(esp),edx
	xor	eax,eax
	inb	dx,al
	ret

.globl	_outb
_outb:
	mov	0x4(esp),edx
	movb	0x8(esp),al
	outb	al,dx
	ret

.globl	_insw
_insw:
	push	edi
	mov	0x08(esp),edx
	mov	0x0c(esp),edi
	mov	0x10(esp),ecx
	cld
	rep; insw
	pop	edi
	ret

.globl	_outsw
_outsw:
	push	esi
	mov	0x08(esp),edx
	mov	0x0c(esp),esi
	mov	0x10(esp),ecx
	cld
	rep; outsw
	pop	esi
	ret

.globl	_ld_cr0
_ld_cr0:
	mov	cr0,eax
	ret

.globl	_ld_cr2
_ld_cr2:
	mov	cr2,eax
	ret

.globl	_ld_cr3
_ld_cr3:
	mov	cr3,eax
	ret

.globl	_invd
_invd:
	mov	cr3,eax
	mov	eax,cr3
	ret

.globl	_cli
_cli:
	cli
	ret

.globl	_sti
_sti:
	sti
	ret

// Constants

// IDT construction control string

.align	8
idtctl: .long 0xfffe8e00, intx00	// int 0x00-0x0f
	.long 0x80008e00, intx10	// int 0x10
	.long 0xffff8e00, intx20	// int 0x20-0x2f
	.long 0x8000ee00, intx30	// int 0x30
	.long 0

.data

_inttbl:.long _clock			// irq0 timer
	.long _scrint			// irq1 keyboard
	.long _stray			// irq2
	.long _srintr			// irq3 serial port 1
	.long _srintr			// irq4 serial port 0
	.long _stray			// irq5
	.long _fdintr			// irq6 floppy drive
	.long _stray			// irq7
	.long _stray			// irq8
	.long _stray			// irq9
	.long _stray			// irq10
	.long _stray			// irq11
	.long _stray			// irq12
	.long _stray			// irq13
	.long _hdintr			// irq14 hard drive
	.long _cdintr			// irq15 cdrom drive
	.long _trap			// trap

_ipltbl:.long IPL6			// irq0 timer
	.long IPL4			// irq1 keyboard
	.long 0 			// irq2
	.long IPL4			// irq3 serial port 1
	.long IPL4			// irq4 serial port 0
	.long 0 			// irq5
	.long IPL5			// irq6 floppy drive
	.long 0 			// irq7
	.long 0 			// irq8
	.long 0 			// irq9
	.long 0 			// irq10
	.long 0 			// irq11
	.long 0 			// irq12
	.long 0 			// irq13
	.long IPL5			// irq14 hard drive
	.long IPL5			// irq15 cdrom drive
	.long IPL0			// trap

.align	16
gdt:	.word 0x0,0x0,0x0,0x0
	.word 0x0067,tss,0xe9c0,0x7f40	// TSS
	.word 0xffff,0x0,0x9a00,0xcf	// KCODE
	.word 0xffff,0x0,0x9200,0xcf	// KDATA
	.word 0xffff,0x0,0xfa00,0xcf	// UCODE
	.word 0xffff,0x0,0xf200,0xcf	// UDATA
gdt1:

pl:	.long IPL7
iq:	.long 0

.globl	_waitloc
_waitloc:.long waitloc

ivt:	.space 0x188
ivt1:

tss:	.space 0x68
tss1:

.align	8
gdtd:	.word gdt1-gdt-1
	.long gdt-KA

.align	8
gdtd2:	.word gdt1-gdt-1
	.long gdt

.align	8
idtd:	.word ivt1-ivt-1
	.long ivt

nofault:.long 0

.globl	_kend
_kend:	.long 0

.globl	_phymem
_phymem:.long 0

.globl	_u
.set	_u,U

.globl	_mem, _pdir, _upt
.set	_mem,0x40000000
.set	_pdir,0x7ff9a000
.set	_upt,0x7ff9b000

.set	kpt,0x7ff9c000
.set	kptu,kpt+[925*4]
.set	kptk,kpt+[927*4]
.set	kstk,0x7ffa0000