/*
 * pmap_nopae3.h
 *
 *  Created on: 23 Oct 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_ARCH_I386_PMAP_NOPAE3_H_
#define SYS_DEVEL_ARCH_I386_PMAP_NOPAE3_H_

#define	NTRPPTD			1
#define	LOWPTDI			1
#define	KERNPTDI		2

#define NPGPTD			1
#define NPGPTD_SHIFT	10

#define	PG_FRAME		PG_FRAME_NOPAE
#define	PG_PS_FRAME		PG_PS_FRAME_NOPAE

#define KVA_PAGES		(256*4)

#ifndef NKPT
#define	NKPT			30
#endif

typedef uint32_t pd_entry_t;
typedef uint32_t pt_entry_t;
typedef	uint32_t pdpt_entry_t;	/* Only to keep struct pmap layout. */

extern pt_entry_t PTmap[];
extern pd_entry_t PTD[];
extern pd_entry_t PTDpde[];
extern pd_entry_t *IdlePTD_nopae;
extern pt_entry_t *KPTmap_nopae;

#endif /* SYS_DEVEL_ARCH_I386_PMAP_NOPAE3_H_ */
