/*
 * Copyright (C) 2005 David Xu <davidxu@freebsd.org>.
 * Copyright (C) 2000 Jason Evans <jasone@freebsd.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*	$OpenBSD: rthread_sem.c,v 1.33 2022/05/14 14:52:20 cheloha Exp $ */
/*
 * Copyright (c) 2004,2005,2013 Ted Unangst <tedu@openbsd.org>
 * Copyright (c) 2018 Paul Irofti <paul@irofti.net>
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Provide a kernel-like backend for sem.c, using atomic operations and mmap.
 */
/*
 * NOTES:
 * - Internal only!!
 * - Provides minimal path and timing for sem.c
 * - Does not use pthreads. (See sem.c)
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/queue.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>

#include "pthread.h"
#include "pthread_int.h"
#include "sema.h"

static char const *sem_prefix = "/var/run/sem";

static int get_path(const char *name, char *path, size_t len, char const **prefix);

int
sema_alloc(int pshared, unsigned int value, semid_t semid, size_t size, sem_t *semp)
{
	sem_t sem, sem_map;

	if (value > SEM_VALUE_MAX) {
		return (EINVAL);
	}

	if (pshared) {
		sem_map = mmap(NULL, size, (PROT_READ | PROT_WRITE), (MAP_ANON | MAP_SHARED), -1, 0);
		if (sem_map == MAP_FAILED) {
			return (ENOSPC);
		}
		sem = sem_map;
	} else {
		sem = malloc(sizeof(struct _sem_st));
		if (sem == NULL) {
			return (ENOSPC);
		}
	}

	sem->usem_magic = USEM_MAGIC;
	pthread_lockinit(&sem->usem_interlock);
	PTQ_INIT(&sem->usem_waiters);
	sem->usem_count = value;
	sem->usem_semid = semid;
	*semp = sem;
	return (0);
}

void
sema_free(sem_t sem)
{
	sem->usem_magic = 0;
	free(sem);
}

int
sem_init(sem_t *sem, int pshared, unsigned int value)
{
	semid_t	semid;
	int error;

	semid = USEM_USER;

	error = sema_alloc(pshared, value, semid, getpagesize(), sem);
	if (error != 0) {
		if (semid != USEM_USER) {
			(void)sema_destroy(sem);
		}
		errno = error;
		return (-1);
	}
	return (0);
}

int
sem_errorcheck(sem_t *sem)
{
#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->usem_magic != USEM_MAGIC) {
		return (-1);
	}
#endif
	return (0);
}

int
sema_destroy(sem_t *sem)
{
	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}

	if ((*sem)->usem_waitcount) {
		errno = EBUSY;
		return (-1);
	}

	if ((*sem)->usem_semid) {
		sem_free(*sem);
	}
	return (0);
}

int
sema_open(const char *name, int oflag, mode_t mode, unsigned int value, size_t size, semid_t *idp)
{
	sem_t sem;
	struct stat sb;
	char path[MAXPATHLEN], sempath[MAXPATHLEN];
	char const *prefix = NULL;
	size_t path_len;
	int error, fd;

	error = get_path(name, path, MAXPATHLEN, &prefix);
	if (error) {
		errno = error;
		return (-1);
	}

	strlcpy(sempath, prefix, sizeof(sempath));
	path_len = strlcat(sempath, "/sem.XXXXXX", sizeof(sempath));
	if (path_len > sizeof(sempath)) {
		errno = ENAMETOOLONG;
		return (-1);
	}

	fd = open(sempath, O_RDWR | O_NOFOLLOW | oflag, mode);
	if (fd == -1) {
		return (-1);
	}
	if (fstat(fd, &sb) == -1 || !S_ISREG(sb.st_mode)) {
		error = EINVAL;
		goto bad;
	}
	if (sb.st_uid != geteuid()) {
		error = EPERM;
		goto bad;
	}
	if (sb.st_size != (off_t)size) {
		if (!(oflag & O_CREAT)) {
			error = EINVAL;
			goto bad;
		}
		if (sb.st_size != 0) {
			error = EINVAL;
			goto bad;
		}
		if (ftruncate(fd, size) == -1) {
			error = EINVAL;
			goto bad;
		}
	}

	sem = mmap(NULL, size, (PROT_READ | PROT_WRITE), (MAP_ANON | MAP_SHARED), fd, 0);
	if (sem == MAP_FAILED) {
		error = EINVAL;
		goto bad;
	}
	*idp = sem;
	return (0);

bad:
	close(fd);
	errno = error;
	return (-1);
}

int
sema_close(sem_t *sem, size_t size)
{
	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}

	munmap(sem, size);
	free(sem);
	return (0);
}

int
sema_unlink(const char *name)
{
	char path[MATHPATHLEN];
	const char *prefix;
	int error;

	error = get_path(name, path, MATHPATHLEN, &prefix);
	if (error) {
		errno = error;
		return (-1);
	}
	error = unlink(name);
	if(error) {
		if ((errno != ENAMETOOLONG) && (errno != ENOENT)) {
			errno = EACCES;
		}
		return (-1);
	}
	return (0);
}

#ifdef notyet
int
sema_wait(sem_t *sem, struct timespec *abstime)
{
	unsigned int val;
	int error;

	atomic_inc_int((*sem)->usem_waitcount);
	for (;;) {
		while ((val = (*sem)->usem_count) > 0) {
			if (atomic_cas_uint((*sem)->usem_count, val, val - 1) == val) {
				membar_enter_after_atomic();
				atomic_dec_int((*sem)->usem_waitcount);
				return (0);
			}
		}
		/* missing time shit here */
		error =
	}

	atomic_dec_int((*sem)->usem_waitcount);
	return (error);
}
#endif

int
sema_trywait(sem_t *sem)
{
	unsigned int val;

	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}

	while ((val = (*sem)->usem_count) > 0) {
		if (atomic_cas_uint((*sem)->usem_count, val, val - 1) == val) {
			membar_enter_after_atomic();
			return (0);
		}
	}

	errno = EAGAIN;
	return (-1);
}

int
sema_post(sem_t *sem)
{
	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}
	atomic_inc_int((*sem)->usem_count);
	return (0);
}

int
sema_getvalue(sem_t *sem, int *sval)
{
	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}
	sval = (*sem)->usem_count;
	return (0);
}

static int
get_path(const char *name, char *path, size_t len, char const **prefix)
{
	size_t path_len;

	*prefix = NULL;

	if (name[0] == '/') {
		*prefix = sem_prefix;

		path_len = strlcpy(path, *prefix, len);

		if (path_len > len) {
			return (ENAMETOOLONG);
		}
	}

	path_len = strlcat(path, name, len);

	if (path_len > len) {
		return (ENAMETOOLONG);
	}
	return (0);
}
