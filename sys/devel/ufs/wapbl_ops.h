/*	$NetBSD: wapbl.h,v 1.21 2018/12/10 21:19:33 jdolecek Exp $	*/

/*-
 * Copyright (c) 2003,2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
#define WAPBL_REPLAY_ISOPEN(MP)					\
    (*(MP)->mnt_wapbl_op->wo_wapbl_replay_isopen)((MP)->mnt_wapbl_replay)
#define WAPBL_REPLAY_CAN_READ(MP, BLK, LEN)		\
    (*(MP)->mnt_wapbl_op->wo_wapbl_replay_can_read)((MP)->mnt_wapbl_replay, \
    (BLK), (LEN))
#define WAPBL_REPLAY_READ(MP, DATA, BLK, LEN)	\
    (*(MP)->mnt_wapbl_op->wo_wapbl_replay_read)((MP)->mnt_wapbl_replay,		\
    (DATA), (BLK), (LEN))
#define WAPBL_ADD_BUF(MP, BP)					\
    (*(MP)->mnt_wapbl_op->wo_wapbl_add_buf)((MP)->mnt_wapbl, (BP))
#define WAPBL_REMOVE_BUF(MP, BP)				\
    (*(MP)->mnt_wapbl_op->wo_wapbl_remove_buf)((MP)->mnt_wapbl, (BP))
#define WAPBL_RESIZE_BUF(MP, BP, OLDSZ, OLDCNT)	\
    (*(MP)->mnt_wapbl_op->wo_wapbl_resize_buf)((MP)->mnt_wapbl, (BP),		\
    (OLDSZ), (OLDCNT))
#define WAPBL_BEGIN(MP)							\
    (*(MP)->mnt_wapbl_op->wo_wapbl_begin)((MP)->mnt_wapbl,					\
    __FILE__, __LINE__)
#define WAPBL_END(MP)							\
    (*(MP)->mnt_wapbl_op->wo_wapbl_end)((MP)->mnt_wapbl)
#define WAPBL_JUNLOCK_ASSERT(MP)				\
    (*(MP)->mnt_wapbl_op->wo_wapbl_junlock_assert)((MP)->mnt_wapbl)
#define WAPBL_JLOCK_ASSERT(MP)					\
    (*(MP)->mnt_wapbl_op->wo_wapbl_jlock_assert)((MP)->mnt_wapbl)


#endif /* _SYS_WAPBL_OPS_H */
