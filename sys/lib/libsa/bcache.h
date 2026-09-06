/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
 * Copyright 2015 Toomas Soome <tsoome@me.com>
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

#ifndef _LIBSA_BCACHE_H_
#define _LIBSA_BCACHE_H_

/*
 * Disk block cache
 */
struct bcache_devdata {
    int     (*dv_strategy)(void *, int, daddr_t, size_t, char *, size_t *);
    void	*dv_devdata;
    void	*dv_cache;
};

/* bcache.c */
void	bcache_init(size_t nblks, size_t bsize);
void	bcache_add_dev(int);
void	*bcache_allocate(void);
void	bcache_free(void *);
int		bcache_strategy(void *, int, daddr_t, size_t, char *, size_t *);
void	bcache_stats(void);
#endif /* _LIBSA_BCACHE_H_ */
