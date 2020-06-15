/* UNIX V7 source code: see /COPYRIGHT or www.tuhs.org for details. */
#include <sys/file.h>

#define	NINDEX		15

struct mpx {
	struct chan 	*mpx_ch;	/* pointer to mpx channel */

	struct proc 	*mpx_p; 	/* pointer to proc; */
	struct protosw 	*mpx_proto;
	struct mpxbuf 	*mpx_buf;
	short 			mpx_pgrp;
	short 			mpx_state;
	struct mpx_group *g_group;

	struct rh { 		/* header returned on read of mpx */
		int 	index;
		int 	count;
		int 	ccount;
	} rh;

	struct wh { 		/*  head expected on write of mpx */
		int 	index;
		int		count;
		int		ccount;
		char 	*data;
	} wh;
};

struct mpxbuf {
	u_short		cnt;
};

struct	mx_args {
	char 	*m_name;
	int		m_cmd;
	int		m_arg[3];
};

struct mpx_group {
	short				g_state;
	char				g_index;
	char				g_rot;
	struct mpx_group 	*g_group;
	struct vnode		*g_vnode;
	struct vattr		*g_vattr;
	struct file			*g_file;
	short				g_rotmask;
	short				g_datq;
	struct chan 		*g_chans[NINDEX];
};

//#ifdef KERNEL
/*
 * internal structure for channel
 */
struct mpx_chan {
	short 				c_flags;
	char 				c_index;
	char 				c_line;
	struct	mpx_group 	*c_group;
	struct	user 		*c_u;
	struct	file 		*c_fy;
	struct	tty	 		*c_ttyp;
	struct	clist 		c_ctlx;
	int 				c_pgrp;
	struct	tty 		*c_ottyp;
	char 				c_oline;
	union {
		struct	clist	datq;
	} cx;
	union {
		struct	clist	datq;
		struct	chan	*c_chan;
	} cy;
	struct	clist 		c_ctly;
};


struct mpx_schan {
	struct 	proc 		*p;		/* pointer to proc */
	struct	mpx_group 	*c_group;
	short				c_flags;
	char				c_index;
	char				c_line;
	struct	file		*c_fy;
	struct	tty			*c_ttyp;
	struct	clist		c_ctlx;
	int					c_pgrp;
};

/*
 * Tuneable Multiplexor variables
 */
#define	NGROUPS		10	/* number of mpx files permitted at one time */
#define	NCHANS		20	/* number of channel structures */
#define	NPORTS		30	/* number of channels to i/o ports */
#define	CNTLSIZ		10
#define	NLEVELS		4
#define	NMSIZE		50	/* max size of mxlstn file name */

/*
 * flags
 */
#define	INUSE	01
#define COPEN	02
#define	XGRP	04
#define	YGRP	010
#define	WCLOSE	020
#define	ISGRP	0100
#define	BLOCK	0200
#define	EOTMARK	0400
#define	SIGBLK	01000
#define	BLKMSG	01000
#define	ENAMSG	02000
#define	WFLUSH	04000
#define	NMBUF	010000
#define	PORT	020000
#define	ALT	040000

//#endif
/*
 * mpxchan command codes
 */
#define	MPX		5
#define	MPXN	6
#define	CHAN	1
#define JOIN	2
#define EXTR	3
#define	ATTACH	4
#define	CONNECT	7
#define	DETACH	8
#define	DISCON	9
#define DEBUG	10
#define	NPGRP	11
#define	CSIG	12
#define PACK	13

#define NDEBUGS	30

/*
 * control channel message codes
 */
#define M_WATCH 1
#define M_CLOSE 2
#define	M_EOT	3
#define	M_OPEN	4
#define	M_BLK	5
#define	M_UBLK	6
#define	DO_BLK	7
#define	DO_UBLK	8
#define	M_IOCTL	12
#define	M_SIG	14

/*
 * debug codes other than mpxchan cmds
 */
#define MCCLOSE 29
#define MCOPEN	28
#define	ALL		27
#define SCON	26
#define	MSREAD	25
#define	SDATA	24
#define	MCREAD	23
#define MCWRITE	22

#define MPX_ASYNC 	0x004
#define	MPX_NBIO	0x008	/* non-blocking ops */

/* put in stat.h*/
#define S_IFCHAN  0200000; /* multiplexor */

/* 3BSD: user.h, inode.h, file.h, stat.h */
caddr_t	u_dirp;			/* pathname pointer */
#define	IFMPC	0030000	/* multiplexed char special */
#define	IFMPB	0070000	/* multiplexed block special */
#define	FMPX	010
#define	S_IFMPC	0030000	/* multiplexed char special */
#define	S_IFMPB	0070000	/* multiplexed block special */

//#ifdef  KERNEL
int mpx_read (struct file *fp, struct uio *uio, struct ucred *cred);
int mpx_write (struct file *fp, struct uio *uio, struct ucred *cred);
int mpx_ioctl (struct file *fp, u_long cmd, caddr_t data, struct proc *p);
int mpx_select (struct file *fp, int which, struct proc *p);
int mpx_close (struct file *fp, struct proc *p);

int	mpxclose (struct mpx *mpx);
int mpxsend (struct mpx *mpx, struct mbuf *addr, struct uio *uio, struct mbuf *top, struct mbuf *control, int flags);
int mpxreceive (struct mpx *mpx, struct mbuf **paddr, struct uio *uio, struct mbuf **mp0, struct mbuf **controlp, int *flagsp);
//#endif /* KERNEL */
