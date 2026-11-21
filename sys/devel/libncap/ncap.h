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
 *	@(#)ncap.h 1.0 	18/11/26
 */

#ifndef _LIBNCAP_NCAP_H_
#define _LIBNCAP_NCAP_H_

#define NCAP_ERRBUF_SIZE 256

typedef struct ncap ncap_t;
typedef struct ncap_table ncap_table_t;

struct ncap {
	int fd;
	struct nbpf_program ncode;
	char errbuf[NCAP_ERRBUF_SIZE + 1];
};

struct ncap_table {
	int cmd;
	u_int tid;
	int alen;
	nbpf_addr_t addr;
	nbpf_netmask_t mask;
	int type;
};

/* ncap.c */
void *zalloc(size_t);
void *xrealloc(void *, size_t);
int ncap_strerror(int);

/* ncap_bpf.c */
int bpf_ncap_enable(ncap_t *);
int bpf_ncap_disable(ncap_t *);

/* ncap_ncgen.c */
/*
 * N-code generation interface.
 */

typedef struct ncap_ctx ncap_ctx_t;

#define	NCAP_MATCH_DST		0x01
#define	NCAP_MATCH_SRC		0x02

#define	NCAP_MATCH_TCP		0x04
#define	NCAP_MATCH_UDP		0x08
#define	NCAP_MATCH_ICMP		0x10
#define	NCAP_MATCH_ICMP6	0x20

ncap_ctx_t *ncap_ncgen_create(void);
void *ncap_ncgen_complete(ncap_ctx_t *, size_t *);
void ncap_ncgen_print(const void *, size_t);

void ncap_ncgen_group(ncap_ctx_t *);
void ncap_ncgen_endgroup(ncap_ctx_t *);

void ncap_gennc_v4cidr(ncap_ctx_t *, int, const nbpf_addr_t *, const nbpf_netmask_t);
void ncap_gennc_v6cidr(ncap_ctx_t *, int, const nbpf_addr_t *, const nbpf_netmask_t);
void ncap_gennc_ports(ncap_ctx_t *, int, in_port_t, in_port_t);
void ncap_gennc_icmp(ncap_ctx_t *, int, int);
void ncap_gennc_icmp6(ncap_ctx_t *, int, int);
void ncap_gennc_tbl(ncap_ctx_t *, int, u_int);
void ncap_gennc_tcpfl(ncap_ctx_t *, uint8_t, uint8_t);
void ncap_gennc_proto(ncap_ctx_t *, uint8_t, uint8_t);

/* ncap_disassemble.c */
/*
 * N-code disassembler.
 */

typedef struct ncap_inf ncap_inf_t;

ncap_inf_t *ncap_ncode_disinf(FILE *);
int ncap_ncode_disassemble(ncap_inf_t *, const void *, size_t);

/* ncap_table.c */
int ncap_table_add(ncap_t *, ncap_table_t *);
int ncap_table_remove(ncap_t *, ncap_table_t *);
int ncap_table_lookup(ncap_t *, ncap_table_t *);
int ncap_table_create(ncap_t *, ncap_table_t *);
int ncap_table_destroy(ncap_t *, ncap_table_t *);

#endif /* _LIBNCAP_NCAP_H_ */
