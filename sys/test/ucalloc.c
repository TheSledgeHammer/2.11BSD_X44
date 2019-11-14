/*
 * ucalloc.c
 *
 *  Created on: 14 Nov 2019
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <pool2.h>

#define KiB		1024u			/* Kibi 0x0000000000000400 */
#define MiB		1048576u		/* Mebi 0x0000000000100000 */
#define GiB		1073741824u		/* Gibi 000000000040000000 */

static void*
ucarena(unsigned long size)
{
	void *uv, *v;

	assert(size == 1*MiB);

	mainmem->maxsize += 1*MiB;
	if((v = mallocalign(1*MiB, 1*MiB, 0, 0)) == NULL ||
	    (uv = mmuuncache(v, 1*MiB)) == NULL){
		free(v);
		mainmem->maxsize -= 1*MiB;
		return NULL;
	}
	return uv;
}

void
ucfree(void* v)
{
	if(v == NULL)
		return;
	vpoolfree(&ucpool, v);
}

void*
ucallocalign(unsigned long size, int align, int span)
{
	void *v;

	assert(size < ucpool.minarena-128);
	v = vpoolallocalign(&ucpool, size, align, 0, span);
	if(v)
		memset(v, 0, size);
	return v;
}

void*
ucalloc(unsigned long size)
{
	return ucallocalign(size, 32, 0);
}
