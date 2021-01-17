/*
 * testbus.c
 *
 *  Created on: 15 Jan 2021
 *      Author: marti
 */


struct resource_bus {
	bus_space_tag_t 	rb_iot;			/* isa i/o space tag */
	bus_space_tag_t 	rb_memt;		/* isa mem space tag */
	bus_dma_tag_t 		rb_dmat;		/* DMA tag */

	bus_addr_t			rb_addr;		/* addr */
	bus_size_t			rb_size;		/* size */

	bus_space_handle_t	rb_ioh0;		/* io handle 0 */
	bus_space_handle_t 	rb_ioh1;		/* io handle 1 */
};

struct resource_address {
	int					ra_iobase;		/* base i/o address */
	int					ra_iosize;		/* span of ports used */
    int					ra_irq;			/* interrupt request */
	int					ra_drq;			/* DMA request */
	int					ra_drq2;		/* second DMA request */
	int					ra_maddr;		/* physical i/o mem addr */
	u_int				ra_msize;		/* size of i/o memory */
};

typedef struct resource_bus 	resource_bus_t;
typedef struct resource_address resource_address_t;

struct uba {
	resource_bus_t 		uba_bus;
	resource_address_t 	uba_addr;
};

uba_resource_bus()
{
	struct uba *uba;
	uba->uba_bus
}

void
uba_resource(uba, iotag, memtag, dmatag, addr, size)
	struct uba 		*uba;
	bus_space_tag_t iotag;
	bus_space_tag_t	memtag;
	bus_dma_tag_t	dmatag;
	bus_addr_t		addr;
	bus_size_t		size;

{
	register resource_bus_t *rbus;

	rbus = uba->uba_bus;

	rbus->rb_iot = iotag;
	rbus->rb_memt = memtag;
	rbus->rb_dmat = dmatag;
	rbus->rb_addr = addr;
	rbus->rb_size = size;
}

/* 2.11BSD mch_xxx.s & seg.h */
#define RW		06		/* Read and write */
#define	KISD5	((u_short *) 0172312)
#define KDSD5	KISD5
#define KDSA5	((u_short *) 0172372)

#define	SEG5	((caddr_t) 0120000)

struct segm_reg {
	u_int	se_desc;
	u_int	se_addr;
};

typedef struct segm_reg segm;
typedef struct segm_reg mapinfo[2];	/* KA5, KA6 */

#define mapseg5(addr,desc) { 	\
	*KDSD5 = desc; 				\
	*KDSA5 = addr; 				\
}

#define saveseg5(save) { 		\
	save.se_addr = *KDSA5; 		\
	save.se_desc = *KDSD5; 		\
}

#define restorseg5(save) { 		\
	*KDSD5 = save.se_desc; 		\
	*KDSA5 = save.se_addr; 		\
}

/* restore normal kernel map for seg5. */
extern segm	seg5;		/* prototype KDSA5, KDSD5 */
#define normalseg5()	restorseg5(seg5)

#include <sys/map.h>
caddr_t
mapin(bp)
	struct buf *bp;
{
	register u_int paddr;
	register u_int offset;
#ifdef DIAGNOSTIC
	if (hasmap) {
		printf("mapping %o over %o\n", bp, hasmap);
		panic("mapin");
	}
	hasmap = bp;
#endif
	offset = bp->b_un.b_addr & 077;
	paddr = bftopaddr(bp);
	mapseg5((u_short)paddr, (u_short)(((u_int)DEV_BSIZE << 2) | (u_int)RW));

	return (SEG5 + offset);
}

#ifdef DIAGNOSTIC
void
mapout(bp)
	struct buf *bp;
{
	if (bp != hasmap) {
		printf("unmapping %o, not %o\n", bp, hasmap);
	 	panic("mapout");
	}
	hasmap = NULL;
	normalseg5();
}
#endif
