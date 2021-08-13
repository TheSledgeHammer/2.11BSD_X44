/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 *
 * @(#)ufs211_bufmap.c	1.00
 */

#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/user.h>

#include <ufs211_fs.h>

struct ufs211_bufmap *ufs211buf;

void
bufmap_init(void *)
{
	MALLOC(&ufs211buf, struct ufs211_bufmap, sizeof(struct ufs211_bufmap *), M_UFS211, M_WAITOK);
}

/* mapin buf */
void
ufs211_mapin(bp)
    struct buf *bp;
{
    if(bufmap_alloc(bp)) {
        printf("buf allocated\n");
    }
}

/* mapout buf */
void
ufs211_mapout(bp)
    struct buf *bp;
{
    if(bufmap_free(bp) == NULL) {
        printf("buf freed\n");
    }
}

/*
 * [Internal use only]
 * allocate bufmap data
 */
static void *
bufmap_alloc(data)
    void  *data;
{
    struct ufs211_bufmap *bm = &ufs211buf;
    if(bm) {
        bm->bm_data = data;
        bm->bm_size = sizeof(data);
        printf("data allocated \n");
    }
    return (bm->bm_data);
}

/*
 * [Internal use only]
 * free bufmap data
 */
static void *
bufmap_free(data)
    void  *data;
{
    struct ufs211_bufmap *bm = &ufs211buf;

    if(bm->bm_data == data) {
    	if(bm->bm_data != NULL && bm->bm_size == sizeof(data)) {
    		bm->bm_data = NULL;
    		bm->bm_size = 0;
    		printf("data freed\n");
    	}
    }
    return (bm->bm_data);
}
