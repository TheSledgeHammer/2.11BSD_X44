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
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/mpx.h>

#include <unistd.h>

int
mpx(int cmd, struct mpx *mx, int idx, void *data)
{
	return (__syscall((quad_t)SYS_mpx, cmd, mx, idx, data));
}

int
mpx_create(struct mpx *mx, int idx, void *data)
{
	return (mpx(MPXCREATE, mx, idx, data));
}

int
mpx_put(struct mpx *mx, int idx, void *data)
{
	return (mpx(MPXPUT, mx, idx, data));
}

int
mpx_get(struct mpx *mx, int idx, void *data)
{
	return (mpx(MPXGET, mx, idx, data));
}

int
mpx_destroy(struct mpx *mx, int idx, void *data)
{
	return (mpx(MPXDESTROY, mx, idx, data));
}

int
mpx_remove(struct mpx *mx, int idx, void *data)
{
	return (mpx(MPXREMOVE, mx, idx, data));
}
