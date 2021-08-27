/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vmmac.h	7.6 (Berkeley) 7/2/90
 */

#ifndef _VMMAC_H_
#define _VMMAC_H_

/*
 * Virtual memory related conversion macros
 */
/* Core clicks to number of pages of page tables needed to map that much */
#define	ctopt(x)		(((x)+NPTEPG-1)/NPTEPG)

/* Virtual page numbers to text|data|stack segment page numbers and back */
#define	vtotp(p, v)		((int)(v)-LOWPAGES)
#define	vtodp(p, v)		((int)((v) - stoc(ctos((p)->p_tsize)) - LOWPAGES))
#define	vtosp(p, v)		((int)(BTOPUSRSTACK - 1 - (v)))
#define	tptov(p, i)		((unsigned)(i) + LOWPAGES)
#define	dptov(p, i)		((unsigned)(stoc(ctos((p)->p_tsize)) + (i) + LOWPAGES))
#define	sptov(p, i)		((unsigned)(BTOPUSRSTACK - 1 - (i)))

/* Tell whether virtual page numbers are in text|data|stack segment */
#define	isassv(p, v)	((v) >= BTOPUSRSTACK - (p)->p_ssize)
#define	isatsv(p, v)	(((v) - LOWPAGES) < (p)->p_tsize)
#define	isadsv(p, v)	(((v) - LOWPAGES) >= stoc(ctos((p)->p_tsize)) && (v) < LOWPAGES + (p)->p_tsize + (p)->p_dsize + (p)->p_mmsize)

#endif /* _VMMAC_H_ */
