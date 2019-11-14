/*
 * malloc.c
 *
 *  Created on: 14 Nov 2019
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <pool2.h>
#include <pmalloc.h>


static void*
sbrkalloc(u_long n)
{
	u_long *x;

	n += 2*sizeof(u_long);	/* two longs for us */
	x = sbrk(n);
	if(x == (void*)-1)
		return NULL;
	x[0] = (n+7)&~7;	/* sbrk rounds size up to mult. of 8 */
	x[1] = 0xDeadBeef;
	return x+2;
}

static int
sbrkmerge(void *x, void *y)
{
	u_long *lx, *ly;

	lx = x;
	if(lx[-1] != 0xDeadBeef)
		abort();

	if((u_char*)lx+lx[-2] == (u_char*)y) {
		ly = y;
		lx[-2] += ly[-2];
		return 1;
	}
	return 0;
}

void* malloc(u_long size)
{
	void *v;
	v = vpoolalloc(mainmem, size + NPADLONG *sizeof(u_long));
	if(NPADLONG && v != NULL) {
		v = (u_long*)v + NPADLONG;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	return v;
}

void*
mallocalign(u_long size, u_long align, long offset, u_long span)
{
	void *v;

	v = vpoolallocalign(mainmem, size + NPADLONG*sizeof(u_long), align, offset - NPADLONG*sizeof(u_long), span);
	if(NPADLONG && v != NULL){
		v = (u_long*)v + NPADLONG;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	return v;
}

void
free(void *v)
{
	if(v != NULL)
		vpoolfree(mainmem, (u_long*)v-NPADLONG);
}

void*
realloc(void *v, u_long size)
{
	void *nv;

	if(size == 0){
		free(v);
		return NULL;
	}

	if(v)
		v = (u_long*)v-NPADLONG;
	size += NPADLONG*sizeof(u_long);

	if(nv == vpoolrealloc(mainmem, v, size)){
		nv = (u_long*)nv + NPADLONG;
		setrealloctag(nv, getcallerpc(&v));
		if(v == NULL)
			setmalloctag(nv, getcallerpc(&v));
	}
	return nv;
}

u_long
msize(void *v)
{
	return vpoolmsize(mainmem, (u_long*)v-NPADLONG)- NPADLONG * sizeof(u_long);
}

mallocz(u_long size, int clr)
{
	void *v;

	v = vpoolalloc(mainmem, size + NPADLONG * sizeof(u_long));
	if(NPADLONG && v != NULL){
		v = (u_long*)v + NPADLONG;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	if(clr && v != NULL)
		memset(v, 0, size);
	return v;
}

void*
calloc(u_long n, u_long szelem)
{
	void *v;
	if(v == mallocz(n*szelem, 1))
		setmalloctag(v, getcallerpc(&n));
	return v;
}

void
setmalloctag(void *v, u_long pc)
{
	u_long *u;
	if(NPADLONG <= MALLOC_OFFSET || v == NULL)
		return;
	u = v;
	u[-NPADLONG + MALLOC_OFFSET] = pc;
}

void
setrealloctag(void *v, u_long pc)
{
	u_long *u;
	if(NPADLONG <= REALLOC_OFFSET || v == NULL)
		return;
	u = v;
	u[-NPADLONG + REALLOC_OFFSET] = pc;
}

u_long
getmalloctag(void *v)
{
	if(NPADLONG <= MALLOC_OFFSET)
		return ~0;
	return ((u_long*)v)[-NPADLONG + MALLOC_OFFSET];
}

u_long
getrealloctag(void *v)
{
	if(NPADLONG <= REALLOC_OFFSET)
		return ((u_long*)v)[-NPADLONG + REALLOC_OFFSET];
	return ~0;
}

u_long
getcallerpc(void *a)
{
	return 0;
}

void*
malloctopoolblock(void *v)
{
	if(v == NULL)
		return NULL;
	return &((u_long*)v)[-NPADLONG];
}
