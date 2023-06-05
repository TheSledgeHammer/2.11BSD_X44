/* $NetBSD: dm_ioctl.c,v 1.56 2022/10/13 06:10:48 andvar Exp $      */

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hamsik.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dm_ioctl.c,v 1.56 2022/10/13 06:10:48 andvar Exp $");

/*
 * Locking is used to synchronise between ioctl calls and between dm_table's
 * users.
 *
 * ioctl locking:
 * Simple reference counting, to count users of device will be used routines
 * dm_dev_busy/dm_dev_unbusy are used for that.
 * dm_dev_lookup/dm_dev_rem call dm_dev_busy before return(caller is therefore
 * holder of reference_counter last).
 *
 * ioctl routines which change/remove dm_dev parameters must wait on
 * dm_dev::dev_cv and when last user will call dm_dev_unbusy they will wake
 * up them.
 *
 * table_head locking:
 * To access table entries dm_table_* routines must be used.
 *
 * dm_table_get_entry will increment table users reference
 * counter. It will return active or inactive table depends
 * on uint8_t argument.
 *
 * dm_table_release must be called for every table_entry from
 * dm_table_get_entry. Between these two calls tables can't be switched
 * or destroyed.
 *
 * dm_table_head_init initialize table_entries SLISTS and io_cv.
 *
 * dm_table_head_destroy destroy cv.
 *
 * There are two types of users for dm_table_head first type will
 * only read list and try to do anything with it e.g. dmstrategy,
 * dm_table_size etc. There is another user for table_head which wants
 * to change table lists e.g. dm_dev_resume_ioctl, dm_dev_remove_ioctl,
 * dm_table_clear_ioctl.
 *
 * NOTE: It is not allowed to call dm_table_destroy, dm_table_switch_tables
 *       with hold table reference counter. Table reference counter is hold
 *       after calling dm_table_get_entry routine. After calling this
 *       function user must call dm_table_release before any writer table
 *       operation.
 *
 * Example: dm_table_get_entry
 *          dm_table_destroy/dm_table_switch_tables
 * This example will lead to deadlock situation because after dm_table_get_entry
 * table reference counter is != 0 and dm_table_destroy have to wait on cv until
 * reference counter is 0.
 *
 */
