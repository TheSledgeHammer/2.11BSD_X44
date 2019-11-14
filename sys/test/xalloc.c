/*
 * xalloc.c
 *
 *  Created on: 14 Nov 2019
 *      Author: marti
 */
#include <sys/param.h>
#include <sys/proc.h>
#include <pool2.h>
#include <xalloc.h>

void
xinit(void)
{
	int i, n, upages, kpages;
	u_long maxpages;
	Confmem *m;
	Pallocmem *pm;
	Conf conf;
	Palloc palloc;
	Hole *h, *eh;

	eh = &xlists.hole[NHOLE - 1];
	for (h = xlists.hole; h < eh; h++)
		h->link = h + 1;

	xlists.flist = xlists.hole;
	upages = conf.upages;
	kpages = conf.npage - upages;
	pm = palloc.mem;
	for (i = 0; i < nelem(conf.mem); i++) {
		m = &conf.mem[i];
		n = m->npage;
		if (n > kpages)
			n = kpages;
		/* don't try to use non-KADDR-able memory for kernel */
		maxpages = cankaddr(m->base) / BY2PG;
		if (n > maxpages)
			n = maxpages;
		/* first give to kernel */
		if (n > 0) {
			m->kbase = (u_long) KADDR(m->base);
			m->klimit = (u_long) KADDR(m->base + n * BY2PG);
			xhole(m->base, n * BY2PG);
			kpages -= n;
		}
		/* if anything left over, give to user */
		if (n < m->npage) {
			if (pm >= palloc.mem + nelem(palloc.mem)) {
				print("xinit: losing %lud pages\n", m->npage - n);
				continue;
			}
			pm->base = m->base + n * BY2PG;
			pm->npage = m->npage - n;
			pm++;
		}
	}
}

void*
xspanalloc(u_long size, int align, u_long span)
{
	u_long a, v, t;
	a = (u_long) xalloc(size + align + span);
	if (a == 0)
		panic("xspanalloc: %lud %d %lux", size, align, span);

	if (span > 2) {
		v = (a + span) & ~(span - 1);
		t = v - a;
		if (t > 0)
			xhole(PADDR(a), t);
		t = a + span - v;
		if (t > 0)
			xhole(PADDR(v + size + align), t);
	} else
		v = a;

	if (align > 1)
		v = (v + align) & ~(align - 1);

	return (void*) v;
}

void*
xallocz(u_long size, int zero)
{
	Xhdr *p;
	Hole *h, **l;

	/* add room for magix & size overhead, round up to nearest vlong */
	size += BY2V + offsetof(p, data[0]);
	size &= ~(BY2V - 1);

	ilock(&xlists);
	l = &xlists.table;
	for (h = *l; h; h = h->link) {
		if (h->size >= size) {
			p = (Xhdr*) KADDR(h->addr);
			h->addr += size;
			h->size -= size;
			if (h->size == 0) {
				*l = h->link;
				h->link = xlists.flist;
				xlists.flist = h;
			}
			iunlock(&xlists);
			if (zero)
				memset(p, 0, size);
			p->magix = MAGIC_HOLE;
			p->size = size;
			return p->data;
		}
		l = &h->link;
	}
	iunlock(&xlists);
	return NULL;
}

void*
xalloc(u_long size)
{
	return xallocz(size, 1);
}

void
xfree(void *p)
{
	Xhdr *x;

	x = (Xhdr*) ((u_long) p - offsetof(Xhdr, data[0]));
	if (x->magix != MAGIC_HOLE) {
		xsummary();
		panic("xfree(%#p) %#ux != %#lux", p, MAGIC_HOLE, x->magix);
	}
	xhole(PADDR((uintptr) x), x->size);
}

int
xmerge(void *vp, void *vq)
{
	Xhdr *p, *q;

	p = (Xhdr*) (((u_long) vp - offsetof(Xhdr, data[0])));
	q = (Xhdr*) (((u_long) vq - offsetof(Xhdr, data[0])));
	if (p->magix != MAGIC_HOLE || q->magix != MAGIC_HOLE) {
		int i;
		u_long *wd;
		void *badp;

		xsummary();
		badp = (p->magix != MAGIC_HOLE ? p : q);
		wd = (u_long*) badp - 12;
		for (i = 24; i-- > 0;) {
			print("%#p: %lux", wd, *wd);
			if (wd == badp)
				print(" <-");
			print("\n");
			wd++;
		}
		panic("xmerge(%#p, %#p) bad magic %#lux, %#lux", vp, vq, p->magix,
				q->magix);
	}
	if ((u_char*) p + p->size == (u_char*) q) {
		p->size += q->size;
		return 1;
	}
	return 0;
}

void
xhole(u_long addr, u_long size)
{
	u_long top;
	Hole *h, *c, **l;

	if (size == 0)
		return;

	top = addr + size;
	ilock(&xlists);
	l = &xlists.table;
	for (h = *l; h; h = h->link) {
		if (h->top == addr) {
			h->size += size;
			h->top = h->addr + h->size;
			c = h->link;
			if (c && h->top == c->addr) {
				h->top += c->size;
				h->size += c->size;
				h->link = c->link;
				c->link = xlists.flist;
				xlists.flist = c;
			}
			iunlock(&xlists);
			return;
		}
		if (h->addr > addr)
			break;
		l = &h->link;
	}
	if (h && top == h->addr) {
		h->addr -= size;
		h->size += size;
		iunlock(&xlists);
		return;
	}

	if (xlists.flist == NULL) {
		iunlock(&xlists);
		print("xfree: no free holes, leaked %lud bytes\n", size);
		return;
	}

	h = xlists.flist;
	xlists.flist = h->link;
	h->addr = addr;
	h->top = top;
	h->size = size;
	h->link = *l;
	*l = h;
	iunlock(&xlists);
}
