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

#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/fnv_hash.h>
#include <sys/user.h>

#include <devel/sys/devsw.h>

struct devswtable_head 	devswtable[MAXDEVSW]; /* hash table of devices in the device switch table (includes bdevsw, cdevsw & linesw) */

void
devsw_init()
{
	for(int i = 0; i < MAXDEVSW; i++) {
		TAILQ_INIT(&devswtable[i]);
	}

	dvop_malloc(&dvops);
	simple_lock_init(&devsw_lock, "devsw_lock");
}

int
devswtable_hash(data, dev)
	void 	*data;
	dev_t 	dev;
{
	Fnv32_t hash1 = fnv_32_buf(&dev, sizeof(&dev), FNV1_32_INIT) % MAXDEVSW;
	Fnv32_t hash2 = fnv_32_buf(&data, sizeof(&data), FNV1_32_INIT) % MAXDEVSW;
	return (hash1^hash2);
}

struct devswtable *
devsw_lookup(data, major)
	void 	*data;
	dev_t 	major;
{
	struct devswtable_head 		*bucket;
	register devswtable_entry_t entry;
	struct devswtable 			*devsw;

	bucket = &devswtable[devswtable_hash(data, major)];
	for(entry = TAILQ_FIRST(bucket); entry != NULL; entry = TAILQ_NEXT(entry, dve_link)) {
		devsw = entry->dve_devswtable;
		if(devsw->dv_data == data && devsw->dv_major == major) {
			return (devsw);
		}
	}
	return (NULL);
}

void
devsw_add(devsw, data, major)
	struct devswtable *devsw;
	void 			*data;
	dev_t 			major;
{
	struct devswtable_head 		*bucket;
	register devswtable_entry_t entry;

	devsw->dv_data = data;
	devsw->dv_major = major;

	bucket = &devswtable[devswtable_hash(data, major)];
	entry = (devswtable_entry_t) malloc((u_long) sizeof *entry, M_DEVSW, M_WAITOK);
	entry->dve_devswtable = devsw;

	TAILQ_INSERT_HEAD(bucket, entry, dve_link);
}

void
devsw_remove(data, major)
	void 	*data;
	dev_t 	major;
{
	struct devswtable_head 		*bucket;
	register devswtable_entry_t entry;
	struct devswtable 			*devsw;

	bucket = &devswtable[devswtable_hash(data, major)];
	for(entry = TAILQ_FIRST(bucket); entry != NULL; entry = TAILQ_NEXT(entry, dve_link)) {
		devsw = entry->dve_devswtable;
		if(devsw->dv_data == data && devsw->dv_major == major) {
			TAILQ_REMOVE(bucket, entry, dve_link);
		}
	}
}

int
devsw_lookup_bdevsw(devsw, dev)
	struct devswtable *devsw;
	dev_t 			dev;
{
	struct bdevsw *bdevsw;
	linesw = devop_lookup_bdevsw(devsw, dev);

	if(bdevsw == NULL) {
		return (ENXIO);
	}

	return (0);
}

int
devsw_lookup_cdevsw(devsw, dev)
	struct devswtable 	*devsw;
	dev_t 				dev;
{
	struct cdevsw *cdevsw;
	linesw = devop_lookup_cdevsw(devsw, dev);

	if(cdevsw == NULL) {
		return (ENXIO);
	}

	return (0);
}

int
devsw_lookup_linesw(devsw, dev)
	struct devswtable *devsw;
	dev_t 			dev;
{
	struct linesw *linesw;
	linesw = devop_lookup_linesw(devsw, dev);

	if(linesw == NULL) {
		return (ENXIO);
	}

	return (0);
}

int
devsw_dev_finder(major, devsw, devswtype)
    dev_t 			major;
    int 			devswtype;
    struct devswtable *devsw;
{
    int error = 0;

    switch(devswtype) {
    case BDEVTYPE:
    	struct bdevsw *bd = DTOB(devsw);
        if(bd) {
            error = devsw_lookup_bdevsw(devsw, major);
            return (error);
        }
        break;

    case CDEVTYPE:
        struct cdevsw *cd = DTOC(devsw);
        if(cd) {
            error = devsw_lookup_cdevsw(devsw, major);
            return (error);
        }
        break;

    case LINETYPE:
        struct linesw *ld = DTOL(devsw);
        if(ld) {
            error = devsw_lookup_linesw(devsw, major);
            return (error);
        }
        break;
    }

    return (error);
}

int
devsw_io(data, major, type)
	void 	*data;
	dev_t 	major;
	int		type;
{
	struct devswtbl *devsw = devsw_lookup(data, major);

	if(devsw == NULL) {
		return (ENXIO);
	}

	return (devsw_dev_finder(major, devsw, type));
}

int
bdev_open(dev_t dev, int flag, int devtype, struct proc *p)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_open)(dev, flag, devtype, p);

	return (rv);
}

int
bdev_close(dev_t dev, int fflag, int devtype, struct proc *p)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_close)(dev, fflag, devtype, p);

	return (rv);
}

int
bdev_strategy(dev_t dev, int fflag, int devtype, struct proc *p)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_strategy)(dev, fflag, devtype, p);

	return (rv);
}

int
bdev_ioctl(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_ioctl)(dev, cmd, data, fflag, p);

	return (rv);
}

/*
int
bdev_dump()
{

}

int
bdev_root()
{
	const struct bdevsw *d;
	dev_t dev;
	int rv;

	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_root)();

	return (rv);
}
*/

daddr_t
bdev_size(dev_t dev)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_psize)(dev);

	return (rv);
}

int
cdev_open(dev_t dev, int oflags, int devtype, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_open)(dev, oflags, devtype, p);

	return (rv);
}

int
cdev_close(dev_t dev, int fflag, int devtype, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_close)(dev, fflag, devtype, p);

	return (rv);
}

int
cdev_read(dev_t dev, struct uio *uio, int ioflag)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_read)(dev, uio, ioflag);

	return (rv);
}

int
cdev_write(dev_t dev, struct uio *uio, int ioflag)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_write)(dev, uio, ioflag);

	return (rv);
}

int
cdev_ioctl(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_ioctl)(dev, cmd, data, fflag, p);

	return (rv);
}

int
cdev_stop(struct tty *tp, int rw)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(tp->t_dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_stop)(tp, rw);

	return (rv);
}
/*
int
cdev_reset(int uban)
{

}
*/

struct tty
cdev_tty(dev_t dev)
{
	const struct cdevsw *c;
	struct tty *rv;
	int error;

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_tty)(dev);

	return (rv);
}

int
cdev_select(dev_t dev, int which, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_select)(dev, which, p);

	return (rv);
}

int
cdev_poll(dev_t dev, int events, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_poll)(dev, events, p);

	return (rv);
}

/*
int
cdev_mmap()
{

}
*/

int
cdev_strategy(struct buf *bp)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(bp->b_dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_strategy)(bp);

	return (rv);
}

int
line_open(dev_t dev, struct tty *tp)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_open)(dev, tp);

	return (rv);
}

int
line_close(struct tty *tp, int flag)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_close)(tp, flag);

	return (rv);
}

int
line_read(struct tty *tp, struct uio *uio, int flag)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_read)(tp, uio, flag);

	return (rv);
}

int
line_write(struct tty *tp, struct uio *uio, int flag)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_write)(tp, uio, flag);

	return (rv);
}

int
line_ioctl(struct tty *tp, int cmd, caddr_t data, int flag, struct proc *p)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_ioctl)(tp, cmd, data, flag, p);

	return (rv);
}

int
line_rint(int c, struct tty *tp)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_rint)(c, tp);

	return (rv);
}

/*
int
line_rend()
{

}

int
line_meta()
{

}
*/

int
line_start(struct tty *tp)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_start)(tp);

	return (rv);
}

int
line_modem(struct tty *tp, int flag)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_modem)(tp, flag);

	return (rv);
}

int
line_poll(struct tty *tp, int flag, struct proc *p)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_poll)(tp, flag, p);

	return (rv);
}
