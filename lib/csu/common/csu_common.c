/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>

typedef void (*fptr_t)(void);
typedef void (*func_t)(void *);

#ifdef CRTBEGIN
#ifdef SHARED
static void common_finalize(func_t, void *);
#endif
#endif /* CRTBEGIN */

#ifdef CRTEND
#if defined(JCR) && defined(__GNUC__)
static void common_jcr(fptr_t *, func_t);
#endif
#endif /* CRTEND */

#ifdef CRTBEGIN

#ifdef SHARED
/*
 * Call __cxa_finalize with the dso handle in shared objects.
 * When we have ctors/dtors call from the dtor handler before calling
 * any dtors, otherwise use a destructor.
 */
#ifndef HAVE_CTORS
__attribute__((destructor))
#endif
static void
common_finalize(func_t final, void *handle)
{
    	if (final != NULL) {
    		final(handle);
    	}
}
#endif

#ifdef HAVE_CTORS
static inline void
common_dtors(fptr_t *list, fptr_t *end)
{
    	fptr_t *p;
    	for (p = list + 1; p < end; ) {
    		(*(*--p))();
    	}
}

static inline void
common_fini(func_t final, void *handle, fptr_t *list, fptr_t *end)
{
    	static int finished;

    	if (finished) {
	  	return;
	}

	finished = 1;

#ifdef SHARED
    	common_finalize(final, handle);
#endif

	/* Call global destructors.	*/
	common_dtors(list, end);
}
#endif
#endif /* CRTBEGIN */

#ifdef CRTEND

#if defined(JCR) && defined(__GNUC__)
static void
common_jcr(fptr_t *list, func_t regclass)
{
    	if (list[0] != NULL && regclass != NULL) {
		regclass(list);
	}
}
#endif

#ifdef HAVE_CTORS
static inline void
common_ctors(fptr_t *list, fptr_t *end)
{
    	fptr_t *p;
    	for (p = end; p > list + 1; ) {
    		(*(*--p))();
    	}
}

static inline void
common_init(fptr_t *jcr, func_t regclass, fptr_t *list, fptr_t *end)
{
    	static int initialized;

	if (initialized) {
	    return;
    	}

    	initialized = 1;

#if defined(JCR) && defined(__GNUC__)
        common_jcr(jcr, regclass);
#endif

	/* Call global constructors. */
    	common_ctors(list, end);
}
#endif
#endif /* CRTEND */
