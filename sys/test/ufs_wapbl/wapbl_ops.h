/*
 * wapbl_ops.h
 *
 *  Created on: 17 Feb 2020
 *      Author: marti
 */

#ifndef _SYS_WAPBL_OPS_H_
#define	_SYS_WAPBL_OPS_H
/*
 * This operations vector is so wapbl can be wrapped into a filesystem lkm.
 * XXX Eventually, we want to move this functionality
 * down into the filesystems themselves so that this isn't needed.
 */
struct wapbl_ops {
	void (*wo_wapbl_discard)(struct wapbl *);
	int (*wo_wapbl_replay_isopen)(struct wapbl_replay *);
	int (*wo_wapbl_replay_can_read)(struct wapbl_replay *, daddr_t, long);
	int (*wo_wapbl_replay_read)(struct wapbl_replay *, void *, daddr_t, long);
	void (*wo_wapbl_add_buf)(struct wapbl *, struct buf *);
	void (*wo_wapbl_remove_buf)(struct wapbl *, struct buf *);
	void (*wo_wapbl_resize_buf)(struct wapbl *, struct buf *, long, long);
	int (*wo_wapbl_begin)(struct wapbl *, const char *, int);
	void (*wo_wapbl_end)(struct wapbl *);
	void (*wo_wapbl_junlock_assert)(struct wapbl *);
	void (*wo_wapbl_jlock_assert)(struct wapbl *);
	void (*wo_wapbl_biodone)(struct buf *);
};

#define WAPBL_DISCARD(MP)						\
    (*(MP)->mnt_wapbl_op->wo_wapbl_discard)((MP)->mnt_wapbl)
#define WAPBL_REPLAY_ISOPEN(MP)						\
    (*(MP)->mnt_wapbl_op->wo_wapbl_replay_isopen)((MP)->mnt_wapbl_replay)
#define WAPBL_REPLAY_CAN_READ(MP, BLK, LEN)				\
    (*(MP)->mnt_wapbl_op->wo_wapbl_replay_can_read)((MP)->mnt_wapbl_replay, \
    (BLK), (LEN))
#define WAPBL_REPLAY_READ(MP, DATA, BLK, LEN)				\
    (*(MP)->mnt_wapbl_op->wo_wapbl_replay_read)((MP)->mnt_wapbl_replay,	\
    (DATA), (BLK), (LEN))
#define WAPBL_ADD_BUF(MP, BP)						\
    (*(MP)->mnt_wapbl_op->wo_wapbl_add_buf)((MP)->mnt_wapbl, (BP))
#define WAPBL_REMOVE_BUF(MP, BP)					\
    (*(MP)->mnt_wapbl_op->wo_wapbl_remove_buf)((MP)->mnt_wapbl, (BP))
#define WAPBL_RESIZE_BUF(MP, BP, OLDSZ, OLDCNT)				\
    (*(MP)->mnt_wapbl_op->wo_wapbl_resize_buf)((MP)->mnt_wapbl, (BP),	\
    (OLDSZ), (OLDCNT))
#define WAPBL_BEGIN(MP)							\
    (*(MP)->mnt_wapbl_op->wo_wapbl_begin)((MP)->mnt_wapbl,		\
    __FILE__, __LINE__)
#define WAPBL_END(MP)							\
    (*(MP)->mnt_wapbl_op->wo_wapbl_end)((MP)->mnt_wapbl)
#define WAPBL_JUNLOCK_ASSERT(MP)					\
    (*(MP)->mnt_wapbl_op->wo_wapbl_junlock_assert)((MP)->mnt_wapbl)
#define WAPBL_JLOCK_ASSERT(MP)						\
    (*(MP)->mnt_wapbl_op->wo_wapbl_jlock_assert)((MP)->mnt_wapbl)


#endif /* _SYS_WAPBL_OPS_H */
