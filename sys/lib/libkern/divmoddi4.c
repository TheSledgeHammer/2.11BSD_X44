/*****************************************************************************\
 *  Copyright (C) 2007-2010 Lawrence Livermore National Security, LLC.
 *  Copyright (C) 2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Brian Behlendorf <behlendorf1@llnl.gov>.
 *  UCRL-CODE-235197
 *
 *  This file is part of the SPL, Solaris Porting Layer.
 *  For details, see <http://zfsonlinux.org/>.
 *
 *  The SPL is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  The SPL is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the SPL.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************
 *  Solaris Porting Layer (SPL) Generic Implementation.
\*****************************************************************************/

#include <sys/cdefs.h>

#include "quad.h"

/*
 * Implementation of 64-bit signed division/modulo for 32-bit machines.
 */
quad_t
__divmoddi4(quad_t n, quad_t d, quad_t *r)
{
	quad_t q, rr;
	boolean_t nn = FALSE;
	boolean_t nd = FALSE;
	if (n < 0) {
		nn = TRUE;
		n = -n;
	}
	if (d < 0) {
		nd = TRUE;
		d = -d;
	}

	q = __udivmoddi4(n, d, (u_quad_t *)&rr);

	if (nn != nd) {
		q = -q;
	}
	if (nn) {
		rr = -rr;
	}
	if (r) {
		*r = rr;
	}
	return (q);
}
