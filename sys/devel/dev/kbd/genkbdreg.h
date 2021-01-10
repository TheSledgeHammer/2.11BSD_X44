/*
 * genkbreg.h
 *
 *  Created on: 10 Jan 2021
 *      Author: marti
 */

#ifndef _DEV_KBD_GENKBDREG_H_
#define _DEV_KBD_GENKBDREG_H_

/* keyboard */
struct keyboard {
	int				kb_flags;	/* internal flags */
	struct lock		*kb_lock;
};

#define KBD_MAXKEYBOARDS	16


/* Locking functions */
#define kbd_lock_init(k)	(simple_lock_init((k)->kb_lock->lk_lnterlock, "kbd_lock"))
#define kbd_lock(k)	 		(simple_lock((k)->kb_lock->lk_lnterlock))
#define kbd_unlock(k)		(simple_unlock((k)->kb_lock->lk_lnterlock))

/* keyboard function table */
typedef int				kbd_probe_t(int unit, void *arg, int flags);
typedef int				kbd_init_t(int unit, keyboard_t **kbdp,	void *arg, int flags);
typedef int				kbd_term_t(keyboard_t *kbd);
typedef int				kbd_intr_t(keyboard_t *kbd, void *arg);
typedef int				kbd_test_if_t(keyboard_t *kbd);
typedef int				kbd_enable_t(keyboard_t *kbd);
typedef int				kbd_disable_t(keyboard_t *kbd);
typedef int				kbd_read_t(keyboard_t *kbd, int wait);
typedef int				kbd_check_t(keyboard_t *kbd);
typedef u_int			kbd_read_char_t(keyboard_t *kbd, int wait);
typedef int				kbd_check_char_t(keyboard_t *kbd);
typedef int				kbd_ioctl_t(keyboard_t *kbd, u_long cmd, caddr_t data);
typedef int				kbd_lock_t(keyboard_t *kbd, int lock);
typedef void			kbd_clear_state_t(keyboard_t *kbd);
typedef int				kbd_get_state_t(keyboard_t *kbd, void *buf, size_t len);
typedef int				kbd_set_state_t(keyboard_t *kbd, void *buf, size_t len);
typedef u_char			*kbd_get_fkeystr_t(keyboard_t *kbd, int fkey, size_t *len);
typedef int				kbd_poll_mode_t(keyboard_t *kbd, int on);
typedef void			kbd_diag_t(keyboard_t *kbd, int level);

typedef struct keyboard_switch {
	kbd_probe_t			*probe;
	kbd_init_t			*init;
	kbd_term_t			*term;
	kbd_intr_t			*intr;
	kbd_test_if_t		*test_if;
	kbd_enable_t		*enable;
	kbd_disable_t		*disable;
	kbd_read_t			*read;
	kbd_check_t			*check;
	kbd_read_char_t		*read_char;
	kbd_check_char_t 	*check_char;
	kbd_ioctl_t			*ioctl;
	kbd_lock_t			*lock;
	kbd_clear_state_t 	*clear_state;
	kbd_get_state_t		*get_state;
	kbd_set_state_t		*set_state;
	kbd_get_fkeystr_t 	*get_fkeystr;
	kbd_poll_mode_t 	*poll;
	kbd_diag_t			*diag;
} keyboard_switch_t;

int genkbd_probe(keyboard_switch_t *sw, int unit, void *arg, int flags);
int genkbd_init(keyboard_switch_t *sw, int unit, keyboard_t **kbdpp, void *arg, int flags);

kbd_term_t			genkbd_term;
kbd_intr_t			genkbd_intr;
kbd_test_if_t		genkbd_test_if;
kbd_enable_t		genkbd_enable;
kbd_disable_t		genkbd_disable;
kbd_read_t			genkbd_read;
kbd_check_t			genkbd_check;
kbd_read_char_t		genkbd_read_char;
kbd_check_char_t 	genkbd_check_char;
kbd_ioctl_t			genkbd_ioctl;
kbd_lock_t			genkbd_lock;
kbd_clear_state_t 	genkbd_clear_state;
kbd_get_state_t		genkbd_get_state;
kbd_set_state_t		genkbd_set_state;
kbd_get_fkeystr_t 	genkbd_get_fkeystr;
kbd_poll_mode_t 	genkbd_poll;
kbd_diag_t			genkbd_diag;

/* global variables */
extern keyboard_switch_t *kbdsw[KBD_MAXKEYBOARDS];


#endif /* _DEV_KBD_GENKBDREG_H_ */
