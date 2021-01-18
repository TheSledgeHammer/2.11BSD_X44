/*
 * genkbds.h
 *
 *  Created on: 16 Jan 2021
 *      Author: marti
 */

#ifndef SYS_DEVEL_DEV_GENKBDS_H_
#define SYS_DEVEL_DEV_GENKBDS_H_

struct genkbddev {
	void	(*gk_init)(struct genkbddev *);
	void	(*gk_probe)(struct genkbddev *);
	void	(*gk_putc)(genkbd_softc_t *sc, char c);
	size_t	(*gk_getc)(genkbd_softc_t *sc, char *buf, size_t len);
	void	**gk_devs;
};

extern struct genkbddev *gk_tab;

void	gkinit (void);
int		gkopen (dev_t, int, int, struct proc *);
int		gkclose (dev_t, int, int, struct proc *);
int		gkread (dev_t, struct uio *, int);
int		gkwrite (dev_t, struct uio *, int);
int		gkioctl (dev_t, u_long, caddr_t, int, struct proc *);
int		gkpoll (dev_t, int, struct proc *);
int		gkgetc (void);
void	gkputc (int);

#define	dev_type_gkprobe(n)	void n (struct genkbddev *)
#define	dev_type_gkinit(n)	void n (struct genkbddev *)
#define	dev_type_gkgetc(n)	int n (genkbd_softc_t *, char)
#define	dev_type_gkputc(n)	void n (genkbd_softc_t *, char *, size_t)

#define	dev_decl(n,t)		__CONCAT(dev_type_,t)(__CONCAT(n,t))
#define	dev_init(n,t)		__CONCAT(n,t)

#define	genkbd_decl(n) 												\
	dev_decl(n,gkprobe); dev_decl(n,gkinit); dev_decl(n,gkgetc); 	\
	dev_decl(n,gkputc);

#define	genkbd_init(n) { \
	dev_init(n,gkprobe), dev_init(n,gkinit), dev_init(n,gkgetc), 	\
	dev_init(n,gkputc) }

#endif /* SYS_DEVEL_DEV_GENKBDS_H_ */
