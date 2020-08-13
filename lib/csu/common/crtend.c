/*
 * crtend.c
 *
 *  Created on: 13 Aug 2020
 *      Author: marti
 */

#include <sys/cdefs.h>

#ifdef HAVE_CTORS
static void
__do_global_ctors_aux(void)
{
	/*
	 * Call global constructors.
	 */
	__ctors_end();
}

static void
__do_global_dtors_aux(void)
{
	/*
	 * Call global destructors.
	 */
	__dtors_end();
}


asm (
    ".pushsection .init		\n"
    "\t" INIT_CALL_SEQ(__do_global_ctors_aux) "\n"
    ".popsection		\n"
);
#endif
