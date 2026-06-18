/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LIB_PTHREAD_SEMA_H_
#define _LIB_PTHREAD_SEMA_H_

struct _sem_st {
	unsigned int	usem_magic;

	LIST_ENTRY(_sem_st) usem_list;
	semid_t 		usem_semid;
	sem_t			*usem_identity;
	/* Protects data below. */
	pthread_spin_t	usem_interlock;

	struct pthread_queue_t usem_waiters;
	unsigned int	usem_count;

	volatile int 	usem_waitcount;
};

#define	USEM_MAGIC	0x09fa4012
#define	USEM_USER	0		/* assumes kernel does not use NULL */

__BEGIN_DECLS
int sem_errorcheck(sem_t *);
int sema_alloc(int, unsigned int, semid_t, size_t, sem_t *);
void sema_free(sem_t);
int sema_destroy(sem_t *);
int sema_open(const char *, int, mode_t, unsigned int, size_t, semid_t *);
int sema_close(sem_t *, size_t);
int sema_unlink(const char *);
int sema_trywait(sem_t *);
int sema_post(sem_t *);
int sema_getvalue(sem_t *, int *);
__END_DECLS

#endif /* _LIB_PTHREAD_SEM_H_ */
