/*
 * asm.h
 *
 *  Created on: 3 Mar 2020
 *      Author: marti
 */

#ifndef _MACHINE_ASM_H_
#define	_MACHINE_ASM_H_

#include <sys/cdefs.h>

#define	ENTRY(name) 	\
	.globl _/**/name; _/**/name:
#define	ALTENTRY(name) 	\
	.globl _/**/name; _/**/name:

#endif /* _MACHINE_ASM_H_ */
