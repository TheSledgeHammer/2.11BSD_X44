/*
 * BMOVE -- block move
 * This is a highly optimized version of the old C-language
 * bmove routine; it's function (should be) identical.
 * It uses a fancy algorithm to move words instead of bytes
 * whenever possible.
 *
 * Parameters:
 *		a [4(sp)] -- source area
 *		b [6(sp)] -- target area
 *		l [10(sp)] -- byte count
 *
 *	Returns:
 *		Pointer to end of target area.
 *
 *	History:
 *		3/14/79 [rse] -- added odd to odd case
 *		2/9/78 [bob] -- converted from "C"
 */

char *
bmove(a, b, l)
	char *a, *b;
	int	 l;
{
	register int	n;
	register char	*p, *q;
	p = a;
	q = b;
	n = l;
	while (n--)
		*q++ = *p++;
	return (q);
}
