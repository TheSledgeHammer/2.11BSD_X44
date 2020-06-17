/*	sgtty.h	4.2	85/01/03	*/

#ifndef	_IOCTL_
#include <sys/ioctl.h>
#include <sys/cdefs.h>
#endif

__BEGIN_DECLS
int gtty __P((int, struct sgttyb *));
int stty __P((int, struct sgttyb *));
__END_DECLS
