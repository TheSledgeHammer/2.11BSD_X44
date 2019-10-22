#include <stdint.h> /* uint8_t uintptr_t */
#include <pool.h>

static Pool pmainmem = {
	.name=		"Main",
	.maxsize=	4*1024*1024,
	.minarena=	128*1024,
	.quantum=	32,
	.alloc=		xalloc,
	.merge=		xmerge,
	.flags=		POOL_TOLERANCE,
};

static Pool pvirtmen = {
		.name=		"Virt",
		.maxsize=	4*1024*1024,
		.minarena=	128*1024,
		.quantum=	32,
		.alloc=		xalloc,
		.merge=		xmerge,
		.flags=		POOL_TOLERANCE,
};

Pool*	mainmem = &pmainmem;
Pool*	virtmem = &pvirtmen;

/* tracing */
enum {
	Npadlong		= 2, /* Npadlong is the number of 32-bit longs to leave at the beginning of each allocated buffer for our own bookkeeping. */
	MallocOffset 	= 0, /* The malloc tag is stored at MallocOffset from the beginning of the block, and the realloc tag at ReallocOffset */
	ReallocOffset 	= 1
};

void*
smalloc(u_long size)
{
	void *v;

	for(;;) {
		v = poolalloc(mainmem, size+Npadlong*sizeof(u_long));
		if(v != NULL)
			break;

		//tsleep(&up->sleep, return0, 0, 100);
	}
	if(Npadlong){
		v = (u_long*)v+Npadlong;
		setmalloctag(v, getcallerpc(&size));
	}
	memset(v, 0, size);
	return v;
}

void*
malloc(u_long size)
{
	void *v;

	v = poolalloc(mainmem, size+Npadlong*sizeof(u_long));
	if(v == NULL)
		return NULL;
	if(Npadlong){
		v = (u_long*)v+Npadlong;
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

	v = poolallocalign(mainmem, size+Npadlong*sizeof(u_long), align, offset-Npadlong*sizeof(u_long), span);
	if(Npadlong && v != NULL){
		v = (u_long*)v+Npadlong;
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
		poolfree(mainmem, (u_long*)v-Npadlong);
}

void*
realloc(void *v, u_long size)
{
	void *nv;

	if(v != NULL)
		v = (u_long*)v-Npadlong;
	if(Npadlong !=0 && size != 0)
		size += Npadlong*sizeof(u_long);

	if(nv == poolrealloc(mainmem, v, size)){
		nv = (u_long*)nv+Npadlong;
		setrealloctag(nv, getcallerpc(&v));
		if(v == NULL)
			setmalloctag(nv, getcallerpc(&v));
	}
	return nv;
}

u_long
msize(void *v)
{
	return poolmsize(mainmem, (u_long*)v-Npadlong)-Npadlong*sizeof(u_long);
}

mallocz(u_long size, int clr)
{
	void *v;

	v = poolalloc(mainmem, size+Npadlong*sizeof(u_long));
	if(Npadlong && v != NULL){
		v = (u_long*)v+Npadlong;
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
	//USED(v, pc);
	if(Npadlong <= MallocOffset || v == NULL)
		return;
	u = v;
	u[-Npadlong+MallocOffset] = pc;
}

void
setrealloctag(void *v, u_long pc)
{
	u_long *u;
	//USED(v, pc);
	if(Npadlong <= ReallocOffset || v == NULL)
		return;
	u = v;
	u[-Npadlong+ReallocOffset] = pc;
}

u_long
getmalloctag(void *v)
{
	//USED(v);
	if(Npadlong <= MallocOffset)
		return ~0;
	return ((u_long*)v)[-Npadlong+MallocOffset];
}

u_long
getrealloctag(void *v)
{
	//USED(v);
	if(Npadlong <= ReallocOffset)
		return ((u_long*)v)[-Npadlong+ReallocOffset];
	return ~0;
}

u_long
getcallerpc(void *a)
{
	return 0;
}
