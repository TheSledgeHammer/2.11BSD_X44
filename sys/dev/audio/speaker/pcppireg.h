/* $NetBSD: pcppireg.h,v 1.1 1998/04/15 20:26:18 drochner Exp $ */

/*
 * PPI speaker control values
 */

#define PITAUX_PORT		0x61	/* port of Programmable Peripheral Interface */
#define PIT_ENABLETMR2	0x01	/* Enable timer/counter 2 */
#define PIT_SPKRDATA	0x02	/* Direct to speaker */

#define PIT_SPKR		(PIT_ENABLETMR2|PIT_SPKRDATA)
