/*
 *      Program Name:   errlst.h
 *      Date: March 6, 1996
 *      Author: S.M. Schultz
 *
 *      -----------------   Modification History   ---------------
 *      Version Date            Reason For Modification
 *      1.0     05Mar96         1. Initial release into the public domain.
*/

#ifndef _ERRLST_H_
#define	_ERRLST_H_

#include <sys/cdefs.h>
#include <sys/ansi.h>

#ifndef	off_t
typedef	__off_t		off_t;
#define	off_t		__off_t
#endif /* off_t */

/*
 * Definitions used by the 'mkerrlst' program which creates error message 
 * files.
 *
 * The format of the file created is:
 *
 *	struct	ERRLSTHDR	ehdr;
 *	struct	ERRLST  emsg[num_of_msgs];
 *	struct	{
 *		char	msg[] = "error message string";
 *		char	lf = '\n';
 *		} [num_of_messages];
 *
 * Note:  the newlines are NOT included in the message lengths, the newlines
 *        are present to make it easy to 'cat' or 'vi' the file.
*/

struct	ERRLSTHDR {
	short	magic;
	short	maxmsgnum;
	short	maxmsglen;
	short	pad[5];		/* Reserved */
};

struct	ERRLST {
	off_t	offmsg;
	short	lenmsg;
};

#define	ERRMAGIC		012345
#define	_PATH_SYSERRLST		"/etc/syserrlst"

__BEGIN_DECLS
char *syserrlst(int);
__END_DECLS
#endif /* !_ERRLST_H_ */
