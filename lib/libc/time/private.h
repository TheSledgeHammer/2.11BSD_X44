/*
 * $FreeBSD: head/contrib/tzcode/stdtime/private.h 192625 2009-05-23 06:31:50Z edwin $
 */

#ifndef PRIVATE_H
#define PRIVATE_H

/*
** Nested includes
*/

#include <limits.h>	/* for CHAR_BIT et al. */
#include <time.h>

/*
** Finally, some convenience items.
*/

#ifndef TRUE
#define TRUE	1
#endif /* !defined TRUE */

#ifndef FALSE
#define FALSE	0
#endif /* !defined FALSE */

#ifndef TYPE_BIT
#define TYPE_BIT(type)	(sizeof (type) * CHAR_BIT)
#endif /* !defined TYPE_BIT */

#ifndef TYPE_SIGNED
#define TYPE_SIGNED(type) (((type) -1) < 0)
#endif /* !defined TYPE_SIGNED */

/* The minimum and maximum finite time values.  */
static time_t const time_t_min =
  (TYPE_SIGNED(time_t)
   ? (time_t) -1 << (CHAR_BIT * sizeof (time_t) - 1)
   : 0);
static time_t const time_t_max =
  (TYPE_SIGNED(time_t)
   ? - (~ 0 < 0) - ((time_t) -1 << (CHAR_BIT * sizeof (time_t) - 1))
   : -1);

/*
** Since the definition of TYPE_INTEGRAL contains floating point numbers,
** it cannot be used in preprocessor directives.
*/

#ifndef TYPE_INTEGRAL
#define TYPE_INTEGRAL(type) (((type) 0.5) != 0.5)
#endif /* !defined TYPE_INTEGRAL */

#ifndef INT_STRLEN_MAXIMUM
/*
** 302 / 1000 is log10(2.0) rounded up.
** Subtract one for the sign bit if the type is signed;
** add one for integer division truncation;
** add one more for a minus sign if the type is signed.
*/
#define INT_STRLEN_MAXIMUM(type) \
	((TYPE_BIT(type) - TYPE_SIGNED(type)) * 302 / 1000 + \
	1 + TYPE_SIGNED(type))
#endif /* !defined INT_STRLEN_MAXIMUM */

/*
** UNIX was a registered trademark of The Open Group in 2003.
*/
#endif /* !defined PRIVATE_H */
