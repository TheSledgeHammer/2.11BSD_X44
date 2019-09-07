/* UNIX V7 source code: see /COPYRIGHT or www.tuhs.org for details. */
/* Changes: Copyright (c) 1999 Robert Nordier. All rights reserved. */

#include "param.h"
#include "../machine/seg.h"
#include "../machine/reg.h"

#include "systm.h"
#include "acct.h"
#include "dir.h"
#include "user.h"
#include "inode.h"
#include "proc.h"
#include "map.h"
#include "buf.h"


extern int kend, phymem;

/*
 * Icode is the hex bootstrap
 * program executed in user mode
 * to bring up the system.
 */
int icode[] =
{
	0x186a106a,	/* push $initp; push $init */
	0xbb8006a,	/* push $0; mov $11,eax */
	0xcd000000,	/* int $0x30 */
	0xfeeb30,	/* jmp . */
	0x18,		/* initp: init; 0 */
	0x0,
	0x6374652f,	/* init: </etc/init\0> */
	0x696e692f,
	0x74
};
int	szicode = sizeof(icode);

/*
 * Machine-dependent startup code
 */
startup()
{
	register i;
	int j;

        outb(0x3f2, 0xc); /* XXX turn floppy drive motor off */

	/*
	 * free all of core
	 */
	j = btoc(kend) + 9;
	for (i = 1; i < phymem; i++) {
		if ((i >= 16 && i < j) || (i >= 160 && i < 256))
			continue;
		maxmem++;
		mfree(coremap, 1, i);
	}
	printf("mem = %D\n", ctob((long)maxmem));
	if(MAXMEM < maxmem)
		maxmem = MAXMEM;
	mfree(swapmap, nswap, 1);
	swplo--;
}

/*
 * set up a physical address
 * into users virtual address space.
 */
sysphys()
{
	if(!suser())
		return;
	u.u_error = EINVAL;
}

/*
 * Start the clock.
 */
clkstart()
{
	unsigned c = (0x1234dd + HZ / 2) / HZ;

	readrtc();
	/* 8253 pit counter 0 mode 3 */
	cli();
	outb(0x43, 0x36);
	outb(0x40, c);
	outb(0x40, c >> 8);
	sti();
	spl0();
}

/*
 * Let a process handle a signal by simulating a call
 */
sendsig(p, signo)
caddr_t p;
{
	register unsigned n;

	n = u.u_ar0[ESP] - 4;
	grow(n);
	suword((caddr_t)n, u.u_ar0[EIP]);
	u.u_ar0[ESP] = n;
	u.u_ar0[EFL] &= ~TBIT;
	u.u_ar0[EIP] = (int)p;
}

/*
 * Set the time from the real time clock.
 */
readrtc()
{
        static short days[] = {
    		0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};
	int yr, mt, dm, hr, mn, sc;

	if ((inrtc(0xd) & 0x80) == 0)
		return;
	while (inrtc(0xa) & 0x80)
		;
	sc = getrtc(0);
	mn = getrtc(2);
	hr = getrtc(4);
	dm = getrtc(7);
	mt = getrtc(8);
	yr = getrtc(9);
	yr += yr >= 70 ? -70 : 30;
	time =	(((yr * 365 + (yr + 1) / 4 +
		days[mt - 1] + ((yr + 2) % 4 == 0 && mt >= 3) +
		dm - 1) * 24 + hr) * 60 + mn) * 60 + sc;
	time += TIMEZONE * 60;
}

int
getrtc(addr)
{
	int x;

	x = inrtc(addr);
	return (x >> 4) * 10 + (x & 15);
}

int
inrtc(addr)
{
	outb(0x70, addr);
	return inb(0x71);
}
