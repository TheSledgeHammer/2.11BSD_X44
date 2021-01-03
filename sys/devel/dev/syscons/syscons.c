/*
 * syscons.c
 *
 *  Created on: 31 Dec 2020
 *      Author: marti
 */

static cn_probe_t 		sc_cnprobe;
static cn_init_t 		sc_cninit;
static cn_term_t 		sc_cnterm;
static cn_getc_t 		sc_cngetc;
static cn_putc_t 		sc_cnputc;
static cn_grab_t 		sc_cngrab;
static cn_ungrab_t 		sc_cnungrab;

static tsw_open_t 		sctty_open;
static tsw_close_t 		sctty_close;
static tsw_outwakeup_t 	sctty_outwakeup;
static tsw_ioctl_t 		sctty_ioctl;
static tsw_mmap_t 		sctty_mmap;

static struct ttydevsw sc_ttydevsw = {
	.tsw_open = sctty_open,
	.tsw_close = sctty_close,
	.tsw_outwakeup = sctty_outwakeup,
	.tsw_ioctl = sctty_ioctl,
	.tsw_mmap = sctty_mmap,
};

static d_ioctl_t consolectl_ioctl;
static d_close_t consolectl_close;

static struct cdevsw consolectl_devsw = {
	.d_version = D_VERSION,
	.d_flags = D_NEEDGIANT | D_TRACKCLOSE,
	.d_ioctl = consolectl_ioctl,
	.d_close = consolectl_close,
	.d_name = "consolectl",
};

