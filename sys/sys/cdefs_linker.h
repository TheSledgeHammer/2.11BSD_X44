/*-
 * Copyright (c) 1999 John D. Polstra
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
 *
 * $FreeBSD: src/sys/sys/linker_set.h,v 1.4.2.1 2000/08/02 21:52:20 peter Exp $
 */

#ifndef _SYS_CDEFS_LINKER_H_
#define _SYS_CDEFS_LINKER_H_

#include <sys/cdefs_elf.h>

/*
 * GLOBL macro exists to preserve __start_set_* and __stop_set_* sections
 * of kernel modules which are discarded from binutils 2.17.50+ otherwise.
 */
#define	__GLOBL1(sym)	__asm__(".globl " #sym)
#define	__GLOBL(sym)	__GLOBL1(sym)

/*
 * The following macros are used to declare global sets of objects, which
 * are collected by the linker into a `struct linker_set' as defined below.
 * For ELF, this is done by constructing a separate segment for each set.
 *
 * In the __MAKE_SET macros below, the lines:
 *
 *   static void const * const __set_##set##_sym_##sym = &sym;
 *
 * are present only to prevent the compiler from producing bogus
 * warnings about unused symbols.
 */

#define __link_make_set(set, sym) 			                	\
	__GLOBL(__start_link_set_##set);		                	\
	__GLOBL(__stop_link_set_##set);			                	\
	static void const * const __link_set_##set##_sym_##sym		\
   	__section("link_set_" #set) __used = &(sym)
    	
#define __link_text_set(set, sym) 	__link_make_set(set, sym)
#define __link_data_set(set, sym)	__link_make_set(set, sym)
#define __link_bss_set(set, sym) 	__link_make_set(set, sym)
#define __link_abs_set(set, sym) 	__link_make_set(set, sym)
#define __link_set_entry(set, sym)  __link_make_set(set, sym)

#define	__link_set_decl(set, ptype)				        		\
	extern ptype *(__link_set_start(set));			        	\
	extern ptype *(__link_set_end(set))

#define __link_set_decl_weak(set, ptype)			        	\
	extern ptype __weak_symbol *(__link_set_start(set));		\
	extern ptype __weak_symbol *(__link_set_end(set));	    	\
	    	
#define __link_set_foreach(pvar, set)							\
	for (pvar = __link_set_start(set); pvar < __link_set_end(set); pvar++)

#define __link_set_item(set, i)                             	\
	((&__link_set_start(set))[i])

#define __MAKE_SET(set, sym)									\
	__link_make_set(set, sym)

#define TEXT_SET(set, sym) 	__link_text_set(set, sym)
#define DATA_SET(set, sym)	__link_data_set(set, sym)
#define BSS_SET(set, sym) 	__link_bss_set(set, sym)
#define ABS_SET(set, sym) 	__link_abs_set(set, sym)
#define SET_ENTRY(set, sym) __link_set_entry(set, sym)

#define SET_DECLARE(set, ptype)									\
	__link_set_decl(set, ptype)
#define SET_DECLARE_WEAK(set, ptype)							\
	__link_set_decl_weak(set, ptype)

/*
 * Iterate over all the elements of a set.
 *
 * Sets always contain addresses of things, and "pvar" points to words
 * containing those addresses.  Thus is must be declared as "type **pvar",
 * and the address of each set item is obtained inside the loop by "*pvar".
 */
#define SET_FOREACH(pvar, set)									\
	__link_set_foreach(pvar, set)

#define SET_ITEM(set, i)										\
	__link_set_item(set, i)

/*
 * Provide a count of the items in a set.
 */
#define SET_COUNT(set)											\
	__link_set_count(set)

#endif	/* _SYS_CDEFS_LINKER_H_ */
