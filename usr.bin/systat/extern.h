/*-
 * Copyright (c) 1991, 1993
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
 *      @(#)extern.h	8.1 (Berkeley) 6/6/93
 */

#include <sys/cdefs.h>
#include <fcntl.h>
#include <kvm.h>

#define ADJINETCTR(c, o, n, e)	(c.e = n.e - o.e)

extern struct	cmdtab *curcmd;
extern struct	cmdtab cmdtab[];
extern struct	text *xtext;
extern WINDOW	*wnd;
extern char	**dr_name;
extern char	c, *namp, hostname[];
extern double	avenrun[3];
extern float	*dk_mspw;
extern kvm_t	*kd;
extern long	ntext, textp;
extern int	*dk_select;
extern int	CMDLINE;
extern int	dk_ndrive;
extern int	hz, stathz;
extern int	naptime, col;
extern int	nhosts;
extern int	nports;
extern int	protos;
extern int	verbose;

struct inpcb;
#ifdef INET6
struct in6pcb;
#endif

/* misc */
int	 checkhost(struct inpcb *);
int	 checkport(struct inpcb *);
#ifdef INET6
int	 checkhost6(struct in6pcb *);
int	 checkport6(struct in6pcb *);
#endif

int	 	cmdiostat(char *, char *);
int	 	cmdkre(char *, char *);
int	 	cmdnetstat(char *, char *);
struct	cmdtab *lookup(char *);
void	command(char *);
void	die(int);
void	display(int);
int	 	dkinit(void);
int	 	dkcmd(char *, char *);
void	error(const char *fmt, ...);
int	 	keyboard(void);
int	 	kvm_ckread(void *, void *, int);
void	labels(void);
void	load(void);
int	 	netcmd(char *, char *);
void	nlisterr(struct nlist []);
int	 	prefix(char *, char *);
void	status(void);
void	suspend(int);

/* open */
WINDOW	*openicmp(void);
WINDOW	*openiostat(void);
WINDOW	*openip(void);
#ifdef INET6
WINDOW	*openip6(void);
#endif
#ifdef KAME_IPSEC
WINDOW *openipsec(void);
#endif
WINDOW	*openkre(void);
WINDOW	*openmbufs(void);
WINDOW	*opennetstat(void);
WINDOW	*openpigs(void);
WINDOW	*openswap(void);
WINDOW 	*opentcp(void);

/* close */
void	closeicmp(WINDOW *);
void	closeiostat(WINDOW *);
void	closeip(WINDOW *);
#ifdef INET6
void	closeip6(WINDOW *);
#endif
#ifdef KAME_IPSEC
void	closeipsec(WINDOW *);
#endif
void	closekre(WINDOW *);
void	closembufs(WINDOW *);
void	closenetstat(WINDOW *);
void	closepigs(WINDOW *);
void	closeswap(WINDOW *);
void	closetcp(WINDOW *);

/* show */
void	showicmp(void);
void	showiostat(void);
void	showip(void);
#ifdef INET6
void	showip6(void);
#endif
#ifdef KAME_IPSEC
void	showipsec(void);
#endif
void	showkre(void);
void	showmbufs(void);
void	shownetstat(void);
void	showpigs(void);
void	showswap(void);
void	showtcp(void);
void	showtcpsyn(void);

/* fetch */
void	fetchicmp(void);
void	fetchiostat(void);
void	fetchip(void);
#ifdef INET6
void	fetchip6(void);
#endif
#ifdef KAME_IPSEC
void	fetchipsec(void);
#endif
void	fetchkre(void);
void	fetchmbufs(void);
void	fetchnetstat(void);
void	fetchpigs(void);
void	fetchswap(void);
void	fetchtcp(void);

/* label */
void	labelicmp(void);
void	labeliostat(void);
void	labelip(void);
#ifdef INET6
void	labelip6(void);
#endif
#ifdef KAME_IPSEC
void	labelipsec(void);
#endif
void	labelkre(void);
void	labelmbufs(void);
void	labelnetstat(void);
void	labelpigs(void);
void	labelswap(void);
void	labeltcp(void);
void	labeltcpsyn(void);

/* init */
int	 	initicmp(void);
int	 	initiostat(void);
int		initip(void);
#ifdef INET6
int		initip6(void);
#endif
#ifdef KAME_IPSEC
int		initipsec(void);
#endif
int	 	initkre(void);
int	 	initmbufs(void);
int	 	initnetstat(void);
int	 	initpigs(void);
int	 	initswap(void);
int		inittcp(void);
