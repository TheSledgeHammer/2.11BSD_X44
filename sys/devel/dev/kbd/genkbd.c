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

#include <machine/console.h>

#include <dev/kbd/genkbdreg.h>

/*
 * genkbd is used for all keyboard cdevsw routines.
 *
 * keyboard_switch has an overlaid setup on the genkbd's cfdriver (FreeBSD's attach & detach equivalent)
 *
 * Todo:
 * - match(probe) & attach for genkbd
 * 	- issues:
 * 		- searching for said keyboard in probe
 * 		- allocating said keyboard in attach
 *
 * 	- Solution 1:
 * 		- setup cfdriver's independently of genkbd. allocating genkbd_softc via a helper method
 *
 * 	- Solution 2:
 * 		- Setup genkbd cfdriver in a com/isa fashion. With other keyboards holding a pointer to genkbd_softc
 */

/* Generic Keyboard Device */
struct genkbd_device {
	struct genkbd_softc		*gkb_sc;

	struct device 			gkb_dev;			/* base device */

	int						gkb_index;			/* kbdio index# */
	int						gkb_minor;			/* minor number of the sub-device */

	char					*gkb_name;			/* driver name */
	int						gkb_unit;			/* unit # */
};

struct cfdriver genkbd_cd = {
	NULL, "genkbd", genkbd_probe, genkbd_attach, DV_DULL, sizeof(struct genkbd_softc)
};

struct genkbd_driver {
	int (*probe)();
	int (*attach)();
};

void
genkbd_init(parent, name, unit, index)
	struct device *parent;
	char *name;
	int index, unit;
{
	struct genkbd_device *gkbd = (struct genkbd_device *) parent;

	gkbd->gkb_name = name;
	gkbd->gkb_unit = unit;
	gkbd->gkb_index = index;
	gkbd->gkb_sc = malloc(sizeof(struct genkbd_softc *), M_DEVBUF, M_WAITOK | M_ZERO);
}

void
kbd_device(driver, int index, uid_t uid, gid_t gid, int perms, /* ??, */char *name, int unit)
{

}

int
genkbd_probe(struct genkbd_driver *gkbd, struct device *parent, struct cfdata *match, void *aux)
{
	int error;

	error = (*gkbd->probe)(gkbd, parent, match, aux);
	return (error);
}

int
genkbd_attach(gkbd, parent, self, aux)
	struct genkbd_driver *gkbd;
	struct device *parent, *self;
	void *aux;
{
	int error;

	error = (*gkbd->attach)(gkbd, parent, self, aux);
	return (error);
}

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

	kbd_lock_init(*kbdpp);
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
