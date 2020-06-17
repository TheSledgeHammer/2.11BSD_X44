/*	$NetBSD: sysarch.h,v 1.9 1998/02/25 21:27:05 perry Exp $	*/

#ifndef _I386_SYSARCH_H_
#define _I386_SYSARCH_H_

#include <sys/cdefs.h>

#define I386_GET_LDT	0
#define I386_SET_LDT	1
#define	I386_IOPL		2
#define	I386_GET_IOPERM	3
#define	I386_SET_IOPERM	4
#define	I386_VM86		5

/*
 * Architecture specific syscalls (i386)
 */
struct i386_get_ldt_args {
	int start;
	union descriptor *desc;
	int num;
};

struct i386_set_ldt_args {
	int start;
	union descriptor *desc;
	int num;
};

struct i386_iopl_args {
	int iopl;
};

struct i386_get_ioperm_args {
	u_long *iomap;
};

struct i386_set_ioperm_args {
	u_long *iomap;
};

#ifndef _KERNEL
int i386_get_ldt (int, union descriptor *, int);
int i386_set_ldt (int, union descriptor *, int);
int i386_iopl (int);
int i386_get_ioperm (u_long *);
int i386_set_ioperm (u_long *);
int sysarch (int, void *);
#endif

#endif /* !_I386_SYSARCH_H_ */
