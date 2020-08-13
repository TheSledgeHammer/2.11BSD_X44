/*
 * crtbegin.c
 *
 *  Created on: 13 Aug 2020
 *      Author: marti
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/exec_elf.h>

#include <stdlib.h>
#include "crt_common.h"

#ifndef SHARED
void *__dso_handle = 0;
#else
void *__dso_handle = &__dso_handle;
void __cxa_finalize(void *) __weak_symbol;

/*
 * Call __cxa_finalize with the dso handle in shared objects.
 * When we have ctors/dtors call from the dtor handler before calling
 * any dtors, otherwise use a destructor.
 */
#ifndef HAVE_CTORS
__attribute__((destructor))
#endif
static void
run_cxa_finalize(void)
{

	if (__cxa_finalize != NULL)
		__cxa_finalize(__dso_handle);
}
#endif

/*
 * On some architectures and toolchains we may need to call the .dtors.
 * These are called in the order they are in the ELF file.
 */
#ifdef HAVE_CTORS

static void
__do_global_ctors_aux(void)
{
	static int initialized;

	if (!initialized) {
		initialized = 1;
	}

	/*
	 * Call global constructors.
	 */
	__ctors_begin();
}

static void
__do_global_dtors_aux(void)
{
	static int finished;

	if (finished)
		return;

#ifdef SHARED
	run_cxa_finalize();
#endif

	/*
	 * Call global destructors.
	 */
	__dtors_begin();
}

asm (
    ".pushsection .fini		\n"
    "\t" INIT_CALL_SEQ(__do_global_dtors_aux) "\n"
    ".popsection		\n"
);
#endif


/*
 * Handler for gcj. These provide a _Jv_RegisterClasses function and fill
 * out the .jcr section. We just need to call this function with a pointer
 * to the appropriate section.
 */
extern void _Jv_RegisterClasses(void *);
static void register_classes(void);

static void *__JCR_LIST__[]
    __attribute__((section(".jcr"))) = { };

#ifndef CTORS_CONSTRUCTORS
__attribute__((constructor))
#endif
static void
register_classes(void)
{

	if (_Jv_RegisterClasses != NULL && __JCR_LIST__[0] != 0)
		_Jv_RegisterClasses(__JCR_LIST__);
}

/*
 * We can't use constructors when they use the .ctors section as they may be
 * placed before __CTOR_LIST__.
 */
#ifdef CTORS_CONSTRUCTORS
asm (
    ".pushsection .init		\n"
    "\t" INIT_CALL_SEQ(register_classes) "\n"
    ".popsection		\n"
);
#endif
