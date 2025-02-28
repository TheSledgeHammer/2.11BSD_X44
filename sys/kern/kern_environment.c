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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/lock.h>
#include <sys/types.h>
#include <sys/stdbool.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kenv.h>
#include <sys/device.h>
#include <sys/ucred.h>

#include <lib/libkern/libkern.h>

#define KENV_SIZE			512	/* Maximum number of environment strings */
static int	kenv_mvallen = 	KENV_MVALLEN;

/* pointer to the config-generated static environment */
char						*kern_envp;

/* pointer to the md-static environment */
char						*md_envp;

/* dynamic environment variables */
char						**kenvp;
struct lock					kenv_lock;
bool_t						dynamic_kenv;

#define KENV_CHECK do { 							\
	if (!dynamic_kenv) {							\
		panic("%s: called before KMEM", __func__);	\
	}												\
} while(0)

static char *_getenv_static(const char *);
static char *_getenv_dynamic(const char *, int *);
static char *_getenv_dynamic_locked(const char *, int *);
static char *getenv_string_buffer(const char *);

int
kenv()
{
	register struct kenv_args {
		syscallarg(int) 		what;
		syscallarg(const char *) name;
		syscallarg(char *) 		value;
		syscallarg(int) 		len;
	} *uap = (struct kenv_args *)u.u_ap;

	char *name, *value, *buffer = NULL;
	size_t len, done, needed, buflen;
	int error, i;

	KASSERT(dynamic_kenv);

	error = 0;
	if (SCARG(uap, what) == KENV_DUMP) {
		done = needed = 0;
		buflen = SCARG(uap, len);
		if (buflen > KENV_SIZE * (KENV_MNAMELEN + kenv_mvallen + 2))
			buflen = KENV_SIZE * (KENV_MNAMELEN + kenv_mvallen + 2);
		if (SCARG(uap, len) > 0 && SCARG(uap, value) != NULL)
			buffer = malloc(buflen, M_TEMP, M_WAITOK | M_ZERO);
		simple_lock(&kenv_lock.lk_lnterlock);
		for (i = 0; kenvp[i] != NULL; i++) {
			len = strlen(kenvp[i]) + 1;
			needed += len;
			len = min(len, buflen - done);
			/*
			 * If called with a NULL or insufficiently large
			 * buffer, just keep computing the required size.
			 */
			if (SCARG(uap, value) != NULL && buffer != NULL && len > 0) {
				bcopy(kenvp[i], buffer + done, len);
				done += len;
			}
		}
		simple_unlock(&kenv_lock.lk_lnterlock);
		if (buffer != NULL) {
			error = copyout(buffer, SCARG(uap, value), done);
			free(buffer, M_TEMP);
		}
		u.u_r.r_val1 = ((done == needed) ? 0 : needed);
		return (error);
	}
	switch (SCARG(uap, what)) {
	case KENV_SET:
		error = priv_check(PRIV_KENV_SET);
		if (error)
			return (error);
		break;

	case KENV_UNSET:
		error = priv_check(PRIV_KENV_UNSET);
		if (error)
			return (error);
		break;
	}
	name = malloc(KENV_MNAMELEN + 1, M_TEMP, M_WAITOK);

	error = copyinstr(SCARG(uap, name), name, KENV_MNAMELEN + 1, NULL);
	if (error)
		goto done;

	switch (SCARG(uap, what)) {
	case KENV_GET:
		value = kern_getenv(name);
		if (value == NULL) {
			error = ENOENT;
			goto done;
		}
		len = strlen(value) + 1;
		if (len > SCARG(uap, len))
			len = SCARG(uap, len);
		error = copyout(value, SCARG(uap, value), len);
		freeenv(value);
		if (error)
			goto done;
		u.u_r.r_val1 = len;
		break;

	case KENV_SET:
		len = SCARG(uap, len);
		if (len < 1) {
			error = EINVAL;
			goto done;
		}
		if (len > kenv_mvallen + 1)
			len = kenv_mvallen + 1;
		value = malloc(len, M_TEMP, M_WAITOK);
		error = copyinstr(SCARG(uap, value), value, len, NULL);
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

void
freeenv(env)
	char *env;
{
	if (dynamic_kenv && env != NULL) {
		bzero(env, strlen(env));
		free(env, M_KENV);
	}
}

/*
 * Look up an environment variable by name.
 * Return a pointer to the string if found.
 * The pointer has to be freed with freeenv()
 * after use.
 */
char *
kern_getenv(name)
	const char *name;
{
	char *ret;

	if (dynamic_kenv) {
		ret = getenv_string_buffer(name);
		if (ret == NULL) {
			lockstatus(&kenv_lock);
		}
	} else
		ret = _getenv_static(name);

	return (ret);
}

/*
 * Set an environment variable by name.
 */
int
kern_setenv(name, value)
	const char *name, *value;
{
	char *buf, *cp, *oldenv;
	int namelen, vallen, i;

	KENV_CHECK;

	namelen = strlen(name) + 1;
	if (namelen > KENV_MNAMELEN + 1)
		return (-1);
	vallen = strlen(value) + 1;
	if (vallen > kenv_mvallen + 1)
		return (-1);
	buf = malloc(namelen + vallen, M_KENV, M_WAITOK);
	sprintf(buf, "%s=%s", name, value);

	simple_lock(&kenv_lock.lk_lnterlock);
	cp = _getenv_dynamic(name, &i);
	if (cp != NULL) {
		oldenv = kenvp[i];
		kenvp[i] = buf;
		simple_unlock(&kenv_lock.lk_lnterlock);
		free(oldenv, M_KENV);
	} else {
		/* We add the option if it wasn't found */
		for (i = 0; (cp = kenvp[i]) != NULL; i++)
			;

		/* Bounds checking */
		if (i < 0 || i >= KENV_SIZE) {
			free(buf, M_KENV);
			simple_unlock(&kenv_lock.lk_lnterlock);
			return (-1);
		}

		kenvp[i] = buf;
		kenvp[i + 1] = NULL;
		simple_unlock(&kenv_lock.lk_lnterlock);
	}

	return (0);
}

/*
 * Unset an environment variable string.
 */
int
kern_unsetenv(name)
	const char *name;
{
	char *cp, *oldenv;
	int i, j;

	KENV_CHECK;

	simple_lock(&kenv_lock.lk_lnterlock);

	cp = _getenv_dynamic(name, &i);
	if (cp != NULL) {
		oldenv = kenvp[i];
		for (j = i + 1; kenvp[j] != NULL; j++)
			kenvp[i++] = kenvp[j];
		kenvp[i] = NULL;
		simple_unlock(&kenv_lock.lk_lnterlock);
		bzero(oldenv, strlen(oldenv));
		free(oldenv, M_KENV);
		return (0);
	}
	simple_unlock(&kenv_lock.lk_lnterlock);

	return (-1);
}

/*
 * Find the next entry after the one which (cp) falls within, return a
 * pointer to its start or NULL if there are no more.
 */
static char *
kernenv_next(cp)
	char *cp;
{

	if (cp != NULL) {
		while (*cp != 0)
			cp++;
		cp++;
		if (*cp == 0)
			cp = NULL;
	}
	return (cp);
}

/*
 * Test if an environment variable is defined.
 */
int
testenv(name)
	const char *name;
{
	char *cp;

	if (dynamic_kenv) {
		simple_lock(&kenv_lock.lk_lnterlock);
		cp = _getenv_dynamic(name, NULL);
		simple_unlock(&kenv_lock.lk_lnterlock);
	} else {
		cp = _getenv_static(name);
	}
	if (cp != NULL) {
		return (1);
	}
	return (0);
}

static void
init_dynamic_kenv_from(init_env, curpos)
	char *init_env;
	int *curpos;
{
	char *cp, *cpnext, *eqpos, *found;
	size_t len;
	int i;

	if (init_env && *init_env != '\0') {
		found = NULL;
		i = *curpos;
		for (cp = init_env; cp != NULL; cp = cpnext) {
			cpnext = kernenv_next(cp);
			len = strlen(cp) + 1;
			if (len > KENV_MNAMELEN + 1 + kenv_mvallen + 1) {
				printf(
				"WARNING: too long kenv string, ignoring %s\n", cp);
				goto sanitize;
			}
			eqpos = strchr(cp, '=');
			if (eqpos == NULL) {
				printf(
				"WARNING: malformed static env value, ignoring %s\n", cp);
				goto sanitize;
			}
			*eqpos = 0;
			/*
			 * De-dupe the environment as we go.  We don't add the
			 * duplicated assignments because config(8) will flip
			 * the order of the static environment around to make
			 * kernel processing match the order of specification
			 * in the kernel config.
			 */
			found = _getenv_dynamic_locked(cp, NULL);
			*eqpos = '=';
			if (found != NULL)
				goto sanitize;
			if (i > KENV_SIZE) {
				printf(
				"WARNING: too many kenv strings, ignoring %s\n", cp);
				goto sanitize;
			}

			kenvp[i] = malloc(len, M_KENV, M_WAITOK);
			strcpy(kenvp[i++], cp);
sanitize:
			bzero(cp, len - 1);
		}
		*curpos = i;
	}
}

/*
 * Setup the dynamic kernel environment.
 */
static void
init_dynamic_kenv(void /* *data */)
{
	int dynamic_envpos;
	int size;

	//TUNABLE_INT_FETCH("kenv_mvallen", &kenv_mvallen);
	size = KENV_MNAMELEN + 1 + kenv_mvallen + 1;

	kenvp = malloc((KENV_SIZE + 1) * sizeof(char *), M_KENV, M_WAITOK | M_ZERO);

	dynamic_envpos = 0;
	init_dynamic_kenv_from(md_envp, &dynamic_envpos);
	init_dynamic_kenv_from(kern_envp, &dynamic_envpos);
	kenvp[dynamic_envpos] = NULL;
	lockinit(&kenv_lock, PLOCK, "kernel environment", 0, 0);
	dynamic_kenv = true;
}

void
kenv_init(void)
{
	init_dynamic_kenv();
}

/*
 * Internal functions for string lookup.
 */
static char *
_getenv_dynamic_locked(name, idx)
	const char *name;
	int *idx;
{
	char *cp;
	int len, i;

	len = strlen(name);
	for (cp = kenvp[0], i = 0; cp != NULL; cp = kenvp[++i]) {
		if ((strncmp(cp, name, len) == 0) &&
		    (cp[len] == '=')) {
			if (idx != NULL)
				*idx = i;
			return (cp + len + 1);
		}
	}
	return (NULL);
}

static char *
_getenv_dynamic(name, idx)
	const char *name;
	int *idx;
{
	return (_getenv_dynamic_locked(name, idx));
}

static char *
_getenv_static_from(chkenv, name)
	char *chkenv;
	const char *name;
{
	char *cp, *ep;
	int len;

	for (cp = chkenv; cp != NULL; cp = kernenv_next(cp)) {
		for (ep = cp; (*ep != '=') && (*ep != 0); ep++)
			;
		if (*ep != '=')
			continue;
		len = ep - cp;
		ep++;
		if (!strncmp(name, cp, len) && name[len] == 0)
			return (ep);
	}
	return (NULL);
}

static char *
_getenv_static(name)
	const char *name;
{
	char *val;

	val = _getenv_static_from(md_envp, name);
	if (val != NULL)
		return (val);
	val = _getenv_static_from(kern_envp, name);
	if (val != NULL)
		return (val);
	return (NULL);
}

/*
 * Return a buffer containing the string value from an environment variable
 */
static char *
getenv_string_buffer(name)
	const char *name;
{
	char *cp, *ret;
	int len;

	if (dynamic_kenv) {
		len = KENV_MNAMELEN + 1 + kenv_mvallen + 1;
		ret = malloc(sizeof(char *), M_KENV, M_WAITOK | M_ZERO);
		simple_lock(&kenv_lock.lk_lnterlock);
		cp = _getenv_dynamic(name, NULL);
		if (cp != NULL)
			strlcpy(ret, cp, len);
		simple_unlock(&kenv_lock.lk_lnterlock);
		if (cp == NULL) {
			free(ret, M_KENV);
			ret = NULL;
		}
	} else
		ret = _getenv_static(name);

	return (ret);
}

/*
 * Return a string value from an environment variable.
 */
int
getenv_string(name, data, size)
	const char *name;
	char *data;
	int size;
{
	char *cp;

	if (dynamic_kenv) {
		simple_lock(&kenv_lock.lk_lnterlock);
		cp = _getenv_dynamic(name, NULL);
		if (cp != NULL)
			strlcpy(data, cp, size);
		simple_unlock(&kenv_lock.lk_lnterlock);
	} else {
		cp = _getenv_static(name);
		if (cp != NULL)
			strlcpy(data, cp, size);
	}
	return (cp != NULL);
}

/*
 * Return an array of integers at the given type size and signedness.
 */
int
getenv_array(name, pdata, size, psize, type_size, allow_signed)
	const char *name;
	void *pdata;
	int size;
	int *psize;
	int type_size;
	bool_t allow_signed;
{
	uint8_t shift;
	int64_t value;
	int64_t old;
	char *buf;
	char *end;
	char *ptr;
	int n;
	int rc;

	if ((buf = getenv_string_buffer(name)) == NULL)
		return (0);

	rc = 0;			  /* assume failure */
	/* get maximum number of elements */
	size /= type_size;

	n = 0;

	for (ptr = buf; *ptr != 0; ) {

		value = strtoul(ptr, &end, 0);

		/* check if signed numbers are allowed */
		if (value < 0 && !allow_signed)
			goto error;

		/* check for invalid value */
		if (ptr == end)
			goto error;

		/* check for valid suffix */
		switch (*end) {
		case 't':
		case 'T':
			shift = 40;
			end++;
			break;
		case 'g':
		case 'G':
			shift = 30;
			end++;
			break;
		case 'm':
		case 'M':
			shift = 20;
			end++;
			break;
		case 'k':
		case 'K':
			shift = 10;
			end++;
			break;
		case ' ':
		case '\t':
		case ',':
		case 0:
			shift = 0;
			break;
		default:
			/* garbage after numeric value */
			goto error;
		}

		/* skip till next value, if any */
		while (*end == '\t' || *end == ',' || *end == ' ')
			end++;

		/* update pointer */
		ptr = end;

		/* apply shift */
		old = value;
		value <<= shift;

		/* overflow check */
		if ((value >> shift) != old)
			goto error;

		/* check for buffer overflow */
		if (n >= size)
			goto error;

		/* store value according to type size */
		switch (type_size) {
		case 1:
			if (allow_signed) {
				if (value < SCHAR_MIN || value > SCHAR_MAX)
					goto error;
			} else {
				if (value < 0 || value > UCHAR_MAX)
					goto error;
			}
			((uint8_t *)pdata)[n] = (uint8_t)value;
			break;
		case 2:
			if (allow_signed) {
				if (value < SHRT_MIN || value > SHRT_MAX)
					goto error;
			} else {
				if (value < 0 || value > USHRT_MAX)
					goto error;
			}
			((uint16_t *)pdata)[n] = (uint16_t)value;
			break;
		case 4:
			if (allow_signed) {
				if (value < INT_MIN || value > INT_MAX)
					goto error;
			} else {
				if (value > UINT_MAX)
					goto error;
			}
			((uint32_t *)pdata)[n] = (uint32_t)value;
			break;
		case 8:
			((uint64_t *)pdata)[n] = (uint64_t)value;
			break;
		default:
			goto error;
		}
		n++;
	}
	*psize = n * type_size;

	if (n != 0)
		rc = 1;	/* success */
error:
	if (dynamic_kenv)
		free(buf, M_KENV);
	return (rc);
}

/*
 * Return an integer value from an environment variable.
 */
int
getenv_int(name, data)
	const char *name;
	int *data;
{
	quad_t tmp;
	int rval;

	rval = getenv_quad(name, &tmp);
	if (rval)
		*data = (int) tmp;
	return (rval);
}

/*
 * Return an unsigned integer value from an environment variable.
 */
int
getenv_uint(name, data)
	const char *name;
	unsigned int *data;
{
	quad_t tmp;
	int rval;

	rval = getenv_quad(name, &tmp);
	if (rval)
		*data = (unsigned int) tmp;
	return (rval);
}

/*
 * Return an int64_t value from an environment variable.
 */
int
getenv_int64(name, data)
	const char *name;
	int64_t *data;
{
	quad_t tmp;
	int64_t rval;

	rval = getenv_quad(name, &tmp);
	if (rval)
		*data = (int64_t) tmp;
	return (rval);
}

/*
 * Return an uint64_t value from an environment variable.
 */
int
getenv_uint64(name, data)
	const char *name;
	uint64_t *data;
{
	quad_t tmp;
	uint64_t rval;

	rval = getenv_quad(name, &tmp);
	if (rval)
		*data = (uint64_t) tmp;
	return (rval);
}

/*
 * Return a long value from an environment variable.
 */
int
getenv_long(name, data)
	const char *name;
	long *data;
{
	quad_t tmp;
	int rval;

	rval = getenv_quad(name, &tmp);
	if (rval)
		*data = (long) tmp;
	return (rval);
}

/*
 * Return an unsigned long value from an environment variable.
 */
int
getenv_ulong(name, data)
	const char *name;
	unsigned long *data;
{
	quad_t tmp;
	int rval;

	rval = getenv_quad(name, &tmp);
	if (rval)
		*data = (unsigned long) tmp;
	return (rval);
}

/*
 * Return a quad_t value from an environment variable.
 */
int
getenv_quad(name, data)
	const char *name;
	quad_t *data;
{
	char	*value, *vtp;
	quad_t	iv;

	value = getenv_string_buffer(name);
	if (value == NULL)
		return (0);
	iv = strtoul(value, &vtp, 0);
	if (vtp == value || (vtp[0] != '\0' && vtp[1] != '\0')) {
		freeenv(value);
		return (0);
	}
	switch (vtp[0]) {
	case 't': case 'T':
		iv *= 1024;
		/* FALLTHROUGH */
	case 'g': case 'G':
		iv *= 1024;
		/* FALLTHROUGH */
	case 'm': case 'M':
		iv *= 1024;
		/* FALLTHROUGH */
	case 'k': case 'K':
		iv *= 1024;
	case '\0':
		break;
	default:
		freeenv(value);
		return (0);
	}
	freeenv(value);
	*data = iv;
	return (1);
}
