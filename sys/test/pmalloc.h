/*
 * malloc.h
 *
 *  Created on: 14 Nov 2019
 *      Author: marti
 */

#ifndef SYS_MALLOC_H_
#define SYS_MALLOC_H_

static void*	sbrkalloc(u_long);
static int		sbrkmerge(void*, void*);

/* tracing */
#define NPADLONG		2
#define MALLOC_OFFSET 	0
#define REALLOC_OFFSET 	1

#endif /* SYS_TEST_MALLOC_H_ */
