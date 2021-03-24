/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2003 John Baldwin <jhb@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/*
 * Machine dependent interrupt code for x86.  For x86, we have to
 * deal with different PICs.  Thus, we use the passed in vector to lookup
 * an interrupt source associated with that vector.  The interrupt source
 * describes which PIC the source belongs to and includes methods to handle
 * that source.
 */

#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/user.h>
#include <pic.h>

#define	MAX_STRAY_LOG	5

typedef void (*mask_fn)(uintptr_t vector);

static int intrcnt_index;
static struct intsrc **interrupt_sources;

static struct lock_object intrpic_lock;
static struct lock_object intrsrc_lock;
static struct lock_object intrcnt_lock;
static TAILQ_HEAD(pics_head, pic) pics;
u_int num_io_irqs;

u_long *intrcnt;
char *intrnames;
size_t sintrcnt = sizeof(intrcnt);
size_t sintrnames = sizeof(intrnames);
int nintrcnt;

static int	intr_assign_cpu(void *arg, int cpu);
static void	intr_disable_src(void *arg);
static void	intr_init(void *__dummy);
static int	intr_pic_registered(struct pic *pic);
static void	intrcnt_setname(const char *name, int index);
static void	intrcnt_updatename(struct intsrc *is);
static void	intrcnt_register(struct intsrc *is);

static int
intr_pic_registered(struct pic *pic)
{
	struct pic *p;

	TAILQ_FOREACH(p, &pics, pic_entry) {
		if (p == pic)
			return (1);
	}
	return (0);
}

int
intr_register_pic(struct pic *pic)
{
	int error;

	simple_lock(&intrpic_lock);
	if (intr_pic_registered(pic))
		error = EBUSY;
	else {
		TAILQ_INSERT_TAIL(&pics, pic, pic_entry);
		error = 0;
	}
	simple_unlock(&intrpic_lock);
	return (error);
}

/*
 * Allocate interrupt source arrays and register interrupt sources
 * once the number of interrupts is known.
 */
static void
intr_init_sources(void *arg)
{
	struct pic *pic;

	MPASS(num_io_irqs > 0);

	interrupt_sources = malloc(num_io_irqs, sizeof(*interrupt_sources), M_INTR, M_WAITOK | M_ZERO);
#ifdef SMP
	interrupt_sorted = malloc(num_io_irqs, sizeof(*interrupt_sorted), M_INTR, M_WAITOK | M_ZERO);
#endif

	/*
	 * - 1 ??? dummy counter.
	 * - 2 counters for each I/O interrupt.
	 * - 1 counter for each CPU for lapic timer.
	 * - 1 counter for each CPU for the Hyper-V vmbus driver.
	 * - 8 counters for each CPU for IPI counters for SMP.
	 */
	nintrcnt = 1 + num_io_irqs * 2 + mp_ncpus * 2;
#ifdef COUNT_IPIS
	if (mp_ncpus > 1)
		nintrcnt += 8 * mp_ncpus;
#endif
	intrcnt = malloc(nintrcnt, sizeof(u_long), M_INTR, M_WAITOK | M_ZERO);
	intrnames = malloc(nintrcnt, MAXCOMLEN + 1, M_INTR, M_WAITOK | M_ZERO);
	sintrcnt = nintrcnt * sizeof(u_long);
	sintrnames = nintrcnt * (MAXCOMLEN + 1);

	intrcnt_setname("???", 0);
	intrcnt_index = 1;

	/*
	 * NB: intrpic_lock is not held here to avoid LORs due to
	 * malloc() in intr_register_source().  However, we are still
	 * single-threaded at this point in startup so the list of
	 * PICs shouldn't change.
	 */
	TAILQ_FOREACH(pic, &pics, pic_entry) {
		if (pic->pic_register_sources != NULL)
			pic->pic_register_sources(pic);
	}
}

/*
 * Register a new interrupt source with the global interrupt system.
 * The global interrupts need to be disabled when this function is
 * called.
 */
int
intr_register_source(struct intsrc *isrc)
{
	int error, vector;

	KASSERT(intr_pic_registered(isrc->is_pic), ("unregistered PIC"));
	vector = isrc->is_pic->pic_vector(isrc);

	KASSERT(vector < num_io_irqs, ("IRQ %d too large (%u irqs)", vector, num_io_irqs));
	if (interrupt_sources[vector] != NULL) {
		return (EEXIST);
	}
	error = intr_event_create(&isrc->is_event, isrc, 0, vector, intr_disable_src, (mask_fn) isrc->is_pic->pic_enable_source, (mask_fn) isrc->is_pic->pic_eoi_source, intr_assign_cpu, "irq%d:", vector);
	if (error) {
		return (error);
	}
	simple_lock(&intrsrc_lock);
	if (interrupt_sources[vector] != NULL) {
		simple_unlock(&intrsrc_lock);
		intr_event_destroy(isrc->is_event);
		return (EEXIST);
	}
	intrcnt_register(isrc);
	interrupt_sources[vector] = isrc;
	isrc->is_handlers = 0;
	simple_unlock(&intrsrc_lock);
	return (0);
}


struct intsrc *
intr_lookup_source(int vector)
{
	if (vector < 0 || vector >= num_io_irqs) {
		return (NULL);
	}
	return (interrupt_sources[vector]);
}

//int intr_add_handler(const char *name, int vector, driver_filter_t filter, driver_intr_t handler, void *arg, enum intr_type flags, void **cookiep, int domain);
//int	intr_remove_handler(void *cookie);

int
intr_config_intr(int vector, enum intr_trigger trig, enum intr_polarity pol)
{
	struct intsrc *isrc;

	isrc = intr_lookup_source(vector);
	if (isrc == NULL) {
		return (EINVAL);
	}
	return (isrc->is_pic->pic_config_intr(isrc, trig, pol));
}

static void
intr_disable_src(void *arg)
{
	struct intsrc *isrc;

	isrc = arg;
	isrc->is_pic->pic_disable_source(isrc, PIC_EOI);
}

void
intr_execute_handlers(struct intsrc *isrc, struct trapframe *frame)
{
	struct intr_event *ie;
	int vector;

	/*
	 * We count software interrupts when we process them.  The
	 * code here follows previous practice, but there's an
	 * argument for counting hardware interrupts when they're
	 * processed too.
	 */
	(*isrc->is_count)++;
	VM_CNT_INC(v_intr);

	ie = isrc->is_event;

	/*
	 * XXX: We assume that IRQ 0 is only used for the ISA timer
	 * device (clk).
	 */
	vector = isrc->is_pic->pic_vector(isrc);
	if (vector == 0)
		clkintr_pending = 1;

	/*
	 * For stray interrupts, mask and EOI the source, bump the
	 * stray count, and log the condition.
	 */
	if (intr_event_handle(ie, frame) != 0) {
		isrc->is_pic->pic_disable_source(isrc, PIC_EOI);
		(*isrc->is_straycount)++;
		if (*isrc->is_straycount < MAX_STRAY_LOG)
			log(LOG_ERR, "stray irq%d\n", vector);
		else if (*isrc->is_straycount == MAX_STRAY_LOG)
			log(LOG_CRIT, "too many stray irq %d's: not logging anymore\n", vector);
	}
}

void
intr_resume(bool suspend_cancelled)
{
	struct pic *pic;

#ifndef DEV_ATPIC
	atpic_reset();
#endif

	simple_lock(&intrpic_lock);
	TAILQ_FOREACH(pic, &pics, pics) {
		if (pic->pic_resume != NULL)
			pic->pic_resume(pic, suspend_cancelled);
	}
	simple_unlock(&intrpic_lock);
}

void
intr_suspend(void)
{
	struct pic *pic;

	simple_lock(&intrpic_lock);
	TAILQ_FOREACH_REVERSE(pic, &pics, pics_head, pics) {
		if (pic->pic_suspend != NULL)
			pic->pic_suspend(pic);
	}
	simple_unlock(&intrpic_lock);
}

static void
intrcnt_setname(const char *name, int index)
{
	snprintf(intrnames + (MAXCOMLEN + 1) * index, MAXCOMLEN + 1, "%-*s", MAXCOMLEN, name);
}

static void
intrcnt_updatename(struct intsrc *is)
{

	intrcnt_setname(is->is_event->ie_fullname, is->is_index);
}

static void
intrcnt_register(struct intsrc *is)
{
	char straystr[MAXCOMLEN + 1];

	KASSERT(is->is_event != NULL, ("%s: isrc with no event", __func__));
	simple_lock(&intrcnt_lock);
	MPASS(intrcnt_index + 2 <= nintrcnt);
	is->is_index = intrcnt_index;
	intrcnt_index += 2;
	snprintf(straystr, MAXCOMLEN + 1, "stray irq%d", is->is_pic->pic_vector(is));
	intrcnt_updatename(is);
	is->is_count = &intrcnt[is->is_index];
	intrcnt_setname(straystr, is->is_index + 1);
	is->is_straycount = &intrcnt[is->is_index + 1];
	simple_unlock(&intrcnt_lock);
}

void
intrcnt_add(const char *name, u_long **countp)
{
	simple_lock(&intrcnt_lock);
	MPASS(intrcnt_index < nintrcnt);
	*countp = &intrcnt[intrcnt_index];
	intrcnt_setname(name, intrcnt_index);
	intrcnt_index++;
	simple_unlock(&intrcnt_lock);
}

static void
intr_init(void *dummy)
{
	TAILQ_INIT(&pics);
	simple_lock_init(&intrpic_lock, "intrpic_lock");
	simple_lock_init(&intrsrc_lock, "intrsrc_lock");
	simple_lock_init(&intrcnt_lock, "intrcnt_lock");
}

void
intr_reprogram(void)
{
	struct intsrc *is;
	u_int v;

	simple_lock(&intrsrc_lock);
	for (v = 0; v < num_io_irqs; v++) {
		is = interrupt_sources[v];
		if (is == NULL) {
			continue;
		}
		if (is->is_pic->pic_reprogram_pin != NULL) {
			is->is_pic->pic_reprogram_pin(is);
		}
	}
	simple_unlock(&intrsrc_lock);
}
