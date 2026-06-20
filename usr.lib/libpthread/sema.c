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
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sha2.h>
#include <unistd.h>

#include "pthread.h"
#include "pthread_int.h"
//#include "sema.h"

struct _sem_st {
	unsigned int	usem_magic;
#define	USEM_MAGIC	0x09fa4012

	LIST_ENTRY(_sem_st) usem_list;
	semid_t			usem_semid;	/* 0 -> user (non-shared) */
#define	USEM_USER	0		/* assumes kernel does not use NULL */
	sem_t			*usem_identity;

	/* Protects data below. */
	pthread_spin_t	usem_interlock;

	struct pthread_queue_t usem_waiters;
	unsigned int	usem_count;
	volatile int 	usem_waitcount;
};

typedef struct _sem_st *sem_t;

//static char const *sem_prefix = "/var/run/sem";

static char const *sem_prefix = "/tmp/%s.sem";

#define SEMID_PROC 		0
#define SEMID_FORK		1
#define SEMID_NAMED		2
#define SEMID_USER 		SEMID_PROC

#define SEM_PATH_SIZE 	(5 + SHA256_DIGEST_STRING_LENGTH + 4)
#define SEM_MMAP_SIZE 	(getpagesize())


static int get_path(const char *name, char *path, size_t len, char const **prefix);

static int sema_create(int, semid_t *, sem_t *);

static int
sema_create(int pshared, semid_t *semid, sem_t *semp)
{
	sem_t sem, sem_base;
	int sem_count;

	if (pshared) {
		sem_base = mmap(NULL, SEM_MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
		sem_count = (SEM_MMAP_SIZE / sizeof(*sem));
		sem = sem_base++;
		if (--sem_count == 0) {
			sem_base = NULL;
		}
		*semid = SEMID_FORK;
	} else {
		sem = malloc(sizeof(struct _sem_st));
		*semid = SEMID_PROC;
	}
	if (sem == NULL) {
		errno = ENOSPC;
		return (ENOSPC);
	}
	*semp = sem;
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
sema_init(sem_t *sem, int pshared, unsigned int value)
{
	semid_t	semid;
	int error;

	error = sema_create(pshared, &semid, sem);
	if (error != 0) {
		errno = error;
		return (-1);
	}
	error = sem_alloc(value, semid, sem);
	if (error != 0) {
		if (semid != SEMID_USER) {
			sema_destroy(sem);
		}
		errno = error;
		return (-1);
	}
	return (0);
}

int
sema_destroy(sem_t *sem)
{
	if ((*sem)->usem_waitcount) {
		errno = EBUSY;
		return (-1);
	}

	sem_free(*sem);
	return (0);
}

static void
makesempath(const char *origpath, char *sempath, size_t len)
{
	char buf[SHA256_DIGEST_STRING_LENGTH];

	SHA256Data(origpath, strlen(origpath), buf);
	snprintf(sempath, len, sem_prefix, buf);
}

int
sema_open(const char *name, int oflag, mode_t mode, unsigned int value, semid_t *idp, sem_t sem)
{
	struct stat sb;
	char sempath[SEM_PATH_SIZE];
	char const *prefix = NULL;
	size_t path_len;
	int error, fd;

	makesempath(name, sempath, sizeof(sempath));
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
	if (sb.st_size != (off_t)SEM_MMAP_SIZE) {
		if (!(oflag & O_CREAT)) {
			error = EINVAL;
			goto bad;
		}
		if (sb.st_size != 0) {
			error = EINVAL;
			goto bad;
		}
		if (ftruncate(fd, SEM_MMAP_SIZE) == -1) {
			error = EINVAL;
			goto bad;
		}
	}

	sem = mmap(NULL, SEM_MMAP_SIZE, (PROT_READ | PROT_WRITE), (MAP_ANON | MAP_SHARED), fd, 0);
	if (sem == MAP_FAILED) {
		error = EINVAL;
		goto bad;
	}
	sem->usem_count = value;
	sem->usem_semid = SEMID_NAMED;
	*idp = sem->usem_semid;
	return (0);

bad:
	close(fd);
	errno = error;
	return (-1);
}

int
sema_close(sem_t *sem)
{
	if ((*sem)->usem_semid == -1) {
		errno = EINVAL;
		return (-1);
	}

	munmap(sem, SEM_MMAP_SIZE);
	free(sem);
	return (0);
}

int
sema_unlink(const char *name)
{
	char sempath[SEM_PATH_SIZE];
	int error;

	makesempath(name, sempath, sizeof(sempath));
	error = unlink(sempath);
	if (error != 0) {
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
	int error, timo;

	atomic_inc_int((*sem)->usem_waitcount);
	for (;;) {
		while ((val = (*sem)->usem_count) > 0) {
			if (atomic_cas_uint(&(*sem)->usem_count, val, val - 1) == val) {
				membar_enter_after_atomic();
				atomic_dec_int((*sem)->usem_waitcount);
				return (0);
			}
		}
		/* missing time shit here */
		error = ts2timo(CLOCK_REALTIME, TIMER_ABSTIME, abstime, &timo, NULL);
		if (error != 0) {
			goto out;
		}
	}

out:
	atomic_dec_int((*sem)->usem_waitcount);
	return (error);
}
#endif

int
sema_trywait(sem_t *sem)
{
	unsigned int val;

	while ((val = (*sem)->usem_count) > 0) {
		if (atomic_cas_uint(&(*sem)->usem_count, val, val - 1) == val) {
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
	membar_exit_before_atomic();
	atomic_inc_int(&(*sem)->usem_count);
	return (0);
}

int
sema_getvalue(sem_t *sem, int *sval)
{
	sval = (int)(*sem)->usem_count;
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

/*
 * Calculate delta and convert from struct timespec to the ticks.
 */
static int
ts2timo(clockid_t clock_id, int flags, struct timespec *ts, int *timo, struct timespec *start)
{
	int error;
	struct timespec tsd;

	if (ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000L) {
		return (EINVAL);
	}

	if ((flags & TIMER_ABSTIME) != 0 || start != NULL) {
		error = clock_gettime(clock_id, &tsd);
		if (error != 0) {
			return (error);
		}
		if (start != NULL) {
			*start = tsd;
		}
	}

	if ((flags & TIMER_ABSTIME) != 0) {
		if (timespeccmp(ts, &tsd, <) || timespeccmp(ts, &tsd, >)) {
			return (EINVAL);
		}
		timespecsub(ts, &tsd, &tsd);
		ts = &tsd;
	}

	error = itimespecfix(ts);
	if (error != 0) {
		return (error);
	}

	if (ts->tv_sec == 0 && ts->tv_nsec == 0) {
		return (ETIMEDOUT);
	}

	*timo = tstohz(ts);
	DIAGASSERT(timo > 0);
	return (0);
}

static int
tstohz(const struct timespec *ts)
{
	struct timeval tv;

	/*
	 * usec has great enough resolution for hz, so convert to a
	 * timeval and use tvtohz() above.
	 */
	tv.tv_sec = ts->tv_sec;
	tv.tv_usec = (ts->tv_nsec + 999)/1000;
	if (tv.tv_usec >= 1000000) {
		if (__predict_false(tv.tv_sec == sizeof(time_t))) {
			return (INT_MAX);
		}
		tv.tv_sec++;
		tv.tv_usec -= 1000000;
	}
	return (tvtohz(&tv));
}

sem_t
sem_get(semid_t semid)
{
	sem_t sem;

	pthread_mutex_lock(&named_sems_mtx);
	LIST_FOREACH(sem, &named_sems, usem_list) {
		if (sem->usem_semid == semid) {
			pthread_mutex_unlock(&named_sems_mtx);
			return (sem->usem_identity);
		}
	}
	pthread_mutex_unlock(&named_sems_mtx);
	return (sem);
}
