/*
 *      Program Name:   syserrlst.c
 *      Date: March 6, 1996; July 22, 2024;
 *      Author: S.M. Schultz
 *
 *      -----------------   Modification History   ---------------
 *      Version Date            Reason For Modification
 *      1.0     06Mar96         1. Initial release into the public domain.
 *      1.1     22Jul2024       1. Generates a default error list, akin to the likes of 4.4BSD-Lite2
*/

/*
 * syserrlst - reads an error message from _PATH_SYSERRLST to a static
 *	       buffer.
 *
 * __errlst - same as above but the caller must specify an error list file.
 *
 * ____errlst_default - same as above but generates a default error list as
 * 			pre-defined by list.
 * syserrlst_default - same as above but sets "sys_errlist" from "errlst.c"
 * 			as the pre-defined list.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <errno.h>
#include <errlst.h>

static	char	msgstr[64];
char	*__errlst(int, char *);
char 	*syserrlst_default(int, char *);
char 	*__errlst_default(int, char *, char **);

char *
syserrlst(err)
	int	err;
{
	return (syserrlst_default(err, _PATH_SYSERRLST));
}

/*
 * Returns NULL if an error is encountered.  It is the responsiblity of the
 * caller (strerror(3) is most cases) to convert NULL into a "Error N".
*/

char *
__errlst(err, path)
	int	err;
	char	*path;
{
	register int	f;
	register char	*retval = NULL;
	int	saverrno;
	struct	iovec	iov[2];
	off_t	off;
	struct	ERRLST	emsg;
	struct	ERRLSTHDR ehdr;

	saverrno = errno;
	f = open(path, O_RDONLY);
	if (f < 0 || !path)
		goto oops;

	if (read(f, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
		goto oops;

	if (err < 0|| err > ehdr.maxmsgnum || ehdr.maxmsgnum <= 0 || ehdr.magic != ERRMAGIC)
		goto oops;

	off = sizeof(ehdr) + ((off_t) sizeof(struct ERRLST) * err);
	if (lseek(f, off, L_SET) == (off_t) -1)
		goto oops;

	if (read(f, &emsg, sizeof(emsg)) != sizeof(emsg))
		goto oops;

	if (emsg.lenmsg >= sizeof(msgstr))
		emsg.lenmsg = sizeof(msgstr) - 1;

	if (lseek(f, emsg.offmsg, L_SET) == (off_t) -1)
		goto oops;

	if (read(f, msgstr, emsg.lenmsg) != emsg.lenmsg)
		goto oops;

	retval = msgstr;
	retval[emsg.lenmsg] = '\0';
oops:
	if (f >= 0)
		close(f);
	errno = saverrno;
	return (retval);
}

char *
syserrlst_default(err, path)
	int 	err;
	char 	*path;
{
	extern char *sys_errlist[];

	return (__errlst_default(err, path, sys_errlist));
}

/*
 * Generate a default list from list.
 */
char *
__errlst_default(err, path, list)
	int err;
	char	*path;
	char **list;
{
	char *msg = NULL;

	msg = list[err];
	strncpy(msgstr, msg, 64);
	return (__errlst(err, path));
}
