/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1998 Michael Smith
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
 */

/*
 * The unified bootloader passes us a pointer to a preserved copy of
 * bootstrap/kernel environment variables.  We convert them to a
 * dynamic array of strings later when the VM subsystem is up.
 *
 * We make these available through the kenv(2) syscall for userland
 * and through kern_getenv()/freeenv() kern_setenv() kern_unsetenv() testenv() for
 * the kernel.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD$"); */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/user.h>
//#include <sys/mutex.h>
//#include <sys/priv.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysent.h>
#include <kenv.h>
//#include <sys/limits.h>

#include <lib/libkern/libkern.h>

#define KENV_SIZE	512	/* Maximum number of environment strings */
static int	kenv_mvallen = KENV_MVALLEN;

/* pointer to the config-generated static environment */
char		*kern_envp;

/* pointer to the md-static environment */
char		*md_envp;
static int	md_env_len;
static int	md_env_pos;


/* dynamic environment variables */
char				**kenvp;
struct simplelock 	kenv_lock;

int
kenv(uap)
	struct kenv_args *uap;
{
	char *name, *value, *buffer = NULL;
	size_t len, done, needed, buflen;
	int error, i;

	error = 0;
	if (uap->what == KENV_DUMP) {
		done = needed = 0;
		buflen = uap->len;
		if (buflen > KENV_SIZE * (KENV_MNAMELEN + kenv_mvallen + 2))
			buflen = KENV_SIZE * (KENV_MNAMELEN + kenv_mvallen + 2);
		if (uap->len > 0 && uap->value != NULL)
			buffer = malloc(buflen, M_TEMP, M_WAITOK | M_ZERO);
		for (i = 0; kenvp[i] != NULL; i++) {
			len = strlen(kenvp[i]) + 1;
			needed += len;
			len = min(len, buflen - done);
			/*
			 * If called with a NULL or insufficiently large
			 * buffer, just keep computing the required size.
			 */
			if (uap->value != NULL && buffer != NULL && len > 0) {
				bcopy(kenvp[i], buffer + done, len);
				done += len;
			}
		}
		if (buffer != NULL) {
			error = copyout(buffer, uap->value, done);
			free(buffer, M_TEMP);
		}
		u->u_r.r_val1 = ((done == needed) ? 0 : needed);
		return (error);
	}
	switch (uap->what) {
	case KENV_SET:
	case KENV_UNSET:
	}
	name = malloc(KENV_MNAMELEN + 1, M_TEMP, M_WAITOK);

	error = copyinstr(uap->name, name, KENV_MNAMELEN + 1, NULL);
	if (error)
		goto done;

	switch (uap->what) {
	case KENV_GET:
		value = kern_getenv(name);
		if (value == NULL) {
			error = ENOENT;
			goto done;
		}
		len = strlen(value) + 1;
		if (len > uap->len)
			len = uap->len;
		error = copyout(value, uap->value, len);
		freeenv(value);
		if (error)
			goto done;
		u->u_r.r_val1 = len;
		break;

	case KENV_SET:
		len = uap->len;
		if (len < 1) {
			error = EINVAL;
			goto done;
		}
		if (len > kenv_mvallen + 1)
			len = kenv_mvallen + 1;
		value = malloc(len, M_TEMP, M_WAITOK);
		error = copyinstr(uap->value, value, len, NULL);
		if (error) {
			free(value, M_TEMP);
			goto done;
		} else {
			kern_setenv(name, value);
		}
		free(value, M_TEMP);
		break;

	case KENV_UNSET:
		error = kern_unsetenv(name);
		if (error)
			error = ENOENT;
		break;

	default:
		error = EINVAL;
		break;
	}

done:
	free(name, M_TEMP);
	return (error);
}

/*
 * Look up an environment variable by name.
 * Return a pointer to the string if found.
 * The pointer has to be freed with freeenv()
 * after use.
 */
char *
kern_getenv(const char *name)
{
	char *ret;

	return (ret);
}

/*
 * Set an environment variable by name.
 */
int
kern_setenv(const char *name, const char *value)
{
	char *buf, *cp, *oldenv;
	int namelen, vallen, i;

	return (0);
}

/*
 * Unset an environment variable string.
 */
int
kern_unsetenv(const char *name)
{
	char *cp, *oldenv;
	int i, j;

	return (-1);
}
