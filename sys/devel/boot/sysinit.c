/*
 * sysinit.c
 *
 *  Created on: 29 Mar 2020
 *      Author: marti
 */

#include <sysinit.h>

#ifndef BOOTHOWTO
#define	BOOTHOWTO	0
#endif
int	boothowto = BOOTHOWTO;	/* initialized so that it can be patched */

#ifndef BOOTVERBOSE
#define	BOOTVERBOSE	0
#endif
int	bootverbose = BOOTVERBOSE;

struct sysinit **sysinit, **sysinit_end;
struct sysinit **newsysinit, **newsysinit_end;

void
mi_startup(void)
{
	struct sysinit **sipp;	/* system initialization*/
	struct sysinit **xipp;	/* interior loop of sort*/
	struct sysinit *save;	/* bubble*/

	if (boothowto & RB_VERBOSE)
		bootverbose++;
	if (sysinit == NULL) {
		sysinit = SET_BEGIN(sysinit_set);
		sysinit_end = SET_LIMIT(sysinit_set);
	}
restart:
	/*
	 * Perform a bubble sort of the system initialization objects by
	 * their subsystem (primary key) and order (secondary key).
	 */
	for (sipp = sysinit; sipp < sysinit_end; sipp++) {
		for (xipp = sipp + 1; xipp < sysinit_end; xipp++) {
			if ((*sipp)->subsystem < (*xipp)->subsystem
					|| ((*sipp)->subsystem == (*xipp)->subsystem
							&& (*sipp)->order <= (*xipp)->order))
				continue; /* skip*/
			save = *sipp;
			*sipp = *xipp;
			*xipp = save;
		}
	}
	/*
	 * Traverse the (now) ordered list of system initialization tasks.
	 * Perform each task, and continue on to the next task.
	 */
	for (sipp = sysinit; sipp < sysinit_end; sipp++) {

		if ((*sipp)->subsystem == SI_SUB_DUMMY)
			continue; /* skip dummy task(s)*/

		if ((*sipp)->subsystem == SI_SUB_DONE)
			continue;
		/* Call function */
		(*((*sipp)->func))((*sipp)->udata);
		/* Check off the one we're just done */
		(*sipp)->subsystem = SI_SUB_DONE;

		/* Check if we've installed more sysinit items via KLD */
		if (newsysinit != NULL) {
			if (sysinit != SET_BEGIN(sysinit_set))
				free(sysinit, M_TEMP);
			sysinit = newsysinit;
			sysinit_end = newsysinit_end;
			newsysinit = NULL;
			newsysinit_end = NULL;
			goto restart;
		}
	}
	/* NOTREACHED*/
}
