#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)isatty.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

/*
 * Returns 1 iff file is a tty
 */

#include <termios.h>
#include <unistd.h>
#include <sgtty.h>

int
isatty(f)
{
	struct sgttyb ttyb;

	if (ioctl(f, TIOCGETP, &ttyb) < 0)
		return(0);
	return(1);
}
/*
int
isatty(fd)
	int fd;
{
	struct termios t;

	return(tcgetattr(fd, &t) != -1);
}
*/
