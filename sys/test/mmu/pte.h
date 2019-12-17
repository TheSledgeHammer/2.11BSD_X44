/*
 * Machine-Independent Page Directory, Page Table Entries & Page Frame
 * For Machine-Dependent go to machine/pte.h
 */
/* Using Hash Algorithm/ Tree Structure
 * Calculating Number of PTE's
 */

/* Page Directory */
struct pd {
	int page_size;			/* 0 = 4Kib. 			1 = 4Mib */
	int accessed;			/* 0 = Not Accessed		1 = Accessed */
	int cache_disabled;		/* 0 = Cache Enabled.	1 = Cache Disabled */
	int write_through;		/* 0 = Write-Back. 		1 = Write-Through */
	int usr_supervisor;		/* 0 = Supervisor Only. 1 = All */
	int read_write;			/* 0 = Read-Only. 		1 = Read/Write */
	int present;			/* 0 = Present. 		1 = Not Present */
};

/* Page Table Entry */
struct pte {
	int page_size;			/* 0 = 4Kib. 			1 = 4Mib */
	int accessed;			/* 0 = Not Accessed		1 = Accessed */
	int cache_disabled;		/* 0 = Cache Enabled.	1 = Cache Disabled */
	int write_through;		/* 0 = Write-Back. 		1 = Write-Through */
	int usr_supervisor;		/* 0 = Supervisor Only. 1 = All */
	int read_write;			/* 0 = Read-Only. 		1 = Read/Write */
	int present;			/* 0 = Present. 		1 = Not Present */

	/* Unique pte bits */
	int cached;
	int global_flag;
	int dirty_flag;
	int pat;
};

/* Page Frame */
struct pf {

};


