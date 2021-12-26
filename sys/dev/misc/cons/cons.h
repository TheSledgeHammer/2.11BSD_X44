/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	@(#)cons.h	8.1 (Berkeley) 6/11/93
 */

struct consdev {
	void		(*cn_probe)(struct consdev *);			/* probe hardware and fill in consdev info */
	void		(*cn_init)(struct consdev *);			/* turn on as console */
	int			(*cn_getc)(dev_t);						/* kernel getchar interface */
	void		(*cn_putc)(dev_t, int);					/* kernel putchar interface */
	void		(*cn_pollc)(dev_t, int); 				/* turn on and off polling */
	void		(*cn_bell)(dev_t, u_int, u_int, u_int);	/* ring bell */
	void		(*cn_flush)(dev_t);						/* flush output */
	struct	tty *cn_tp;									/* tty structure for console device */
	dev_t		cn_dev;									/* major/minor of device */
	int			cn_pri;									/* pecking order; the higher the better */
};

/* values for cn_pri - reflect our policy for console selection */
#define	CN_DEAD		0	/* device doesn't exist */
#define CN_NULL		1	/* noop console */
#define CN_NORMAL	2	/* device exists but is nothing special */
#define CN_INTERNAL	3	/* "internal" bit-mapped display */
#define CN_REMOTE	4	/* serial interface with remote bit set */

/* XXX */
#define	CONSMAJOR	0

#ifdef KERNEL
extern	struct consdev constab[];
extern	struct consdev *cn_tab;

void	cninit(void);
int		cnopen(dev_t, int, int, struct proc *);
int		cnclose(dev_t, int, int, struct proc *);
int		cnread(dev_t, struct uio *, int);
int		cnwrite(dev_t, struct uio *, int);
int		cnioctl(dev_t, u_long, caddr_t, int, struct proc *);
int		cnpoll(dev_t, int, struct proc *);
int		cnkqfilter(dev_t, struct knote *);
void	cnbell(u_int, u_int, u_int);
void	cnflush(void);
int		cngetc(void);
void	cnputc(int);
void	cnpollc(int);
void	cnrint(void);
void	nullcnpollc(dev_t, int);

/* console-specific types */
#define	dev_type_cnprobe(n)	void n (struct consdev *)
#define	dev_type_cninit(n)	void n (struct consdev *)
#define	dev_type_cngetc(n)	int n (dev_t)
#define	dev_type_cnputc(n)	void n (dev_t, int)
#define	dev_type_cnpollc(n)	void n (dev_t, int)
#define	dev_type_cnbell(n)	void n(dev_t, u_int, u_int, u_int)
#define	dev_type_cnflush(n)	void n(dev_t)

#define	dev_decl(n,t)		__CONCAT(dev_type_,t)(__CONCAT(n,t))
#define	dev_init(n,t)		__CONCAT(n,t)

#define	cons_decl(n) 												\
	dev_decl(n,cnprobe); dev_decl(n,cninit); dev_decl(n,cngetc); 	\
	dev_decl(n,cnputc); dev_decl(n,cnpollc); dev_decl(n,cnbell);	\
	dev_decl(n,cnflush);

#define	cons_init(n) { 												\
	dev_init(n,cnprobe), dev_init(n,cninit), dev_init(n,cngetc), 	\
	dev_init(n,cnputc), dev_init(n,cnpollc), NULL, NULL,			\
	0, 0 }

#define	cons_init_bell(n) { 										\
	dev_init(n,cnprobe), dev_init(n,cninit), dev_init(n,cngetc), 	\
	dev_init(n,cnputc), dev_init(n,cnpollc), dev_init(n,cnbell) }
#endif
