/*
 * __exec.h
 *
 *  Created on: 31 Jan 2020
 *      Author: marti
 */

#ifndef SYS_TEST_EXEC___EXEC_H_
#define SYS_TEST_EXEC___EXEC_H_

struct proc;
struct exec_linker;
struct vnode;

typedef int (*exec_makecmds_fcn)(struct proc *, struct exec_linker *);

struct execsw {
	int 				(*ex_exec_linker)(void * /* struct exec_linker* */);
	const char 			*ex_name;

	u_int				ex_hdrsz;		/* size of header for this format */
	int					ex_arglen;		/* Extra argument size in words */
										/* Copy arguments on the new stack */
	exec_makecmds_fcn	ex_makecmds;	/* function to setup vmcmds */
	int					ex_prio;		/* entry priority */
	void				(*ex_setregs)(struct proc *, struct exec_linker *, u_long);
	int					(*ex_setup_stack)(struct exec_linker *);

	union {
		int (*elf_probe_func)(struct proc *, struct exec_linker *, void *, char *, caddr_t *);
		int (*ecoff_probe_func)(struct proc *, struct exec_linker *);
	} u;

};

#define EXECSW_PRIO_ANY		0x000	/* default, no preference */
#define EXECSW_PRIO_FIRST	0x001	/* this should be among first */
#define EXECSW_PRIO_LAST	0x002	/* this should be among last */


#endif /* SYS_TEST_EXEC___EXEC_H_ */
