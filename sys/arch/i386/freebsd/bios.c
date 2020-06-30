/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1997 Michael Smith
 * Copyright (c) 1998 Jonathan Lemon
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD$"); */

/*
 * Code for dealing with the BIOS in x86 PC systems.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <vm/include/vm.h>
#include <vm/include/pmap.h>

#include <machine/bus.h>
#include <machine/segments.h>
#include <machine/stdarg.h>
#include <machine/vmparam.h>
#include <machine/pc/bios.h>

#ifdef DEV_ISA
#include <dev/isa/isavar.h>
#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>
#endif

#define BIOS_START	0xe0000
#define BIOS_SIZE	0x20000

/* exported lookup results */
struct bios32_SDentry		PCIbios;

static u_int				bios32_SDCI;

/* start fairly early */
static void					bios32_init(); /* init'ed in autoconf.c */

/*
 * bios32_init
 *
 * Locate various bios32 entities.
 */
static void
bios32_init()
{
    u_long					sigaddr;
    struct bios32_SDheader	*sdh;
    struct PnPBIOS_table	*pt;
    u_int8_t				ck, *cv;
    int						i;
    char					*p;

    /*
	 * BIOS32 Service Directory, PCI BIOS
	 */

	/* look for the signature */
	if ((sigaddr = bios_sigsearch(0, "_32_", 4, 16, 0)) != 0) {

		/* get a virtual pointer to the structure */
		sdh = (struct bios32_SDheader*) (uintptr_t) BIOS_PADDRTOVADDR(sigaddr);
		for (cv = (u_int8_t*) sdh, ck = 0, i = 0; i < (sdh->len * 16); i++) {
			ck += cv[i];
		}
		/* If checksum is OK, enable use of the entrypoint */
		if ((ck == 0) && (BIOS_START <= sdh->entry)
				&& (sdh->entry < (BIOS_START + BIOS_SIZE))) {
			bios32_SDCI = BIOS_PADDRTOVADDR(sdh->entry);
			if (bootverbose) {
				printf("bios32: Found BIOS32 Service Directory header at %p\n",
						sdh);
				printf("bios32: Entry = 0x%x (%x)  Rev = %d  Len = %d\n",
						sdh->entry, bios32_SDCI, sdh->revision, sdh->len);
			}

			/* Allow user override of PCI BIOS search */
			if (((p = kern_getenv("machdep.bios.pci")) == NULL)
					|| strcmp(p, "disable")) {

				/* See if there's a PCI BIOS entrypoint here */
				PCIbios.ident.id = 0x49435024; /* PCI systems should have this */
				if (!bios32_SDlookup(&PCIbios) && bootverbose)
					printf("pcibios: PCI BIOS entry at 0x%x+0x%x\n",
							PCIbios.base, PCIbios.entry);
			}
			if (p != NULL)
				freeenv(p);
		} else {
			printf("bios32: Bad BIOS32 Service Directory\n");
		}
	}

    if (p != NULL)
	    freeenv(p);
    if (bootverbose) {
	    /* look for other know signatures */
	    printf("Other BIOS signatures found:\n");
    }
}

/*
 * bios32_SDlookup
 *
 * Query the BIOS32 Service Directory for the service named in (ent),
 * returns nonzero if the lookup fails.  The caller must fill in
 * (ent->ident), the remainder are populated on a successful lookup.
 */
int
bios32_SDlookup(struct bios32_SDentry *ent)
{
    struct bios_regs args;

	if (bios32_SDCI == 0)
		return (1);

	args.eax = ent->ident.id; /* set up arguments */
	args.ebx = args.ecx = args.edx = 0;
	bios32(&args, bios32_SDCI, GSEL(GCODE_SEL, SEL_KPL));
	if ((args.eax & 0xff) == 0) { /* success? */
		ent->base = args.ebx;
		ent->len = args.ecx;
		ent->entry = args.edx;
		ent->ventry = BIOS_PADDRTOVADDR(ent->base + ent->entry);
		return (0); /* all OK */
	}
	return (1); /* failed */
}

/*
 * bios_sigsearch
 *
 * Search some or all of the BIOS region for a signature string.
 *
 * (start)	Optional offset returned from this function
 *		(for searching for multiple matches), or NULL
 *		to start the search from the base of the BIOS.
 *		Note that this will be a _physical_ address in
 *		the range 0xe0000 - 0xfffff.
 * (sig)	is a pointer to the byte(s) of the signature.
 * (siglen)	number of bytes in the signature.
 * (paralen)	signature paragraph (alignment) size.
 * (sigofs)	offset of the signature within the paragraph.
 *
 * Returns the _physical_ address of the found signature, 0 if the
 * signature was not found.
 */

u_int32_t
bios_sigsearch(u_int32_t start, u_char *sig, int siglen, int paralen, int sigofs)
{
    u_char	*sp, *end;

    /* compute the starting address */
	if ((start >= BIOS_START) && (start <= (BIOS_START + BIOS_SIZE))) {
		sp = (char*) BIOS_PADDRTOVADDR(start);
	} else if (start == 0) {
		sp = (char*) BIOS_PADDRTOVADDR(BIOS_START);
	} else {
		return 0; /* bogus start address */
	}

	/* compute the end address */
	end = (u_char*) BIOS_PADDRTOVADDR(BIOS_START + BIOS_SIZE);

	/* loop searching */
	while ((sp + sigofs + siglen) < end) {

		/* compare here */
		if (!bcmp(sp + sigofs, sig, siglen)) {
			/* convert back to physical address */
			return ((u_int32_t) BIOS_VADDRTOPADDR(sp));
		}
		sp += paralen;
	}
	return (0);
}

/*
 * do not staticize, used by bioscall.s
 */
union {
	struct {
		u_short offset;
		u_short segment;
	} vec16;
	struct {
		u_int 	offset;
		u_short segment;
	} vec32;
} bioscall_vector; /* bios jump vector */

void
set_bios_selectors(struct bios_segments *seg, int flags)
{
    struct soft_segment_descriptor ssd = {
    		0, 				/* segment base address (overwritten) */
			0, 				/* length (overwritten) */
			SDT_MEMERA, 	/* segment type (overwritten) */
			0, 				/* priority level */
			1, 				/* descriptor present */
			0, 0, 1, 		/* descriptor size (overwritten) */
			0 				/* granularity == byte units */
	};
    union descriptor *p_gdt;

#ifdef SMP
    p_gdt = &gdt[PCPU_GET(cpuid) * NGDT];
#else
    p_gdt = gdt;
#endif

	ssd.ssd_base = seg->code32.base;
	ssd.ssd_limit = seg->code32.limit;
	ssdtosd(&ssd, &p_gdt[GBIOSCODE32_SEL].sd);

	ssd.ssd_def32 = 0;
	if (flags & BIOSCODE_FLAG) {
		ssd.ssd_base = seg->code16.base;
		ssd.ssd_limit = seg->code16.limit;
		ssdtosd(&ssd, &p_gdt[GBIOSCODE16_SEL].sd);
	}

	ssd.ssd_type = SDT_MEMRWA;
	if (flags & BIOSDATA_FLAG) {
		ssd.ssd_base = seg->data.base;
		ssd.ssd_limit = seg->data.limit;
		ssdtosd(&ssd, &p_gdt[GBIOSDATA_SEL].sd);
	}

	if (flags & BIOSUTIL_FLAG) {
		ssd.ssd_base = seg->util.base;
		ssd.ssd_limit = seg->util.limit;
		ssdtosd(&ssd, &p_gdt[GBIOSUTIL_SEL].sd);
	}

	if (flags & BIOSARGS_FLAG) {
		ssd.ssd_base = seg->args.base;
		ssd.ssd_limit = seg->args.limit;
		ssdtosd(&ssd, &p_gdt[GBIOSARGS_SEL].sd);
	}
}

extern void bios16_jmp(void);

/*
 * this routine is really greedy with selectors, and uses 5:
 *
 * 32-bit code selector:	to return to kernel
 * 16-bit code selector:	for running code
 *        data selector:	for 16-bit data
 *        util selector:	extra utility selector
 *        args selector:	to handle pointers
 *
 * the util selector is set from the util16 entry in bios16_args, if a
 * "U" specifier is seen.
 *
 * See <machine/pc/bios.h> for description of format specifiers
 */
int
bios16(struct bios_args *args, char *fmt, ...)
{
    char		*p, *stack, *stack_top;
    va_list 	ap;
    int 		flags = BIOSCODE_FLAG | BIOSDATA_FLAG;
    u_int 		i, arg_start, arg_end;
    void		*bios16_pmap_handle;
    arg_start = 0xffffffff;
    arg_end = 0;

	/*
	 * Some BIOS entrypoints attempt to copy the largest-case
	 * argument frame (in order to generalise handling for
	 * different entry types).  If our argument frame is
	 * smaller than this, the BIOS will reach off the top of
	 * our constructed stack segment.  Pad the top of the stack
	 * with some garbage to avoid this.
	 */
	stack = (caddr_t) PAGE_SIZE - 32;

	va_start(ap, fmt);
	for (p = fmt; p && *p; p++) {
		switch (*p) {
		case 'p': /* 32-bit pointer */
			i = va_arg(ap, u_int);
			arg_start = min(arg_start, i);
			arg_end = max(arg_end, i);
			flags |= BIOSARGS_FLAG;
			stack -= 4;
			break;

		case 'i': /* 32-bit integer */
			i = va_arg(ap, u_int);
			stack -= 4;
			break;

		case 'U': /* 16-bit selector */
			flags |= BIOSUTIL_FLAG;
			/* FALLTHROUGH */
		case 'D': /* 16-bit selector */
		case 'C': /* 16-bit selector */
			stack -= 2;
			break;

		case 's': /* 16-bit integer passed as an int */
			i = va_arg(ap, int);
			stack -= 2;
			break;

		default:
			va_end(ap);
			return (EINVAL);
		}
	}
	va_end(ap);

	if (flags & BIOSARGS_FLAG) {
		if (arg_end - arg_start > ctob(16))
			return (EACCES);
		args->seg.args.base = arg_start;
		args->seg.args.limit = 0xffff;
	}

	args->seg.code32.base = pmap_pg_frame((u_int) &bios16_jmp);
	args->seg.code32.limit = 0xffff;

	bios16_pmap_handle = pmap_bios16_enter();

	stack_top = stack;
	va_start(ap, fmt);
	for (p = fmt; p && *p; p++) {
		switch (*p) {
		case 'p': /* 32-bit pointer */
			i = va_arg(ap, u_int);
			*(u_int*) stack = (i - arg_start)
					| (GSEL(GBIOSARGS_SEL, SEL_KPL) << 16);
			stack += 4;
			break;

		case 'i': /* 32-bit integer */
			i = va_arg(ap, u_int);
			*(u_int*) stack = i;
			stack += 4;
			break;

		case 'U': /* 16-bit selector */
			*(u_short*) stack = GSEL(GBIOSUTIL_SEL, SEL_KPL);
			stack += 2;
			break;

		case 'D': /* 16-bit selector */
			*(u_short*) stack = GSEL(GBIOSDATA_SEL, SEL_KPL);
			stack += 2;
			break;

		case 'C': /* 16-bit selector */
			*(u_short*) stack = GSEL(GBIOSCODE16_SEL, SEL_KPL);
			stack += 2;
			break;

		case 's': /* 16-bit integer passed as an int */
			i = va_arg(ap, int);
			*(u_short*) stack = i;
			stack += 2;
			break;

		default:
			va_end(ap);
			return (EINVAL);
		}
	}
	va_end(ap);

	set_bios_selectors(&args->seg, flags);
	bioscall_vector.vec16.offset = (u_short) args->entry;
	bioscall_vector.vec16.segment = GSEL(GBIOSCODE16_SEL, SEL_KPL);

	i = bios16_call(&args->r, stack_top);
	pmap_bios16_leave(bios16_pmap_handle);
	return (i);
}

int
bios_oem_strings(struct bios_oem *oem, u_char *buffer, size_t maxlen)
{
	size_t idx = 0;
	struct bios_oem_signature *sig;
	u_int from, to;
	u_char c, *s, *se, *str, *bios_str;
	size_t i, off, len, tot;

	if (!oem || !buffer || maxlen < 2)
		return (-1);

	sig = oem->signature;
	if (!sig)
		return (-2);

	from = oem->range.from;
	to = oem->range.to;
	if ((to <= from) || (from < BIOS_START) || (to > (BIOS_START + BIOS_SIZE)))
		return (-3);

	while (sig->anchor != NULL) {
		str = sig->anchor;
		len = strlen(str);
		off = sig->offset;
		tot = sig->totlen;
		/* make sure offset doesn't go beyond bios area */
		if ((to + off) > (BIOS_START + BIOS_SIZE)
				|| ((from + off) < BIOS_START)) {
			printf("sys/i386/i386/bios.c: sig '%s' "
					"from 0x%0x to 0x%0x offset %d "
					"out of BIOS bounds 0x%0x - 0x%0x\n", str, from, to, off,
			BIOS_START, BIOS_START + BIOS_SIZE);
			return (-4);
		}
		/* make sure we don't overrun return buffer */
		if (idx + tot > maxlen - 1) {
			printf("sys/i386/i386/bios.c: sig '%s' "
					"idx %d + tot %d = %d > maxlen-1 %d\n", str, idx, tot,
					idx + tot, maxlen - 1);
			return (-5);
		}
		bios_str = NULL;
		s = (u_char*) BIOS_PADDRTOVADDR(from);
		se = (u_char*) BIOS_PADDRTOVADDR(to - len);
		for (; s < se; s++) {
			if (!bcmp(str, s, len)) {
				bios_str = s;
				break;
			}
		}
		/*
		 *  store pretty version of totlen bytes of bios string with
		 *  given offset; 0x20 - 0x7E are printable; uniquify spaces
		 */
		if (bios_str) {
			for (i = 0; i < tot; i++) {
				c = bios_str[i + off];
				if ((c < 0x20) || (c > 0x7E))
					c = ' ';
				if (idx == 0) {
					if (c != ' ')
						buffer[idx++] = c;
				} else if ((c != ' ')
						|| ((c == ' ') && (buffer[idx - 1] != ' ')))
					buffer[idx++] = c;
			}
		}
		sig++;
	}
	/* remove a final trailing space */
	if ((idx > 1) && (buffer[idx - 1] == ' '))
		idx--;
	buffer[idx] = '\0';
	return (idx);
}
