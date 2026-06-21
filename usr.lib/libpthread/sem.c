/*	$NetBSD: sem.c,v 1.7 2003/11/24 23:54:13 cl Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of Wasabi Systems, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
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

#define USE_KSEM /* Sema functions not ready */

#include <sys/cdefs.h>
__RCSID("$NetBSD: sem.c,v 1.7 2003/11/24 23:54:13 cl Exp $");

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
 /* Implements kernel semaphores */
#ifdef USE_KSEM
#include "pthread_syscalls.h"
#else /* !USE_KSEM */

#define SEM_PATH_SIZE 	(5 + SHA256_DIGEST_STRING_LENGTH + 4)
#define SEM_MMAP_SIZE 	(getpagesize())

static const char *sem_prefix = "/tmp/%s.sem";
#endif

struct _sems_st {
	unsigned int	usem_magic;

	LIST_ENTRY(_sem_st) usem_list;
	semid_t			usem_semid;	/* 0 -> user (non-shared) */

	sem_t			*usem_identity;

	/* Protects data below. */
	pthread_spin_t	usem_interlock;

	struct pthread_queue_t usem_waiters;
	unsigned int	usem_count;
#ifndef USE_KSEM
	volatile int 	usem_waitcount;
#endif
};

#define	USEM_MAGIC		0x09fa4012
#define	USEM_USER		0		/* assumes kernel does not use NULL */

#ifndef USE_KSEM
#define	SEM_MAGIC		USEM_MAGIC
#define SEMID_PROC 		USEM_USER
#define SEMID_FORK		1
#define SEMID_NAMED		2
#endif

static LIST_HEAD(, _sem_st) named_sems = LIST_HEAD_INITIALIZER(&named_sems);
static pthread_mutex_t named_sems_mtx = PTHREAD_MUTEX_INITIALIZER;

static int sem_errorcheck(sem_t *);
static int sem_alloc(unsigned int, semid_t, sem_t *);
static void sem_free(sem_t);
static sem_t sem_lookup(semid_t);
int sem_timedwait(sem_t *, const struct timespec *);

#ifndef USE_KSEM
static int sema_create(int, semid_t *, sem_t *);
static int sema_destroy(semid_t);
static void makesempath(const char *, char *, size_t);
static int sema_open(const char *, int, mode_t, unsigned int, semid_t *, sem_t);
static int sema_close(semid_t);
static int sema_unlink(const char *);
static int sema_wait(semid_t);
static int sema_timedwait(semid_t, struct timespec *);
static int sema_trywait(semid_t);
static int sema_getvalue(semid_t, int *);
static int sema_post(semid_t);
static int sema_do_wait(sem_t, bool_t, struct timespec *);
static int ts2timo(clockid_t, int, struct timespec *, int *, struct timespec *);
static int tstohz(const struct timespec *);
#endif

static int
sem_errorcheck(sem_t *sem)
{
#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->usem_magic != USEM_MAGIC) {
		return (-1);
	}
#endif
	return (0);
}

static int
sem_alloc(unsigned int value, semid_t semid, sem_t *semp)
{
	sem_t sem;

	if (value > SEM_VALUE_MAX) {
		return (EINVAL);
	}

	sem = malloc(sizeof(struct _sem_st));
	if (sem == NULL) {
		return (ENOSPC);
	}

	sem->usem_magic = USEM_MAGIC;
	pthread_lockinit(&sem->usem_interlock);
	PTQ_INIT(&sem->usem_waiters);
	sem->usem_count = value;
	sem->usem_semid = semid;
	*semp = sem;
	return (0);
}

static void
sem_free(sem_t sem)
{
	sem->usem_magic = 0;
	free(sem);
}

static sem_t
sem_lookup(semid_t semid)
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
	return (NULL);
}

int
sem_init(sem_t *sem, int pshared, unsigned int value)
{
	semid_t	semid;
	int error;

#ifdef USE_KSEM
	semid = USEM_USER;
	if (pshared && pthread_sys_ksem_init(value, &semid) == -1) {
		return (-1);
	}
#else
	error = sema_create(pshared, &semid, sem);
	if (error != 0) {
		errno = error;
		return (-1);
	}
#endif
	error = sem_alloc(value, semid, sem);
	if (error != 0) {
		if (semid != USEM_USER) {
#ifdef USE_KSEM
			pthread_sys_ksem_destroy(semid);
#else
			sema_destroy(semid);
#endif
		}
		errno = error;
		return (-1);
	}

	return (0);
}

int
sem_destroy(sem_t *sem)
{
	pthread_t self;

	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}

	if ((*sem)->usem_semid != USEM_USER) {
#ifdef USE_KSEM
		if (pthread_sys_ksem_destroy((*sem)->usem_semid)) {
#else
		if (sema_destroy((*sem)->usem_semid)) {
#endif
			return (-1);
		}
	} else {
		self = pthread__self();
		pthread_spinlock(self, &(*sem)->usem_interlock);
		if (!PTQ_EMPTY(&(*sem)->usem_waiters)) {
			pthread_spinunlock(self, &(*sem)->usem_interlock);
			errno = EBUSY;
			return (-1);
		}
		pthread_spinunlock(self, &(*sem)->usem_interlock);
	}

	sem_free(*sem);
	return (0);
}

sem_t *
sem_open(const char *name, int oflag, ...)
{
	sem_t *sem, s;
#ifndef USE_KSEM
	sem_t tmp;
#endif
	semid_t semid;
	mode_t mode;
	unsigned int value;
	int error;
	va_list ap;

	mode = 0;
	value = 0;

	if (oflag & O_CREAT) {
		va_start(ap, oflag);
		mode = va_arg(ap, int);
		value = va_arg(ap, unsigned int);
		va_end(ap);
	}

	/*
	 * We can be lazy and let the kernel handle the oflag,
	 * we'll just merge duplicate IDs into our list.
	 */
#ifdef USE_KSEM
	if (pthread_sys_ksem_open(name, oflag, mode, value, &semid) == -1) {
#else
	if (sema_open(name, oflag, mode, value, &semid, tmp) != 0) {
#endif
		return (SEM_FAILED);
	}

	/*
	 * Search for a duplicate ID, we must return the same sem_t *
	 * if we locate one.
	 */
	s = sem_lookup(semid);
	if (s != NULL) {
#ifndef USE_KSEM
		/* free duplicate in mmap */
		munmap(tmp, SEM_MMAP_SIZE);
#endif
		return (s->usem_identity);
	}

	pthread_mutex_lock(&named_sems_mtx);
#ifndef USE_KSEM
	s = tmp;
#endif
	sem = malloc(sizeof(*sem));
	if (sem == NULL) {
#ifndef USE_KSEM
		munmap(s, SEM_MMAP_SIZE);
#endif
		error = ENOSPC;
		goto bad;
	}

	error = sem_alloc(value, semid, sem);
	if (error != 0) {
		goto bad;
	}

	LIST_INSERT_HEAD(&named_sems, *sem, usem_list);
	(*sem)->usem_identity = sem;
	pthread_mutex_unlock(&named_sems_mtx);
	return (sem);

 bad:
	pthread_mutex_unlock(&named_sems_mtx);
#ifdef USE_KSEM
	pthread_sys_ksem_close(semid);
#else
	sema_close(semid);
#endif
	if (sem != NULL) {
		if (*sem != NULL) {
			sem_free(*sem);
		}
		free(sem);
	}
	errno = error;
	return (SEM_FAILED);
}

int
sem_close(sem_t *sem)
{
	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}

	if ((*sem)->usem_semid == USEM_USER) {
		errno = EINVAL;
		return (-1);
	}

	pthread_mutex_lock(&named_sems_mtx);
#ifdef USE_KSEM
	if (pthread_sys_ksem_close((*sem)->usem_semid) == -1) {
#else
	if (sema_close((*sem)->usem_semid) == -1) {
#endif
		pthread_mutex_unlock(&named_sems_mtx);
		return (-1);
	}

	LIST_REMOVE((*sem), usem_list);
	pthread_mutex_unlock(&named_sems_mtx);
	sem_free(*sem);
	free(sem);
	return (0);
}

int
sem_unlink(const char *name)
{
#ifdef USE_KSEM
	return (pthread_sys_ksem_unlink(name));
#else
	return (sema_unlink(name));
#endif
}

int
sem_wait(sem_t *sem)
{
	pthread_t self;

	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}

	self = pthread__self();

	if ((*sem)->usem_semid != USEM_USER) {
		pthread__testcancel(self);
#ifdef USE_KSEM
		return (pthread_sys_ksem_wait((*sem)->usem_semid));
#else
		return (sema_wait((*sem)->usem_semid));
#endif
	}

	for (;;) {
		pthread_spinlock(self, &(*sem)->usem_interlock);
		pthread_spinlock(self, &self->pt_statelock);
		if (self->pt_cancel) {
			pthread_spinunlock(self, &self->pt_statelock);
			pthread_spinunlock(self, &(*sem)->usem_interlock);
			pthread_exit(PTHREAD_CANCELED);
		}

		if ((*sem)->usem_count > 0) {
			pthread_spinunlock(self, &self->pt_statelock);
			break;
		}

		PTQ_INSERT_TAIL(&(*sem)->usem_waiters, self, pt_sleep);
		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		self->pt_sleepobj = *sem;
		self->pt_sleepq = &(*sem)->usem_waiters;
		self->pt_sleeplock = &(*sem)->usem_interlock;
		pthread_spinunlock(self, &self->pt_statelock);

		/* XXX What about signals? */

		pthread__block(self, &(*sem)->usem_interlock);
		/* interlock is not held when we return */
	}

	(*sem)->usem_count--;

	pthread_spinunlock(self, &(*sem)->usem_interlock);

	return (0);
}

int
sem_timedwait(sem_t *sem, const struct timespec *abstime)
{
	pthread_t self;

	if (sem_errorcheck(sem) != 0 || (abstime == NULL)) {
		errno = EINVAL;
		return (-1);
	}

	self = pthread__self();

	if ((*sem)->usem_semid != USEM_USER) {
#ifdef USE_KSEM
		return (ENOSYS);
#else
		return (sema_timedwait((*sem)->usem_semid, abstime));
#endif
	}

	for (;;) {
		pthread_spinlock(self, &(*sem)->usem_interlock);
		pthread_spinlock(self, &self->pt_statelock);
		if (self->pt_cancel) {
			pthread_spinunlock(self, &self->pt_statelock);
			pthread_spinunlock(self, &(*sem)->usem_interlock);
			pthread_exit(PTHREAD_CANCELED);
		}

		if ((*sem)->usem_count > 0) {
			pthread_spinunlock(self, &self->pt_statelock);
			break;
		}

		PTQ_INSERT_TAIL(&(*sem)->usem_waiters, self, pt_sleep);
		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		self->pt_sleepobj = *sem;
		self->pt_sleepq = &(*sem)->usem_waiters;
		self->pt_sleeplock = &(*sem)->usem_interlock;
		pthread_spinunlock(self, &self->pt_statelock);

		/* XXX What about signals? */

		pthread__block(self, &(*sem)->usem_interlock);
		/* interlock is not held when we return */
	}

	(*sem)->usem_count--;

	pthread_spinunlock(self, &(*sem)->usem_interlock);

	return (0);
}

int
sem_trywait(sem_t *sem)
{
	pthread_t self;

	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}

	if ((*sem)->usem_semid != USEM_USER) {
#ifdef USE_KSEM
		return (pthread_sys_ksem_trywait((*sem)->usem_semid));
#else
		return (sema_trywait((*sem)->usem_semid));
#endif
	}

	self = pthread__self();

	pthread_spinlock(self, &(*sem)->usem_interlock);

	if ((*sem)->usem_count == 0) {
		pthread_spinunlock(self, &(*sem)->usem_interlock);
		errno = EAGAIN;
		return (-1);
	}

	(*sem)->usem_count--;

	pthread_spinunlock(self, &(*sem)->usem_interlock);

	return (0);
}

int
sem_post(sem_t *sem)
{
	pthread_t self, blocked;

	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}

	if ((*sem)->usem_semid != USEM_USER) {
#ifdef USE_KSEM
		return (pthread_sys_ksem_post((*sem)->usem_semid));
#else
		return (sema_post((*sem)->usem_semid));
#endif
	}

	self = pthread__self();

	pthread_spinlock(self, &(*sem)->usem_interlock);
	(*sem)->usem_count++;
	blocked = PTQ_FIRST(&(*sem)->usem_waiters);
	if (blocked) {
		PTQ_REMOVE(&(*sem)->usem_waiters, blocked, pt_sleep);
		/* Give the head of the blocked queue another try. */
		pthread__sched(self, blocked);
	}
	pthread_spinunlock(self, &(*sem)->usem_interlock);

	return (0);
}

int
sem_getvalue(sem_t *sem, int *sval)
{
	pthread_t self;

	if (sem_errorcheck(sem) != 0) {
		errno = EINVAL;
		return (-1);
	}

	if ((*sem)->usem_semid != USEM_USER) {
#ifdef USE_KSEM
		return (pthread_sys_ksem_getvalue((*sem)->usem_semid, sval));
#else
		return (sema_getvalue((*sem)->usem_semid, sval));
#endif
	}

	self = pthread__self();

	pthread_spinlock(self, &(*sem)->usem_interlock);
	*sval = (int)(*sem)->usem_count;
	pthread_spinunlock(self, &(*sem)->usem_interlock);

	return (0);
}

/* Sema: internal sem functions */
#ifndef USE_KSEM

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

static int
sema_destroy(semid_t semid)
{
	sem_t sem;

	sem = sem_lookup(semid);
	if (sem != NULL) {
		if (sem->usem_waitcount) {
			errno = EBUSY;
			return (-1);
		}
		sem_free(sem);
		return (0);
	}
	return (-1);
}

static void
makesempath(const char *origpath, char *sempath, size_t len)
{
	char buf[SHA256_DIGEST_STRING_LENGTH];

	SHA256Data(origpath, strlen(origpath), buf);
	snprintf(sempath, len, sem_prefix, buf);
}

static int
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

static int
sema_close(semid_t semid)
{
	sem_t sem;

	sem = sem_lookup(semid);
	if (sem != NULL) {
		if (sem->usem_semid == -1) {
			errno = EINVAL;
			return (-1);
		}
		munmap(sem, SEM_MMAP_SIZE);
		free(sem);
		return (0);
	}

	errno = EINVAL;
	return (-1);
}

static int
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

static int
sema_wait(semid_t semid)
{
	sem_t sem;
	int error;

	sem = sem_lookup(semid);
	if (sem != NULL) {
		error = sema_do_wait(sem, FALSE, NULL);
		if (error == 0) {
			return (0);
		}
		errno = error;
	} else {
		errno = EAGAIN;
	}
	return (-1);
}

static int
sema_timedwait(semid_t semid, struct timespec *abstime)
{
	sem_t sem;
	int error;

	sem = sem_lookup(semid);
	if (sem != NULL) {
		error = sema_do_wait(sem, FALSE, abstime);
		if (error == 0) {
			return (0);
		}
		errno = error;
	} else {
		errno = EAGAIN;
	}
	return (-1);
}

static int
sema_trywait(semid_t semid)
{
	sem_t sem;
	int error;

	sem = sem_lookup(semid);
	if (sem != NULL) {
		error = sema_do_wait(sem, TRUE, NULL);
		if (error == 0) {
			return (0);
		}
		errno = error;
	} else {
		errno = EAGAIN;
	}
	return (-1);
}

static int
sema_getvalue(semid_t semid, int *sval)
{
	sem_t sem;

	sem = sem_lookup(semid);
	if (sem != NULL) {
		*sval = sem->usem_count;
		return (0);
	}
	return (-1);
}

static int
sema_post(semid_t semid)
{
	sem_t sem;

	sem = sem_lookup(semid);
	if (sem != NULL) {
		membar_exit_before_atomic();
		atomic_inc_int(&sem->usem_count);
		return (0);
	}
	return (-1);
}

static int
sema_do_wait(sem_t sem, bool_t try_p, struct timespec *abstime)
{
	int error, timeo;
	unsigned int val;

	error = 0;
	atomic_inc_int(sem->usem_waitcount);
	while ((val = sem->usem_count) > 0) {
		if (atomic_cas_uint(&sem->usem_count, val, val - 1) == val) {
			membar_enter_after_atomic();
			goto out;
		}
	}
	while ((val = sem->usem_count) == 0) {
		atomic_inc_int(&sem->usem_count);
		if (!try_p && abstime != NULL) {
			error = ts2timo(CLOCK_REALTIME, TIMER_ABSTIME, abstime, &timeo, NULL);
			if (error != 0) {
				goto out;
			}
		} else {
			timeo = 0;
		}
		if (try_p) {
			error = EAGAIN;
		} else {
			error = 0;
		}
		goto out;
	}

out:
	atomic_dec_int(sem->usem_waitcount);
	return (error);
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

#endif /* USE_KSEM */
