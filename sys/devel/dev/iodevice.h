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

/*
 * IO (Keyboard, Mouse, Terminal, etc) Generic Driver.
 * To provide a generic device chain API.
 * So that a single driver that can have multiple devices can be easily daisy chained.
 */

#ifndef SYS_DEVEL_DEV_IODEVICE_H_
#define SYS_DEVEL_DEV_IODEVICE_H_

#include <sys/queue.h>


struct io_driver {
	char					*io_name;
	struct  device 			*io_dev;		/* back pointer */
};

struct io_controller {
	char					*ioc_name;
	int						ioc_index;
};

struct io_device {
	char					*iod_name;
	int						iod_flags;
	int						iod_unit;
	//iod_minor;

	int 					iod_iobase;
	int						iod_iosize;		/* span of ports used */
	int						iod_irq;		/* interrupt request */
	int						iod_drq;		/* DMA request */
	int						iod_drq2;		/* second DMA request */
	int						iod_maddr;		/* physical i/o mem addr */
	u_int					iod_msize;		/* size of i/o memory */

	bus_space_tag_t			iod_iot;		/* device space tag */
	bus_space_handle_t 		iod_ioh;		/* device mem space tag */
	bus_dma_tag_t 			iod_dmat;		/* DMA tag */

	void					*iod_aux;		/* driver specific */
};

#endif /* SYS_DEVEL_DEV_IODEVICE_H_ */
