/*
 * tlb.h
 *
 *  Created on: 16 Dec 2019
 *      Author: marti
 */
/*
 * Machine-Independent Translation Lookaside Buffer
 * For Machine-Dependent go to machine/tlb.h
 */
#ifndef SYS_MMU_TLB_H_
#define SYS_MMU_TLB_H_

/* Should be in separate header */
/* Translation Lookaside Buffer (TLB) */
struct tlb {
	struct u_tlb	*utlb;	/* unified tlb */
	struct i_tlb 	*itlb; 	/* instruction tlb */
	struct d_tlb 	*dtlb; 	/* data tlb */
	struct l2_tlb 	*l2tlb; /* level 2 tlb */
};

/* Unified TLB */
struct u_tlb {

};

/* Instruction TLB */
struct i_tlb {

};

/* Data TLB */
struct d_tlb {

};

/* Level 2 TLB */
struct l2_tlb {

};


#endif /* SYS_TEST_MMU_TLB_H_ */
