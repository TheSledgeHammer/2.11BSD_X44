/*	$NetBSD: sysarch.h,v 1.9 1998/02/25 21:27:05 perry Exp $	*/

#ifndef _I386_SYSARCH_H_
#define _I386_SYSARCH_H_

#include <sys/cdefs.h>

#define I386_GET_LDT		0
#define I386_SET_LDT		1
#define	I386_IOPL			2
#define	I386_GET_IOPERM		3
#define	I386_SET_IOPERM		4
#define	I386_VM86			5
#define	I386_GET_GSBASE		6
#define	I386_GET_FSBASE		7
#define	I386_SET_GSBASE		8
#define	I386_SET_FSBASE		9

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
int i386_get_ldt (struct proc *, char *, register_t *);
int i386_set_ldt (struct proc *, char *, register_t *);
int i386_iopl (struct proc *, char *, register_t *);
int i386_get_ioperm (struct proc *, char *, register_t *);
int i386_set_ioperm (struct proc *, char *, register_t *);
int i386_get_fsbase(struct proc *, char *, register_t *);
int i386_set_fsbase(struct proc *, char *, register_t *);
int i386_get_gsbase(struct proc *, char *, char);
int i386_set_gsbase(struct proc *, char *, char);
int sysarch (struct proc *, struct sysarch_args *, register_t *);
#endif

#endif /* !_I386_SYSARCH_H_ */
