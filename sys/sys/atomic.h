/*	$OpenBSD: atomic.h,v 1.6 2019/03/09 06:14:21 visa Exp $ */
/*
 * Copyright (c) 2014 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SYS_ATOMIC_H_
#define _SYS_ATOMIC_H_

#include <machine/atomic.h>

/*
 * an arch wanting to provide its own implementations does so by defining
 * macros.
 */

/*
 * atomic_cas_*
 */

#ifndef atomic_cas_uint
static inline unsigned int
atomic_cas_uint(volatile unsigned int *p, unsigned int o, unsigned int n)
{
	return __sync_val_compare_and_swap(p, o, n);
}
#endif

#ifndef atomic_cas_ulong
static inline unsigned long
atomic_cas_ulong(volatile unsigned long *p, unsigned long o, unsigned long n)
{
	return __sync_val_compare_and_swap(p, o, n);
}
#endif

#ifndef atomic_cas_ptr
static inline void *
atomic_cas_ptr(volatile void *pp, void *o, void *n)
{
	void * volatile *p = (void * volatile *)pp;
	return __sync_val_compare_and_swap(p, o, n);
}
#endif

/*
 * atomic_swap_*
 */

#ifndef atomic_swap_uint
static inline unsigned int
atomic_swap_uint(volatile unsigned int *p, unsigned int v)
{
	return __sync_lock_test_and_set(p, v);
}
#endif

#ifndef atomic_swap_ulong
static inline unsigned long
atomic_swap_ulong(volatile unsigned long *p, unsigned long v)
{
	return __sync_lock_test_and_set(p, v);
}
#endif

#ifndef atomic_swap_ptr
static inline void *
atomic_swap_ptr(volatile void *pp, void *v)
{
	void * volatile *p = (void * volatile *)pp;
	return __sync_lock_test_and_set(p, v);
}
#endif

/*
 * atomic_add_*_nv - add and fetch
 */

#ifndef atomic_add_int_nv
static inline unsigned int
atomic_add_int_nv(volatile unsigned int *p, unsigned int v)
{
	return __sync_add_and_fetch(p, v);
}
#endif

#ifndef atomic_add_long_nv
static inline unsigned long
atomic_add_long_nv(volatile unsigned long *p, unsigned long v)
{
	return __sync_add_and_fetch(p, v);
}
#endif

/*
 * atomic_add - add
 */

#ifndef atomic_add_int
#define atomic_add_int(_p, _v) ((void)atomic_add_int_nv((_p), (_v)))
#endif

#ifndef atomic_add_long
#define atomic_add_long(_p, _v) ((void)atomic_add_long_nv((_p), (_v)))
#endif

/*
 * atomic_inc_*_nv - increment and fetch
 */

#ifndef atomic_inc_int_nv
#define atomic_inc_int_nv(_p) atomic_add_int_nv((_p), 1)
#endif

#ifndef atomic_inc_long_nv
#define atomic_inc_long_nv(_p) atomic_add_long_nv((_p), 1)
#endif

/*
 * atomic_inc_* - increment
 */

#ifndef atomic_inc_int
#define atomic_inc_int(_p) ((void)atomic_inc_int_nv(_p))
#endif

#ifndef atomic_inc_long
#define atomic_inc_long(_p) ((void)atomic_inc_long_nv(_p))
#endif

/*
 * atomic_sub_*_nv - sub and fetch
 */

#ifndef atomic_sub_int_nv
static inline unsigned int
atomic_sub_int_nv(volatile unsigned int *p, unsigned int v)
{
	return __sync_sub_and_fetch(p, v);
}
#endif

#ifndef atomic_sub_long_nv
static inline unsigned long
atomic_sub_long_nv(volatile unsigned long *p, unsigned long v)
{
	return __sync_sub_and_fetch(p, v);
}
#endif

/*
 * atomic_sub_* - sub
 */

#ifndef atomic_sub_int
#define atomic_sub_int(_p, _v) ((void)atomic_sub_int_nv((_p), (_v)))
#endif

#ifndef atomic_sub_long
#define atomic_sub_long(_p, _v) ((void)atomic_sub_long_nv((_p), (_v)))
#endif

/*
 * atomic_dec_*_nv - decrement and fetch
 */

#ifndef atomic_dec_int_nv
#define atomic_dec_int_nv(_p) atomic_sub_int_nv((_p), 1)
#endif

#ifndef atomic_dec_long_nv
#define atomic_dec_long_nv(_p) atomic_sub_long_nv((_p), 1)
#endif

/*
 * atomic_dec_* - decrement
 */

#ifndef atomic_dec_int
#define atomic_dec_int(_p) ((void)atomic_dec_int_nv(_p))
#endif

#ifndef atomic_dec_long
#define atomic_dec_long(_p) ((void)atomic_dec_long_nv(_p))
#endif

/*
 * memory barriers
 */

#ifndef membar_enter
#define membar_enter() __sync_synchronize()
#endif

#ifndef membar_exit
#define membar_exit() __sync_synchronize()
#endif

#ifndef membar_producer
#define membar_producer() __sync_synchronize()
#endif

#ifndef membar_consumer
#define membar_consumer() __sync_synchronize()
#endif

#ifndef membar_sync
#define membar_sync() __sync_synchronize()
#endif

#ifndef membar_enter_after_atomic
#define membar_enter_after_atomic() membar_enter()
#endif

#ifndef membar_exit_before_atomic
#define membar_exit_before_atomic() membar_exit()
#endif

#ifdef _KERNEL

/*
 * Force any preceding reads to happen before any subsequent reads that
 * depend on the value returned by the preceding reads.
 */
static inline void
membar_datadep_consumer(void)
{
#ifdef __alpha__
	membar_consumer();
#endif
}

#define READ_ONCE(x) ({								\
	typeof(x) __tmp = *(volatile typeof(x) *)&(x);	\
	membar_datadep_consumer();						\
	__tmp;											\
})

#define WRITE_ONCE(x, val) ({						\
	typeof(x) __tmp = (val);						\
	*(volatile typeof(x) *)&(x) = __tmp;			\
	__tmp;											\
})

#ifdef _LP64
#define	__HAVE_ATOMIC64_LOADSTORE	1
#define	__ATOMIC_SIZE_MAX			8
#else
#define	__ATOMIC_SIZE_MAX			4
#endif

/*
 * We assume that access to an aligned pointer to a volatile object of
 * at most __ATOMIC_SIZE_MAX bytes is guaranteed to be atomic.  This is
 * an assumption that may be wrong, but we hope it won't be wrong
 * before we just adopt the C11 atomic API.
 */
#define	__ATOMIC_PTR_CHECK(p) do					      	\
{									      					\
	CTASSERT(sizeof(*(p)) <= __ATOMIC_SIZE_MAX);			\
	KASSERT(((uintptr_t)(p) & (sizeof(*(p)) - 1)) == 0);	\
} while (0)

#define __BEGIN_ATOMIC_LOAD(p, v) 							\
	__typeof__(*(p)) v = *(p)
#define __END_ATOMIC_LOAD(v) 								\
	v
#define __DO_ATOMIC_STORE(p, v) 							\
	*p = v

#define	atomic_load_relaxed(p)						      	\
({									      					\
	const volatile __typeof__(*(p)) *__al_ptr = (p);		\
	__ATOMIC_PTR_CHECK(__al_ptr);					      	\
	__BEGIN_ATOMIC_LOAD(__al_ptr, __al_val);			    \
	__END_ATOMIC_LOAD(__al_val);					      	\
})

#define	atomic_load_consume(p)						      	\
({									      					\
	const volatile __typeof__(*(p)) *__al_ptr = (p);		\
	__ATOMIC_PTR_CHECK(__al_ptr);					      	\
	__BEGIN_ATOMIC_LOAD(__al_ptr, __al_val);			    \
	membar_datadep_consumer();					      		\
	__END_ATOMIC_LOAD(__al_val);					      	\
})

/*
 * We want {loads}-before-{loads,stores}.  It is tempting to use
 * membar_enter, but that provides {stores}-before-{loads,stores},
 * which may not help.  So we must use membar_sync, which does the
 * slightly stronger {loads,stores}-before-{loads,stores}.
 */
#define	atomic_load_acquire(p)						      	\
({									      					\
	const volatile __typeof__(*(p)) *__al_ptr = (p);		\
	__ATOMIC_PTR_CHECK(__al_ptr);					      	\
	__BEGIN_ATOMIC_LOAD(__al_ptr, __al_val);			    \
	membar_sync();							      			\
	__END_ATOMIC_LOAD(__al_val);					      	\
})

#define	atomic_store_relaxed(p,v)					      	\
({									      					\
	volatile __typeof__(*(p)) *__as_ptr = (p);			    \
	__typeof__(*(p)) __as_val = (v);				      	\
	__ATOMIC_PTR_CHECK(__as_ptr);					      	\
	__DO_ATOMIC_STORE(__as_ptr, __as_val);				    \
})

#define	atomic_store_release(p,v)					    	\
({									     	 				\
	volatile __typeof__(*(p)) *__as_ptr = (p);			    \
	__typeof__(*(p)) __as_val = (v);				      	\
	__ATOMIC_PTR_CHECK(__as_ptr);					      	\
	membar_exit();							      			\
	__DO_ATOMIC_STORE(__as_ptr, __as_val);				    \
})

#endif /* _KERNEL */

#endif /* _SYS_ATOMIC_H_ */
