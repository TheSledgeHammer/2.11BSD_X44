/*
 * ksyms.h
 *
 *  Created on: 4 Jun 2020
 *      Author: marti
 */

#ifndef _SYS_KSYMS_H_
#define _SYS_KSYMS_H_

/*
 * Do a lookup of a symbol using the in-kernel lookup algorithm.
 */
struct ksyms_gsymbol {
	const char *kg_name;
	union {
		void 			*ku_sym;		 /* Normally Elf_Sym */
		unsigned long 	*ku_value;
	} _un;
#define	kg_sym 		_un.ku_sym
#define	kg_value 	_un.ku_value
};

#define	KIOCGSYMBOL	_IOW('l', 1, struct ksyms_gsymbol)
#define	KIOCGVALUE	_IOW('l', 2, struct ksyms_gsymbol)
#define	KIOCGSIZE	_IOR('l', 3, int)

#ifdef _KERNEL
/*
 * Definitions used in ksyms_getname() and ksyms_getval().
 */
#define	KSYMS_CLOSEST	0001	/* Nearest lower match */
#define	KSYMS_EXACT		0002	/* Only exact match allowed */
#define KSYMS_EXTERN	0000	/* Only external symbols (pseudo) */
#define KSYMS_PROC		0100	/* Procedures only */
#define KSYMS_ANY		0200	/* Also local symbols (DDB use only) */

/*
 * Prototypes
 */
int 	ksyms_getname(const char **, char **, caddr_t, int);
int 	ksyms_getval(const char *, char *, unsigned long *, int, int);
#define	ksyms_getval_from_kernel(a,b,c,d)	ksyms_getval(a,b,c,d,0)
#define	ksyms_getval_from_userland(a,b,c,d)	ksyms_getval(a,b,c,d,1)
int 	ksyms_addsymtab(const char *, void *, size_t, char *, size_t);
int 	ksyms_delsymtab(const char *);
int 	ksyms_rensymtab(const char *, const char*);
void 	ksyms_init(int, void *, void *);
#ifdef DDB
int 	ksyms_sift(char *, char *, int);
#endif
#endif /* _KERNEL */
#endif /* _SYS_KSYMS_H_ */
