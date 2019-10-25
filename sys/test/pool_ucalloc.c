#include <stdint.h> /* uint8_t uintptr_t */
#include <pool.h>

static Pool ucpool = {
		.name		= "Uncached",
		.maxsize	= 4*MiB,
		.minarena	= 1*MiB-32,
		.quantum	= 32,
		.alloc		= ucarena,
		.merge		= NULL,
		.flags		= /*POOL_TOLERANCE|POOL_ANTAGONISM|POOL_PARANOIA|*/0,
};


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
	poolfree(&ucpool, v);
}

void*
ucallocalign(unsigned long size, int align, int span)
{
	void *v;

	assert(size < ucpool.minarena-128);
	v = poolallocalign(&ucpool, size, align, 0, span);
	if(v)
		memset(v, 0, size);
	return v;
}

void*
ucalloc(unsigned long size)
{
	return ucallocalign(size, 32, 0);
}
