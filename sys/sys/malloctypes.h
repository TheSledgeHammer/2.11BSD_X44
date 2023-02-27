/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)malloc.h	8.5 (Berkeley) 5/3/95
 */

#ifndef _SYS_MALLOCTYPES_H_
#define _SYS_MALLOCTYPES_H_

/* flags to malloc */
#define	M_WAITOK		0x0000
#define	M_NOWAIT		0x0001
#define M_CANFAIL		0x0002
#define M_ZERO			0x0004
#define M_OVERLAY		0x0008			/* Allocate to Overlay Space (Note: Only use in tandem with Overlays) */

/* Types of memory to be allocated */
#define	M_FREE			0	/* should be on free list */
#define	M_MBUF			1	/* mbuf */
#define	M_DEVBUF		2	/* device driver memory */
#define	M_SOCKET		3	/* socket structure */
#define	M_PCB			4	/* protocol control block */
#define	M_RTABLE		5	/* routing tables */
#define	M_HTABLE		6	/* IMP host tables */
#define	M_FTABLE		7	/* fragment reassembly header */
#define	M_ZOMBIE		8	/* zombie proc status */
#define	M_IFADDR		9	/* interface address */
#define	M_SOOPTS		10	/* socket options */
#define	M_SONAME		11	/* socket name */
#define	M_NAMEI			12	/* namei path name buffer */
#define	M_GPROF			13	/* kernel profiling buffer */
#define	M_IOCTLOPS		14	/* ioctl data buffer */
#define	M_MAPMEM		15	/* mapped memory descriptors */					/* UN-USED */
#define	M_CRED			16	/* credentials */
#define	M_PGRP			17	/* process group header */
#define	M_SESSION		18	/* session header */
#define	M_IOV			19	/* large iov's */
#define	M_MOUNT			20	/* vfs mount struct */
#define	M_FHANDLE		21	/* network file handle */						/* UN-USED */
#define	M_NFSREQ		22	/* NFS request header */						/* UN-USED */
#define	M_NFSMNT		23	/* NFS mount structure */						/* UN-USED */
#define	M_NFSNODE		24	/* NFS vnode private part */
#define	M_VNODE			25	/* Dynamically allocated vnodes */
#define	M_CACHE			26	/* Dynamically allocated cache entries */
#define	M_DQUOT			27	/* UFS quota entries */
#define	M_UFSMNT		28	/* UFS mount structure */
#define	M_SHM			29	/* SVID compatible shared memory segments */	/* UN-USED */
#define	M_VMMAP			30	/* VM map structures */
#define	M_VMMAPENT		31	/* VM map entry structures */
#define	M_VMOBJ			32	/* VM object structure */
#define	M_VMOBJHASH		33	/* VM object hash structure */
#define	M_VMPMAP		34	/* VM pmap */
#define	M_VMPVENT		35	/* VM phys-virt mapping entry */
#define	M_VMPAGER		36	/* VM pager struct */
#define	M_VMPGDATA		37	/* VM pager private data */
#define	M_FILE			38	/* Open file structure */
#define	M_FILEDESC		39	/* Open file descriptor table */
#define	M_LOCKF			40	/* Byte-range locking structures */
#define	M_PROC			41	/* Proc structures */
#define	M_SUBPROC		42	/* Proc sub-structures */
#define	M_SEGMENT		43	/* Segment for LFS */
#define	M_LFSNODE		44	/* LFS vnode private part */
#define	M_FFSNODE		45	/* FFS vnode private part */
#define	M_MFSNODE		46	/* MFS vnode private part */
#define	M_NQLEASE		47	/* Nqnfs lease */								/* UN-USED */
#define	M_NQMHOST		48	/* Nqnfs host address table */					/* UN-USED */
#define	M_NETADDR		49	/* Export host address structure */
#define	M_NFSSVC		50	/* Nfs server structure */						/* UN-USED */
#define	M_NFSUID		51	/* Nfs uid mapping structure */					/* UN-USED */
#define	M_NFSD			52	/* Nfs server daemon structure */				/* UN-USED */
#define	M_IPMOPTS		53	/* internet multicast options */
#define	M_IPMADDR		54	/* internet multicast address */
#define	M_IFMADDR		55	/* link-level multicast address */
#define	M_MRTABLE		56	/* multicast routing tables */
#define M_ISOFSMNT		57	/* ISOFS mount structure */
#define M_ISOFSNODE		58	/* ISOFS vnode private part */
#define M_NFSRVDESC		59	/* NFS server socket descriptor */				/* UN-USED */
#define M_NFSDIROFF		60	/* NFS directory offset data */					/* UN-USED */
#define M_NFSBIGFH		61	/* NFS version 3 file handle */					/* UN-USED */
#define	M_MSDOSFSMNT	62	/* MSDOS FS mount structure */
#define	M_MSDOSFSFAT	63	/* MSDOS FS fat table */
#define	M_MSDOSFSNODE	64	/* MSDOS FS vnode private part */
#define M_KENV			65	/* kern environment */
#define M_EXEC			66	/* argument lists & other mem used by exec */
#define M_COREMAP		67	/* 2.11BSD's Coremap */
#define M_SWAPMAP		68	/* 2.11BSD's Swapmap */
#define M_MEMDESC		69	/* memory range descriptors */
#define M_DEVSW			70	/* device switch table */
#define M_DEVSWHASH		71	/* device switch table hash structure */
#define M_USB			72	/* USB general */
#define	M_TTY			73	/* allocated tty structures */
#define M_KEVENT 		74	/* kevents/knotes */
#define M_TOPO			75	/* cpu topology structure */
#define M_BITLIST		76	/* bitlist structure */
#define M_PERCPU		77	/* percpu structure */
#define M_UFS211		78	/* UFS211 bufmap */
#define M_UFMLOPS		79	/* UFML uop structure */
#define M_GSCHED		80	/* global scheduler structures */
#define M_EVDEV			81	/* evdev structures */
#define M_IFMEDIA       82  /* ifmedia interface media state */
#define M_PACKET_TAGS	83	/* Packet-attached information */
#define	M_IPQ			84	/* IP packet queue entry */
#define	M_CRYPTO_DATA	85	/* crypto session records */
#define M_XDATA         86  /* xform data buffers */
#define	M_IP6OPT		87	/* IPv6 options */
#define M_IP6NDP		88	/* IPv6 Neighbour Discovery */
#define M_SPIDPQ        89  /* SP packet queue entry */
#define	M_PF			90	/* Network Packet Filter (PF) */
#define	M_TEMP			91	/* misc temporary data buffers */
#define	M_LAST 			M_TEMP+1 	/* Must be last type + 1 */

#define INITKMEMNAMES {						\
	"free",			/* 0 M_FREE */ 			\
	"mbuf",			/* 1 M_MBUF */ 			\
	"devbuf",		/* 2 M_DEVBUF */ 		\
	"socket",		/* 3 M_SOCKET */ 		\
	"pcb",			/* 4 M_PCB */ 			\
	"routetbl",		/* 5 M_RTABLE */ 		\
	"hosttbl",		/* 6 M_HTABLE */ 		\
	"fragtbl",		/* 7 M_FTABLE */ 		\
	"zombie",		/* 8 M_ZOMBIE */ 		\
	"ifaddr",		/* 9 M_IFADDR */ 		\
	"soopts",		/* 10 M_SOOPTS */ 		\
	"soname",		/* 11 M_SONAME */ 		\
	"namei",		/* 12 M_NAMEI */ 		\
	"gprof",		/* 13 M_GPROF */ 		\
	"ioctlops",		/* 14 M_IOCTLOPS */ 	\
	"mapmem",		/* 15 M_MAPMEM */ 		\
	"cred",			/* 16 M_CRED */ 		\
	"pgrp",			/* 17 M_PGRP */ 		\
	"session",		/* 18 M_SESSION */ 		\
	"iov",			/* 19 M_IOV */ 			\
	"mount",		/* 20 M_MOUNT */ 		\
	"fhandle",		/* 21 M_FHANDLE */ 		\
	"NFS req",		/* 22 M_NFSREQ */ 		\
	"NFS mount",	/* 23 M_NFSMNT */ 		\
	"NFS node",		/* 24 M_NFSNODE */ 		\
	"vnodes",		/* 25 M_VNODE */ 		\
	"namecache",	/* 26 M_CACHE */ 		\
	"UFS quota",	/* 27 M_DQUOT */ 		\
	"UFS mount",	/* 28 M_UFSMNT */ 		\
	"shm",			/* 29 M_SHM */ 			\
	"VM map",		/* 30 M_VMMAP */ 		\
	"VM mapent",	/* 31 M_VMMAPENT */ 	\
	"VM object",	/* 32 M_VMOBJ */ 		\
	"VM objhash",	/* 33 M_VMOBJHASH */	\
	"VM pmap",		/* 34 M_VMPMAP */ 		\
	"VM pvmap",		/* 35 M_VMPVENT */ 		\
	"VM pager",		/* 36 M_VMPAGER */ 		\
	"VM pgdata",	/* 37 M_VMPGDATA */ 	\
	"file",			/* 38 M_FILE */ 		\
	"file desc",	/* 39 M_FILEDESC */ 	\
	"lockf",		/* 40 M_LOCKF */ 		\
	"proc",			/* 41 M_PROC */ 		\
	"subproc",		/* 42 M_SUBPROC */ 		\
	"LFS segment",	/* 43 M_SEGMENT */ 		\
	"LFS node",		/* 44 M_LFSNODE */ 		\
	"FFS node",		/* 45 M_FFSNODE */ 		\
	"MFS node",		/* 46 M_MFSNODE */ 		\
	"NQNFS Lease",	/* 47 M_NQLEASE */ 		\
	"NQNFS Host",	/* 48 M_NQMHOST */ 		\
	"Export Host",	/* 49 M_NETADDR */ 		\
	"NFS srvsock",	/* 50 M_NFSSVC */ 		\
	"NFS uid",		/* 51 M_NFSUID */ 		\
	"NFS daemon",	/* 52 M_NFSD */ 		\
	"ip_moptions",	/* 53 M_IPMOPTS */ 		\
	"in_multi",		/* 54 M_IPMADDR */ 		\
	"ether_multi",	/* 55 M_IFMADDR */ 		\
	"mrt",			/* 56 M_MRTABLE */ 		\
	"ISOFS mount",	/* 57 M_ISOFSMNT */ 	\
	"ISOFS node",	/* 58 M_ISOFSNODE */ 	\
	"NFSV3 srvdesc",/* 59 M_NFSRVDESC */ 	\
	"NFSV3 diroff",	/* 60 M_NFSDIROFF */	\
	"NFSV3 bigfh",	/* 61 M_NFSBIGFH */ 	\
	"MSDOSFS mount"	/* 62 M_MSDOSFSMNT */	\
	"MSDOSFS fat",	/* 63 M_MSDOSFSFAT */ 	\
	"MSDOSFS node",	/* 64 M_MSDOSFSNODE */ 	\
	"kern envir" 	/* 65 M_KENV */			\
	"exec",			/* 66 M_EXEC */			\
	"coremap",		/* 67 M_COREMAP */		\
	"swapmap",		/* 68 M_SWAPMAP */		\
	"memdesc",		/* 69 M_MEMDESC */		\
	"devsw",		/* 70 M_DEVSW */		\
	"devswhash",	/* 71 M_DEVSWHASH */	\
	"usb",			/* 72 M_USB */			\
	"tty",			/* 73 M_TTY */			\
	"kevent",		/* 74 M_KEVENT */		\
	"cpu topo",		/* 75 M_TOPO */			\
	"bitmap",		/* 76 M_BITMAP */		\
	"percpu",		/* 77 M_PERCPU */		\
	"ufs211 buf",	/* 78 M_UFS211 */		\
	"ufml uops",	/* 79 M_UFMLOPS */		\
	"gscheduler",	/* 80 M_GSCHED */		\
	"evdev",		/* 81 M_EVDEV */		\
	"ifmedia",      /* 82 M_IFMEDIA */      \
	"packet tags",	/* 83 M_PACKET_TAGS */	\
	"IP queue ent", /* 84 M_IPQ */ 			\
	"crypto session records",  	/* 85 M_CRYPTO_DATA */     \
	"xform data buffers",  		/* 86 M_XDATA */           \
	"ip6_options",	/* 87 M_IP6OPT */		\
	"NDP",			/* 88 M_IP6NDP */		\
	"SP queue ent", /* 89 M_SPIDPQ */       \
	"PF",			/* 90 M_PF */ 			\
	"temp",			/* 91 M_TEMP */ 		\
}

#endif /* _SYS_MALLOCTYPES_H_ */
