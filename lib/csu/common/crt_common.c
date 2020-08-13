/*
 * crt_common.c
 *
 *  Created on: 13 Aug 2020
 *      Author: marti
 */

#include <sys/cdefs.h>
#include <sys/param.h>

#include <sys/exec_elf.h>
#include <stdlib.h>

#include "crt_common.h"

static void (*__CTOR_LIST__[])(void)
    __attribute__((section(".ctors"))) = { (void *)-1 };	/* XXX */
static void (*__DTOR_LIST__[])(void)
    __attribute__((section(".dtors"))) = { (void *)-1 };	/* XXX */

static void (*__CTOR_END__[])(void)
    __attribute__((section(".ctors"))) = { (void *)0 };		/* XXX */
static void (*__DTOR_END__[])(void)
    __attribute__((section(".dtors"))) = { (void *)0 };		/* XXX */

void
__ctors_begin(void)
{
	unsigned long i;
	void (**p)(void);

	for (i = 1;; i++) {
		p = __CTOR_LIST__[i];
		if(p == (void *)0 || p == (void *)-1) {
			break;
		}
		(**p)();
	}
}

void
__dtors_begin(void)
{
	unsigned long i;
	void (**p)(void);

	for (i = 1;; i++) {
		p = __DTOR_LIST__[i];
		if(p == (void *)0 || p == (void *)-1) {
			break;
		}
		(**p)();
	}
}

void
__ctors_end(void)
{
	unsigned long i;
	void (**p)(void);

	for (i = 1;; i++) {
		p = __CTOR_END__[-i];
		if(p == (void *)0 || p == (void *)-1) {
			break;
		}
		(**p)();
	}
}

void
__dtors_end(void)
{
	unsigned long i;
	void (**p)(void);

	for (i = 1;; i++) {
		p = __DTOR_END__[-i];
		if(p == (void *)0 || p == (void *)-1) {
			break;
		}
		(**p)();
	}
}
