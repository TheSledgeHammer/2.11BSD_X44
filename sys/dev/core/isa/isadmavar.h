/*	$NetBSD: isadmavar.h,v 1.10 1997/08/04 22:13:33 augustss Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#define MAX_ISADMA		65536

#define	DMAMODE_WRITE	0
#define	DMAMODE_READ	1
#define	DMAMODE_LOOP	2

struct isa_mem {
	struct device 			*isadev;
	int 					chan;
	bus_size_t 				size;
	bus_addr_t 				addr;
	caddr_t 				kva;
	struct isa_mem 			*next;
} *isa_mem_head = 0;

struct proc;

void	   	isa_dmacascade (struct device *, int);

int	  		isa_dmamap_create (struct device *, int, bus_size_t, int);
void	   	isa_dmamap_destroy (struct device *, int);

int	   		isa_dmastart (struct device *, int, void *, bus_size_t, struct proc *, int, int);
void	   	isa_dmaabort (struct device *, int);
bus_size_t 	isa_dmacount (struct device *, int);
int	   		isa_dmafinished (struct device *, int);
void		isa_dmadone (struct device *, int);

int	   		isa_dmamem_alloc (struct device *, int, bus_size_t, bus_addr_t *, int);
void		isa_dmamem_free (struct device *, int, bus_addr_t, bus_size_t);
int	   		isa_dmamem_map (struct device *, int, bus_addr_t, bus_size_t, caddr_t *, int);
void		isa_dmamem_unmap (struct device *, int, caddr_t, size_t);
int	   		isa_dmamem_mmap (struct device *, int, bus_addr_t, bus_size_t, int, int, int);

int	   		isa_drq_isfree (struct device *, int);

void    	*isa_malloc (struct device *, int, size_t, int, int);
void		isa_free (void *, int);
int	   		isa_mappage (void *, int, int);
