/* UNIX V7 source code: see /COPYRIGHT or www.tuhs.org for details. */
/* Changes: Copyright (c) 1999 Robert Nordier. All rights reserved. */

/*
 * Location of the users' stored
 * registers relative to R0 (EAX for the x86).
 * Usage is u.u_ar0[XX].
 */
#define	EAX	(0)
#define	ECX	(-1)
#define	EDX	(-2)
#define	EBX	(-3)
#define	ESP	(8)
#define	EBP	(-9)
#define	ESI	(-4)
#define	EDI	(-5)
#define	EIP	(5)
#define	EFL	(7)

#define	TBIT	0x100		/* EFLAGS trap flag */

struct trap {
    int dev;
    int pl;
    int edi;
    int esi;
    int ebx;
    int edx;
    int ecx;
    int eax;
    int ds;
    int es;
    int num;
    int err;
    int eip;
    int cs;
    int efl;
    int esp;
    int ss;
};