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
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/user.h>

#include <arch/i386/include/pic.h>

static TAILQ_HEAD(pics_head, pic) 	pics;

static int
intr_pic_registered(pic)
	struct pic *pic;
{
	struct pic *p;

	TAILQ_FOREACH(p, &pics, pic_entry) {
		if (p == pic) {
			return (1);
		}
	}
	return (0);
}

int
intr_register_pic(pic)
	struct pic *pic;
{
	int error;

	if (intr_pic_registered(pic)) {
		error = EBUSY;
	} else {
		TAILQ_INSERT_TAIL(&pics, pic, pic_entry);
		error = 0;
	}

	return (error);
}

static struct pic *
intr_lookup_pic_template(pictemplate)
	int pictemplate;
{
	struct pic *pic, *template;
	TAILQ_FOREACH(pic, &pics, pic_entry) {
		if(pic->pic_type == pictemplate) {
			template = pic;
			return (template);
		}
	}
	return (NULL);
}

struct pic *
intr_handle_pic(pictemplate)
	int pictemplate;
{
	struct pic *ptemplate;

	switch(pictemplate) {
	case PIC_I8259:
		ptemplate = intr_lookup_pic_template(PIC_I8259);
		if(ptemplate == i8259_template) {
			return (ptemplate);
		}
		break;
	case PIC_IOAPIC:
		ptemplate = intr_lookup_pic_template(PIC_IOAPIC);
		if(ptemplate == ioapic_template) {
			return (ptemplate);
		}
		break;
	case PIC_LAPIC:
		ptemplate = intr_lookup_pic_template(PIC_LAPIC);
		if(ptemplate == lapic_template) {
			return (ptemplate);
		}
		break;
	case PIC_SOFT:
		ptemplate = intr_lookup_pic_template(PIC_SOFT);
		if(ptemplate == lapic_template) {
			return (ptemplate);
		}
		break;
	}
	return (NULL);
}

void
intr_pic_init(dummy)
	void *dummy;
{
	TAILQ_INIT(&pics);
}

void
intr_pic_hwmask(pictemplate, intpin, pin)
	struct ioapic_intsrc *intpin;
	int pin, pictemplate;
{
	register struct pic *pic;
	extern int cold;
	pic = intr_handle_pic(pictemplate);
	if (!cold && pic != NULL) {
		(*pic->pic_hwmask)(intpin, pin);
	}
}

void
intr_pic_hwunmask(pictemplate, intpin, pin)
	struct ioapic_intsrc *intpin;
	int pin, pictemplate;
{
	register struct pic *pic;
	extern int cold;
	pic = intr_handle_pic(pictemplate);
	if (!cold && pic != NULL) {
		(*pic->pic_hwunmask)(intpin, pin);
	}
}

void
intr_pic_addroute(pictemplate, intpin, ci, pin, idtvec, type)
	struct ioapic_intsrc *intpin;
	struct cpu_info *ci;
	int pin, idtvec, type, pictemplate;
{
	register struct pic *pic;
	pic = intr_handle_pic(pictemplate);
	if (pic != NULL) {
		(*pic->pic_addroute)(intpin, pin, idtvec, type);
	}
}

void
intr_pic_delroute(pictemplate, intpin, ci, pin, idtvec, type)
	struct ioapic_intsrc *intpin;
	struct cpu_info *ci;
	int pin, idtvec, type, pictemplate;
{
	register struct pic *pic;
	pic = intr_handle_pic(pictemplate);
	if (pic != NULL) {
		(*pic->pic_delroute)(intpin, ci, pin, idtvec, type);
	}
}
