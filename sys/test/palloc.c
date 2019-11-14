/*
 * palloc.c
 *
 *  Created on: 14 Nov 2019
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <pool2.h>
#include <pmalloc.h>

void*
smalloc(u_long size)
{
	void *v;

	for(;;) {
		v = vpoolalloc(mainmem, size + NPADLONG *sizeof(u_long));
		if(v != NULL)
			break;

		//tsleep(&up->sleep, return0, 0, 100);
	}
	if(NPADLONG){
		v = (u_long*)v + NPADLONG;
		setmalloctag(v, getcallerpc(&size));
	}
	memset(v, 0, size);
	return v;
}

void*
malloc(u_long size)
{
	void *v;

	v = vpoolalloc(mainmem, size+ NPADLONG * sizeof(u_long));
	if(v == NULL)
		return NULL;
	if(NPADLONG){
		v = (u_long*)v + NPADLONG;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	memset(v, 0, size);
	return v;
}

void*
mallocalign(u_long size, u_long align, long offset, u_long span)
{
	void *v;

	v = vpoolallocalign(mainmem, size+NPADLONG * sizeof(u_long), align, offset-NPADLONG*sizeof(u_long), span);
	if(NPADLONG && v != NULL){
		v = (u_long*)v + NPADLONG;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	if(v)
		memset(v, 0, size);
	return v;
}

void
free(void *v)
{
	if(v != NULL)
		poolfree(mainmem, (u_long*)v - NPADLONG);
}

void*
realloc(void *v, u_long size)
{
	void *nv;

	if(v != NULL)
		v = (u_long*)v-NPADLONG;
	if(NPADLONG !=0 && size != 0)
		size += NPADLONG*sizeof(u_long);

	if(nv == vpoolrealloc(mainmem, v, size)){
		nv = (u_long*)nv+NPADLONG;
		setrealloctag(nv, getcallerpc(&v));
		if(v == NULL)
			setmalloctag(nv, getcallerpc(&v));
	}
	return nv;
}

u_long
msize(void *v)
{
	return poolmsize(mainmem, (u_long*)v-NPADLONG)- NPADLONG * sizeof(u_long);
}

mallocz(u_long size, int clr)
{
	void *v;

	v = vpoolalloc(mainmem, size+NPADLONG*sizeof(u_long));
	if(NPADLONG && v != NULL){
		v = (u_long*)v+NPADLONG;
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
