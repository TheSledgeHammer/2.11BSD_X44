/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)saerrno.h	8.1 (Berkeley) 6/11/93
 */

#ifndef _LIBSA_SAERRNO_H_
#define _LIBSA_SAERRNO_H_

#include <sys/errno.h>

extern	int errno;	/* just like unix */

/* stand error codes */
#define	EADAPT	(ELAST+1)	/* bad adaptor */
#define	ECTLR	(ELAST+2)	/* bad controller */
#define	EUNIT	(ELAST+3)	/* bad drive */
#define ESLICE	(ELAST+4)	/* bad slice */
#define	EPART	(ELAST+5)	/* bad partition */
#define	ERDLAB	(ELAST+6)	/* can't read disk label */
#define	EUNLAB	(ELAST+7)	/* unlabeled disk */
#define	ENXIO	(ELAST+8)	/* bad device specification */
#define	EBADF	(ELAST+9)	/* bad file descriptor */
#define	EOFFSET	(ELAST+10)	/* relative seek not supported */
#define	ESRCH	(ELAST+11)	/* directory search for file failed */
#define	EIO		(ELAST+12)	/* generic error */
#define	ECMD	(ELAST+13)	/* undefined driver command */
#define	EBSE	(ELAST+14)	/* bad sector error */
#define	EWCK	(ELAST+15)	/* write check error */
#define	EECC	(ELAST+16)	/* uncorrectable ecc error */
#define	EHER	(ELAST+17)	/* hard error */
#define	ESALAST	(ELAST+17)	/* */

#endif /* _LIBSA_SAERRNO_H_ */
