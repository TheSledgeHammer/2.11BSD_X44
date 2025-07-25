/* $NetBSD: dot_init.h,v 1.8 2008/05/10 15:31:04 martin Exp $ */

/*-
 * Copyright (c) 2001 Ross Harvey
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _I386_DOT_INIT_H_
#define _I386_DOT_INIT_H_

#include <sys/cdefs.h>

#define	MD_SECTION_PROLOGUE(sect, entry_pt)			\
	__asm (							\
			".section "#sect",\"ax\",@progbits	\n"\
			".globl "#entry_pt" 			\n"\
			"	.align	16			\n"\
			#entry_pt":				\n"\
			"	pushl	%ebp			\n"\
			"	movl	%esp, %ebp		\n"\
			"	/* fall thru */			\n"\
			".previous                              \n"\
          )

		/* placement of the .align _after_ the label is intentional */

#define	MD_SECTION_EPILOGUE(sect)				\
	__asm (							\
			".section "#sect",\"ax\",@progbits	\n"\
			"	leave				\n"\
			"	ret				\n"\
			".previous                              \n"\
          )

#define	MD_CALL_STATIC_FUNCTION(section, func)	    		\
    __asm (                                           		\
            ".section " #section "		            	\n"\
            " call " #func "                     		\n"\
            ".previous		                        	\n"\
        )

#define	MD_INIT_SECTION_PROLOGUE MD_SECTION_PROLOGUE(.init, _init)
#define	MD_FINI_SECTION_PROLOGUE MD_SECTION_PROLOGUE(.fini, _fini)

#define	MD_INIT_SECTION_EPILOGUE MD_SECTION_EPILOGUE(.init)
#define	MD_FINI_SECTION_EPILOGUE MD_SECTION_EPILOGUE(.fini)

#endif /* _I386_DOT_INIT_H_ */
