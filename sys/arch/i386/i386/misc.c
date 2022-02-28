/*-
 * Copyright (c) 1982, 1987, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)Original from machdep.c	8.3 (Berkeley) 5/9/95
 *
 */

#include <sys/systm.h>
#include <sys/errno.h>
#include <vm/include/vm_param.h>

#include <machine/cpufunc.h>
#include <machine/param.h>
#include <machine/pmap.h>
#include <machine/pte.h>
#include <machine/vmparam.h>

extern pt_entry_t		*CMAP1, *CMAP2;
extern caddr_t			CADDR1, CADDR2;

/*
 * zero out physical memory
 * specified in relocation units (NBPG bytes)
 */
void
clearseg(n)
	int n;
{
	*(int*) CMAP2 = PG_V | PG_KW | ctob(n);
	lcr3(rcr3());
	bzero(CADDR2, NBPG);
	*(int*) CADDR2 = 0;
}

/*
 * copy a page of physical memory
 * specified in relocation units (NBPG bytes)
 */
void
copyseg(frm, n)
	int frm, n;
{
	*(int*) CMAP2 = PG_V | PG_KW | ctob(n);
	lcr3(rcr3());
	bcopy((void*) frm, (void*) CADDR2, NBPG);
}

/*
 * copy a page of physical memory
 * specified in relocation units (NBPG bytes)
 */
void
physcopyseg(frm, to)
	int frm, to;
{
	*(int*) CMAP1 = PG_V | PG_KW | ctob(frm);
	*(int*) CMAP2 = PG_V | PG_KW | ctob(to);
	lcr3(rcr3());
	bcopy(CADDR1, CADDR2, NBPG);
}

/*
 * insert an element into a queue
 */
#undef insque
_insque(element, head)
	register struct prochd *element, *head;
{
	element->ph_link = head->ph_link;
	head->ph_link = (struct proc*) element;
	element->ph_rlink = (struct proc*) head;
	((struct prochd*) (element->ph_link))->ph_rlink = (struct proc*) element;
}

/*
 * remove an element from a queue
 */
#undef remque
_remque(element)
	register struct prochd *element;
{
	((struct prochd*) (element->ph_link))->ph_rlink = element->ph_rlink;
	((struct prochd*) (element->ph_rlink))->ph_link = element->ph_link;
	element->ph_rlink = (struct proc*) 0;
}


vmunaccess()
{

}

int
copyout(from, to, len)
	const void *from;
	void *to;
	size_t len;
{
	int *pte, *pde;
	int rest_of_page;
	int thistime;
	int err;

	/* be very careful not to overflow doing this check */
	if (to >= (void *)USRSTACK || (void *)USRSTACK - to < len) {
		return (EFAULT);
	}

	pte = (int *)vtopte(to);
	pde = (int *)vtopte(pte);

	rest_of_page = PAGE_SIZE - ((int)to & (PAGE_SIZE - 1));

	while (1) {
		thistime = len;
		if (thistime > rest_of_page) {
			thistime = rest_of_page;
		}

		if ((*pde & PG_V) == 0 || (*pte & (PG_V | PG_UW)) != (PG_V | PG_UW)) {
			if (err == user_write_fault(to)) {
				return (err);
			}
		}

		bcopy(from, to, thistime);

		len -= thistime;

		/*
		 * Break out as soon as possible in the common case
		 * that the whole transfer is containted in one page.
		 */
		if (len == 0) {
			break;
		}

		from += thistime;
		to += thistime;
		pte++;
		pde = (u_int *)vtopte(pte);
		rest_of_page = PAGE_SIZE;
	}

	return (0);
}

/*
 * Below written in C to allow access to debugging code
 */
int
copyinstr(fromaddr, toaddr, maxlength, lencopied)
	const void *fromaddr;
	void *toaddr;
	size_t *lencopied, maxlength;
{
	int c, tally;

	tally = 0;
	while (maxlength--) {
		c = fubyte(fromaddr++);
		if (c == -1) {
			if (lencopied)
				*lencopied = tally;
			return (EFAULT);
		}
		tally++;
		*(char*) toaddr++ = (char) c;
		if (c == 0) {
			if (lencopied)
				*lencopied = tally;
			return (0);
		}
	}
	if (lencopied)
		*lencopied = tally;
	return (ENAMETOOLONG);
}

int
copyoutstr(fromaddr, toaddr, maxlength, lencopied)
	const void *fromaddr;
	void *toaddr;
	size_t *lencopied, maxlength;
{
	int c, tally;

	tally = 0;
	while (maxlength--) {
		c = subyte(toaddr++, *(char*) fromaddr);
		if (c == -1)
			return (EFAULT);
		tally++;
		if (*(char*) fromaddr++ == 0) {
			if (lencopied)
				*lencopied = tally;
			return (0);
		}
	}
	if (lencopied)
		*lencopied = tally;
	return (ENAMETOOLONG);
}

int
copystr(fromaddr, toaddr, maxlength, lencopied)
	const void *fromaddr;
	void *toaddr;
	size_t *lencopied, maxlength;
{
	u_int tally;

	tally = 0;
	while (maxlength--) {
		*(u_char*) toaddr = *(u_char*) fromaddr++;
		tally++;
		if (*(u_char*) toaddr++ == 0) {
			if (lencopied)
				*lencopied = tally;
			return (0);
		}
	}
	if (lencopied)
		*lencopied = tally;
	return (ENAMETOOLONG);
}
