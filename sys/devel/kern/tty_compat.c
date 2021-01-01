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

/* API's for compatibility or expanding tty */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/devsw.h>

/* cdevsw tty */
dev_type_open(ctty_open);
dev_type_close(ctty_close);
dev_type_read(ctty_read);
dev_type_write(ctty_write);
dev_type_ioctl(ctty_ioctl);
dev_type_stop(ctty_stop);
dev_type_tty(ctty_tty);
dev_type_select(ctty_select);
dev_type_poll(ctty_poll);
dev_type_mmap(ctty_mmap);
dev_type_strategy(ctty_strategy);
dev_type_discard(ctty_discard);

int
ctty_open(dev, flag, type, p)
	dev_t dev;
	int flag, type;
	struct proc *p;
{
	return (cdev_open(dev, flag, type, p));
}

int
ctty_close(dev, flag, type, p)
	dev_t dev;
	int flag, type;
	struct proc *p;
{
	return (cdev_close(dev, flag, type, p));
}

int
ctty_read(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	return (cdev_read(dev, uio, ioflag));
}

int
ctty_write(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	return (cdev_write(dev, uio, ioflag));
}

int
ctty_ioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	return (cdev_ioctl(dev, cmd, data, flag, p));
}

int
ctty_stop(tp, rw)
	struct tty *tp;
	int rw;
{
	return (cdev_stop(tp, rw));
}

struct tty *
ctty_tty(dev)
	dev_t dev;
{
	return (cdev_tty(dev));
}

int
ctty_select(dev, which, p)
	dev_t dev;
	int which;
	struct proc *p;
{
	return (cdev_select(dev, which, p));
}

int
ctty_poll(dev, events, type, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	return (cdev_poll(dev, events, p));
}

caddr_t
ctty_mmap(dev, off, flag)
	dev_t dev;
	off_t off;
	int flag;
{
	return (cdev_mmap(dev, off, flag));
}

int
ctty_strategy(bp)
	struct buf *bp;
{
	return (cdev_strategy(bp));
}

daddr_t
ctty_discard(dev, pos, len)
	dev_t dev;
	off_t pos, len;
{
	return (cdev_discard(dev, pos, len));
}

/* linesw tty */
dev_type_open(ltty_open);
dev_type_close(ltty_close);
dev_type_read(ltty_read);
dev_type_write(ltty_write);
dev_type_ioctl(ltty_ioctl);
dev_type_rint(ltty_rint);
dev_type_start(ltty_start);
dev_type_modem(ltty_modem);
dev_type_poll(ltty_poll);

int
ltty_open(dev, tp)
	dev_t dev;
	struct tty *tp;
{
	return (line_open(dev, tp));
}

int
ltty_close(tp, flag)
	struct tty *tp;
	int flag;
{
	return (line_close(tp, flag));
}

int
ltty_read(tp, uio, flag)
	struct tty *tp;
	struct uio *uio;
	int flag;
{
	return (line_read(tp, uio, flag));
}

int
ltty_write(tp, uio, flag)
	struct tty *tp;
	struct uio *uio;
	int flag;
{
	return (line_write(tp, uio, flag));
}

int
ltty_ioctl(tp, cmd, data, flag, p)
	struct tty *tp;
	caddr_t data;
	int cmd, flag;
	struct proc *p;
{
	return (line_ioctl(tp, cmd, data, flag, p));
}

int
ltty_rint(c, tp)
	int c;
	struct tty *tp;
{
	return (line_rint(c, tp));
}

int
ltty_start(tp)
	struct tty *tp;
{
	return (line_start(tp));
}

int
ltty_modem(tp, flag)
	struct tty *tp;
	int flag;
{
	return (line_modem(tp, flag));
}

int
ltty_poll(tp, flag, p)
	struct tty 	*tp;
	int 		flag;
	struct proc *p;
{
	return (line_poll(tp, flag, p));
}
