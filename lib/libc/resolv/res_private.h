/*-
 * Copyright (c) 2024
 * 	Martin Kelly.
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
 */

#ifndef _RES_PRIVATE_H_
#define _RES_PRIVATE_H_

#define __res_ninit         res_ninit
#define __res_nmkquery      res_nmkquery
#define __res_nquery        res_nquery
#define __res_nquerydomain  res_nquerydomain
#define __res_hostalias  	res_hostalias
#define __res_nsearch       res_nsearch
#define __res_nsend         res_nsend
#define __res_nclose        res_nclose
#define __res_ndestroy      res_ndestroy
#define __res_rndinit       res_rndinit
#define __res_nrandomid     res_nrandomid

__BEGIN_DECLS
int	    res_ninit(res_state);
int		res_nmkquery(res_state, int, const char *, int, int, const u_char *, int, const u_char *, u_char *, int);
int		res_nquery(res_state, char *, int, int, u_char *, int);
int		res_nquerydomain(res_state, char *, char *, int, int, u_char *, int);
const char *res_hostalias(res_state, const char *, char *, size_t);
int		res_nsearch(res_state, char *, int, int, u_char *, int);
int		res_nsend(res_state, const u_char *, int, u_char *, int);
void	res_nclose(res_state);
void	res_ndestroy(res_state);
void	res_rndinit(res_state);
u_int	res_nrandomid(res_state);
__END_DECLS
#endif /* _RES_PRIVATE_H_ */
