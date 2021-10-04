
/*
 * test trace flag routine.
 * Call:
 *		if (tTf(m, n)) ...
 *		Tests bit n of trace flag m (or any bit of m if n < 0)
 * History:
 *		12/13/78 (eric) -- written from C
 */

int
tTf(m, n)
	int m, n;
{
	extern char	tTany;
	extern int	tT[];
	if (n < 0) {
		return (tT[m]);
	} else {
		return ((tT[m] >> n) & 01);
	}
}
