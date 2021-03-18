/*
 * Copyright (c) 1998 Michael Smith.
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
 * From	$NetBSD: stand.h,v 1.22 1997/06/26 19:17:40 drochner Exp $
 */

/*-
 * Copyright (c) 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)stand.h	8.1 (Berkeley) 6/11/93
 */

#ifndef _LIBSA_ENVIRONMENT_H_
#define _LIBSA_ENVIRONMENT_H_

#include <lib/libsa/stand.h>

/* stand/boot/common/environment.c */
#define EV_DYNAMIC		(1<<0)		/* value was dynamically allocated, free if changed/unset */
#define EV_VOLATILE		(1<<1)		/* value is volatile, make a copy of it */
#define EV_NOHOOK		(1<<2)		/* don't call hook when setting */

struct env_var;
typedef char			*(ev_format_t)(struct env_var *ev);
typedef int				(ev_sethook_t)(struct env_var *ev, int flags, const void *value);
typedef int				(ev_unsethook_t)(struct env_var *ev);

struct env_var {
    char				*ev_name;
    int					ev_flags;
    void				*ev_value;
    ev_sethook_t		*ev_sethook;
    ev_unsethook_t		*ev_unsethook;
    struct env_var		*ev_next, *ev_prev;
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

#endif /* _LIBSA_ENVIRONMENT_H_ */
