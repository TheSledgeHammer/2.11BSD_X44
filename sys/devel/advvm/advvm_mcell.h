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
 *
 *	@(#)advvm_mcell.h 1.0 	22/05/23
 */

#ifndef _ADVVM_MCELL_H_
#define _ADVVM_MCELL_H_

#include <sys/queue.h>
#include <sys/types.h>

#include <devel/advvm/advvm_fileset.h>

/*
 * mcell functions:
 * used to location advvm file directory from fileset id and tag
 */
struct advcell_list;
LIST_HEAD(advcell_list, advvm_cell);
struct advvm_cell {
	advvm_tag_dir_t 		*ac_tag;
	advvm_file_dir_t		*ac_fdir;
	LIST_ENTRY(advvm_cell)	ac_next;
};
typedef struct advvm_cell advvm_cell_t;

void 				advvm_mcell_init(advvm_fileset_t *);
void				advvm_mcell_add(advvm_fileset_t *);
advvm_cell_t 		*advvm_mcell_find(advvm_fileset_t *);
void				advvm_mcell_remove(advvm_fileset_t *);
advvm_file_dir_t 	*advvm_mcell_get_fdir(advvm_fileset_t *);

#endif /* _ADVVM_MCELL_H_ */
