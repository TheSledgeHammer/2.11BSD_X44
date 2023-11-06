/*
 * The 3-Clause BSD License:
 * Copyright (c) 2021 Martin Kelly
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

#ifndef _SYS_MUTEX_H_
#define _SYS_MUTEX_H_

struct mtx {
	struct lock_object	*mtx_lock;
	struct lock_holder 	*mtx_holder;
	char 				*mtx_name;
};

#ifdef _KERNEL
struct lock_holder;
struct lock_object;
struct pgrp;

void mtx_init(struct mtx *, struct lock_holder *, char *, void *, pid_t);
void mtx_lock(struct mtx *, struct lock_holder *);
void mtx_unlock(struct mtx *, struct lock_holder *);
int  mtx_lock_try(struct mtx *);
int  mtx_owned(struct mtx *);
void *mtx_owner(struct mtx *);
#endif /* _KERNEL */
#endif /* _SYS_MUTEX_H_ */
