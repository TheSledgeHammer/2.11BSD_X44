/*
 * Sourced from Unix V7x86
 * Machine-dependent bits and macros
 */
#define PGSH  12
#define PGSZ  4096

#define USTK  0x400000
#define PHY   0x40000000

#define KBASE 0x7fc00000
#define KSTK  0x7ffa0000

#define	USERMODE(cs)	(((cs)&0xffff)!=0x10)
#define	BASEPRI(pl)	((pl)!=0xffff)