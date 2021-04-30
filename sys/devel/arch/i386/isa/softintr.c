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

struct pic softintr_template = {
		.pic_type = PIC_SOFT,
		.pic_hwmask = softpic_hwmask,
		.pic_hwunmask = softpic_hwunmask,
		.pic_addroute = softpic_addroute,
		.pic_delroute = softpic_delroute,
		.pic_register = softpic_register_pic
};

void
softpic_hwmask(struct ioapic_intsrc *intpin, int pin)
{
	/* Do Nothing */
}

void
softpic_hwunmask(struct ioapic_intsrc *intpin, int pin)
{
	/* Do Nothing */
}

static void
softpic_addroute(struct ioapic_intsrc *intpin, struct cpu_info *ci, int pin, int idtvec, int type)
{
	/* Do Nothing */
}


static void
softpic_delroute(struct ioapic_intsrc *intpin, struct cpu_info *ci, int pin, int idtvec, int type)
{
	/* Do Nothing */
}

static void
softpic_register_pic()
{
	intr_register_pic(&softintr_template);
}
