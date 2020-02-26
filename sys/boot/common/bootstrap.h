/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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
 * $FreeBSD$
 */

/* Re-basing & cleanup of this file in bootstrap2.h
 * Only keeping needed macros and functions.
 */
#ifndef _BOOTSTRAP_H_
#define	_BOOTSTRAP_H_

#include <sys/types.h>
#include <sys/queue.h>

/*
 * Generic device specifier; architecture-dependent
 * versions may be larger, but should be allowed to
 * overlap.
 */

struct devdesc
{
    struct devsw	*d_dev;
    int				d_type;
#define DEVT_NONE	0
#define DEVT_DISK	1
#define DEVT_NET	2
#define	DEVT_CD		3
};

/* calloc.c */
void    *calloc(unsigned int, unsigned int);

/* various string functions */
size_t	strspn(const char *s1, const char *s2);
size_t	strlen(const char *s);
char 	*strcpy(char * restrict dst, const char * restrict src);
char 	*strcat(char * restrict s, const char * restrict append);

/* pager.c */
extern void	pager_open(void);
extern void	pager_close(void);
extern int	pager_output(const char *lines);
extern int	pager_file(const char *fname);

/* environment.c */
#define EV_DYNAMIC	(1<<0)		/* value was dynamically allocated, free if changed/unset */
#define EV_VOLATILE	(1<<1)		/* value is volatile, make a copy of it */
#define EV_NOHOOK	(1<<2)		/* don't call hook when setting */

struct env_var;
typedef char	*(ev_format_t)(struct env_var *ev);
typedef int		(ev_sethook_t)(struct env_var *ev, int flags, const void *value);
typedef int		(ev_unsethook_t)(struct env_var *ev);

struct env_var
{
    char			*ev_name;
    int				ev_flags;
    void			*ev_value;
    ev_sethook_t	*ev_sethook;
    ev_unsethook_t	*ev_unsethook;
    struct env_var	*ev_next, *ev_prev;
};
extern struct env_var	*environ;

extern struct env_var	*env_getenv(const char *name);
extern int				env_setenv(const char *name, int flags, const void *value, ev_sethook_t sethook, ev_unsethook_t unsethook);
extern char				*getenv(const char *name);
extern int				setenv(const char *name, const char *value, int overwrite);
extern int				putenv(const char *string);
extern int				unsetenv(const char *name);

extern ev_sethook_t		env_noset;			/* refuse set operation */
extern ev_unsethook_t	env_nounset;		/* refuse unset operation */

/* FreeBSD wrappers */
struct dirent *readdirfd(int);               /* XXX move to stand.h */

#define free(ptr) dealloc(ptr, 0) /* XXX UGLY HACK!!! This should work for just now though. See: libsa/alloc.c:free() */

/* XXX Hack Hack Hack!!! Need to update stand.h with fs_ops->fo_readdir */
#ifdef SKIFS /* defined via stand/ia64/ski/Makefile */
#define FS_READDIR(f, dirptr) skifs_readdir(f, dirptr)
#else
#define FS_READDIR(f, dirptr) efifs_readdir(f, dirptr)
#endif

/* gets.c XXX move to libsa/ */

extern int	fgetstr(char *buf, int size, int fd);
extern void	ngets(char *, int);

/* imports from stdlib, modified for sa */

extern long	strtol(const char *, char **, int);
extern char	*optarg;			/* getopt(3) external variables */
extern int	optind, opterr, optopt, optreset;
extern int	getopt(int, char * const [], const char *);

extern long	strtol(const char *, char **, int);

#endif /* !_BOOTSTRAP_H_ */
