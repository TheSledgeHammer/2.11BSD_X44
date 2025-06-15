
/*
 *      Program Name:   nsym.c
 *      Author:  S.M. Schultz
 *
 *      -----------   Modification History   ------------
 *      Version Date            Reason For Modification
 *      1.0     31Oct93         1. Initial release into the public domain.
 *				   Calculating the offsets of the string
 *				   and symbol tables in an executable is
 *				   rather messy and verbose when dealing
 *				   with overlaid objects.  The macros (in
 *				   a.out.h) N_STROFF, N_SYMOFF, etc simply
 *				   call these routines.
 *      --------------------------------------------------
*/

#include <a.out.h>

off_t n_stroff(struct xexec *);
off_t n_datoff(struct xexec *);
off_t n_dreloc(struct xexec *);
off_t n_treloc(struct xexec *);
off_t n_symoff(struct xexec *);

#ifdef OVERLAY
#define N_SYMOFF(ep)		(n_symoff(ep))
#define N_DRELOC(ep)		(n_dreloc(ep))
#define N_TRELOC(ep)		(n_treloc(ep))
#define	N_DATOFF(ep) 		(n_datoff(ep))
#define	N_STROFF(ep) 		(n_stroff(ep))
#endif

off_t n_ovlsum(u_int *);

off_t
n_stroff(ep)
	register struct xexec *ep;
{
	off_t l;

	l = n_symoff(ep);
	l += ep->e.a_syms;
	return (l);
}

off_t
n_datoff(ep)
	register struct xexec *ep;
{
	off_t l;

	l = n_treloc(ep);
	l -= ep->e.a_data;
	return (l);
}

/*
 * Obviously if bit 0 of the flags word (a_flag) is not off then there's
 * no relocation information present and this routine shouldn't have been
 * called.
 */

off_t
n_dreloc(ep)
	register struct xexec *ep;
{
	register int i;
	off_t l;

	l = (sizeof(struct exec) + ep->e.a_text + ep->e.a_data);
#ifdef OVERLAY
	if (N_GETMAGIC(ep->e) == A_MAGIC5 || N_GETMAGIC(ep->e) == A_MAGIC6) {
		l += n_ovlsum(ep->o.ov_siz);
		l += sizeof(struct ovlhdr);
	}
#endif
	l += ep->e.a_text;
	return (l);
}

off_t
n_treloc(ep)
	register struct xexec *ep;
{
	off_t l;

	l = n_dreloc(ep);
	l -= ep->e.a_text;
	return (l);
}

off_t
n_symoff(ep)
	register struct xexec *ep;
{
	register int i;
	off_t sum, l;

	l = N_TXTOFF(ep->e);
	sum = (ep->e.a_text + ep->e.a_data);
#ifdef OVERLAY
	if (N_GETMAGIC(ep->e) == A_MAGIC5 || N_GETMAGIC(ep->e) == A_MAGIC6) {
		sum += n_ovlsum(ep->o.ov_siz);
	}
#endif
	l += N_ALIGN(ep->e, sum);
	if (l == N_RELOFF(ep->e)) {
		l = N_SYMOFF(ep->e);
	} else {
		l += ep->e.a_trsize + ep->e.a_drsize;
	}
	return (l);
}

off_t
n_ovlsum(ovsize)
	u_int *ovsize;
{
	register int i;
	register u_int *ov;
	off_t sum;

	for (ov = ovsize, i = 0; i < NOVL; i++) {
		sum += *ov++;
	}
	return (sum);
}
