/*
 * Copyright (c) 2010 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/tty.h>
#include <sys/poll.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <machine/console.h>

#include <dev/kbd/kbdreg.h>

int
genkbd_probe(keyboard_switch_t *sw, int unit, void *arg, int flags)
{
	int error;

	error = (*sw->probe)(unit, arg, flags);

	return (error);
}

int
genkbd_init(keyboard_switch_t *sw, int unit, keyboard_t **kbdpp, void *arg, int flags)
{
	int error;

	error = (*sw->init)(unit, kbdpp, arg, flags);
	return (error);
}

int
genkbd_term(keyboard_t *kbd)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->term)(kbd);
	if (error)
		kbd_unlock(kbd);
	/* kbd structure is stale if error is 0 */
	return (error);
}

/*
 * Handle a keyboard interrupt.
 *
 * Be suspicious, just in case kbd_intr() is called from an interrupt
 * before the keyboard switch is completely installed.
 */
int
genkbd_intr(keyboard_t *kbd, void *arg)
{
	int error;
	int i;

	error = EINVAL;
	i = kbd->kb_index;

	if (i >= 0 && i < KBD_MAXKEYBOARDS && kbdsw[i]) {
		kbd_lock(kbd);
		error = (*kbdsw[i]->intr)(kbd, arg);
		kbd_unlock(kbd);
	}
	return (error);
}

int
genkbd_test_if(keyboard_t *kbd)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->test_if)(kbd);
	kbd_unlock(kbd);

	return (error);
}

int
genkbd_enable(keyboard_t *kbd)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->enable)(kbd);
	kbd_unlock(kbd);

	return (error);
}

int
genkbd_disable(keyboard_t *kbd)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->disable)(kbd);
	kbd_unlock(kbd);

	return (error);
}

int
genkbd_read(keyboard_t *kbd, int wait)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->read)(kbd, wait);
	kbd_unlock(kbd);

	return (error);
}

int
genkbd_check(keyboard_t *kbd)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->check)(kbd);
	kbd_unlock(kbd);

	return (error);
}

u_int
genkbd_read_char(keyboard_t *kbd, int wait)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->read_char)(kbd, wait);
	kbd_unlock(kbd);

	return (error);
}

int
genkbd_check_char(keyboard_t *kbd)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->check_char)(kbd);
	kbd_unlock(kbd);

	return (error);
}

int
genkbd_ioctl(keyboard_t *kbd, u_long cmd, caddr_t data)
{
	int error;

	if (kbd) {
		kbd_lock(kbd);
		error = (*kbdsw[kbd->kb_index]->ioctl)(kbd, cmd, data);
		kbd_unlock(kbd);
	} else {
		error = ENODEV;
	}
	return (error);
}

int
genkbd_lock(keyboard_t *kbd, int xlock)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->lock)(kbd, xlock);
	kbd_unlock(kbd);

	return (error);
}

void
genkbd_clear_state(keyboard_t *kbd)
{
	kbd_lock(kbd);
	(*kbdsw[kbd->kb_index]->clear_state)(kbd);
	kbd_unlock(kbd);
}

int
genkbd_get_state(keyboard_t *kbd, void *buf, size_t len)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->get_state)(kbd, buf, len);
	kbd_unlock(kbd);

	return (error);
}

int
genkbd_set_state(keyboard_t *kbd, void *buf, size_t len)
{
	int error;

	kbd_lock(kbd);
	error = (*kbdsw[kbd->kb_index]->set_state)(kbd, buf, len);
	kbd_unlock(kbd);

	return (error);
}

u_char *
genkbd_get_fkeystr(keyboard_t *kbd, int fkey, size_t *len)
{
	u_char *retstr;

	kbd_lock(kbd);
	retstr = (*kbdsw[kbd->kb_index]->get_fkeystr)(kbd, fkey, len);
	kbd_unlock(kbd);

	return (retstr);
}

/*
 * Polling mode set by debugger, we cannot lock!
 */
int
genkbd_poll(keyboard_t *kbd, int on)
{
	int error;

	if (!on)
		KBD_UNPOLL(kbd);
	error = (*kbdsw[kbd->kb_index]->poll)(kbd, on);
	if (on)
		KBD_POLL(kbd);

	return (error);
}

void
genkbd_diag(keyboard_t *kbd, int level)
{
	kbd_lock(kbd);
	(*kbdsw[kbd->kb_index]->diag)(kbd, level);
	kbd_unlock(kbd);
}
