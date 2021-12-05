/*	$NetBSD: bus.h,v 1.13 2020/03/08 02:42:00 thorpej Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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

#ifndef _SYS_BUS_H_
#define	_SYS_BUS_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#include <machine/bus.h>

/* bus_space(9) */

/* Map types. */
#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02
#define	BUS_SPACE_MAP_PREFETCHABLE	0x04

/* Bus read/write barrier methods. */
#define	BUS_SPACE_BARRIER_READ		0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE		0x02		/* force write barrier */

int			bus_space_map(bus_space_tag_t, bus_addr_t, bus_size_t, int, bus_space_handle_t *);
void		bus_space_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);
int			bus_space_subregion(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_size_t, bus_space_handle_t *);
int			bus_space_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
void		bus_space_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);
caddr_t		bus_space_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
void		*bus_space_vaddr(bus_space_tag_t, bus_space_handle_t);
//void		bus_space_barrier(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_size_t, int);

u_int8_t	bus_space_read_1(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int16_t 	bus_space_read_2(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int32_t 	bus_space_read_4(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int64_t 	bus_space_read_8(bus_space_tag_t, bus_space_handle_t, bus_size_t);

u_int8_t	bus_space_read_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int16_t 	bus_space_read_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int32_t 	bus_space_read_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int64_t 	bus_space_read_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t);

void		bus_space_read_multi_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, uint8_t *, size_t);
void		bus_space_read_multi_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, uint16_t *, size_t);
void		bus_space_read_multi_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *, size_t);
void		bus_space_read_multi_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *, size_t);

void		bus_space_read_multi_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t *, size_t);
void		bus_space_read_multi_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t *, size_t);
void		bus_space_read_multi_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *, size_t);
void		bus_space_read_multi_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *, size_t);

void		bus_space_read_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int8_t *, size_t);
void		bus_space_read_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int16_t *, size_t);
void		bus_space_read_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int32_t *, size_t);
void		bus_space_read_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int64_t *, size_t);

void		bus_space_read_region_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int8_t *, size_t);
void		bus_space_read_region_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int16_t *, size_t);
void		bus_space_read_region_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int32_t *, size_t);
void		bus_space_read_region_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int64_t *, size_t);

void		bus_space_write_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t);
void		bus_space_write_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t);
void		bus_space_write_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t);
void		bus_space_write_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t);

void		bus_space_write_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t);
void		bus_space_write_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t);
void		bus_space_write_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t);
void		bus_space_write_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t);

void		bus_space_write_multi_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int8_t *, size_t);
void		bus_space_write_multi_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int16_t *, size_t);
void		bus_space_write_multi_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int32_t *, size_t);
void		bus_space_write_multi_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int64_t *, size_t);

void		bus_space_write_multi_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int8_t *, size_t);
void		bus_space_write_multi_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int16_t *, size_t);
void		bus_space_write_multi_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int32_t *, size_t);
void		bus_space_write_multi_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int64_t *, size_t);

void		bus_space_write_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int8_t *, size_t);
void		bus_space_write_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int16_t *, size_t);
void		bus_space_write_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int32_t *, size_t);
void		bus_space_write_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int64_t *, size_t);

void		bus_space_set_multi_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t, size_t);
void		bus_space_set_multi_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t, size_t);
void		bus_space_set_multi_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t, size_t);
void		bus_space_set_multi_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t, size_t);

void		bus_space_set_multi_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t, size_t);
void		bus_space_set_multi_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t, size_t);
void		bus_space_set_multi_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t, size_t);
void		bus_space_set_multi_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t, size_t);

void		bus_space_set_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t, size_t);
void		bus_space_set_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t, size_t);
void		bus_space_set_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t, size_t);
void		bus_space_set_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t, size_t);

void		bus_space_set_region_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t, size_t);
void		bus_space_set_region_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t, size_t);
void		bus_space_set_region_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t, size_t);
void		bus_space_set_region_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t, size_t);

void		bus_space_copy_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);

void		bus_space_copy_region_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);

#endif	/* _SYS_BUS_H_ */
