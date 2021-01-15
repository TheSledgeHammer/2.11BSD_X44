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

iobase(uba)
	struct uba *uba;
{
	uba->uba_bus->rb_iot = tag;
}
