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
 *
 *	@(#)ncap_bpf.c 1.0 	18/11/26
 */

#include <sys/file.h>
#include <sys/ioctl.h>

#include <net/bpf.h>

#include "ncap.h"

int
bpf_ncap_enable(ncap_t *n)
{
	int error;

	error = ioctl(n->fd, BIOCSTARTNBPF, 1);
    if (error < 0) {
    	(void)snprintf(n->errbuf, NCAP_ERRBUF_SIZE, "BIOCSTARTNBPF: %s", ncap_strerror(errno));
        return (error);
    }
    return (0);
}

int
bpf_ncap_disable(ncap_t *n)
{
	int error;

	error = ioctl(n->fd, BIOCSTOPNBPF, 0);
    if (error < 0) {
    	(void)snprintf(n->errbuf, NCAP_ERRBUF_SIZE, "BIOCSTOPNBPF: %s", ncap_strerror(errno));
        return (error);
    }
    return (0);
}

