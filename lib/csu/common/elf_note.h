/* $NetBSD: sysident.S,v 1.3 2014/05/14 14:59:14 joerg Exp $ */

/*
 * Copyright (c) 1997 Christopher G. Demetriou
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

/*
 * Here we define the NetBSD OS Version in an ELF .note section, structured
 * like:
 *
 * [NOTE HEADER]
 *	long		name size
 *	long		description size
 *	long		note type
 *
 * [NOTE DATUM]
 *	string		OS name
 *
 * OSVERSION notes also have:
 *	long		OS version (__211BSD_Version__ constant from param.h)
 *
 * The DATUM fields should be padded out such that their actual (not
 * declared) sizes % 4 == 0.
 *
 * These are used by the kernel to determine if this binary is really a
 * NetBSD binary, or some other OS's.
 */

__asm(
        "	.section \".note.netbsd.ident\", \"a\"\n"
        "	.p2align 2\n"
        "	.long	7\n"
        "	.long	4\n"
        "	.long	1\n"
        "	.ascii \"NetBSD\\0\"\n"
        "	.long	0\n"
        "	.previous\n");

__asm(
        "	.section \".note.netbsd.pax\", \"a\"\n"
        "	.p2align 2\n"
        "	.long	4\n"
        "	.long	4\n"
        "	.long	3\n"
        "	.ascii \"Pax\\0\"\n"
        "	.long	0\n"
        "	.previous\n");
