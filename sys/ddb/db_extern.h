/*	$NetBSD: db_extern.h,v 1.6 1998/12/04 20:18:05 thorpej Exp $	*/

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _DDB_DB_EXTERN_H_
#define _DDB_DB_EXTERN_H_

/* db_sym.c */
void ddb_init(int, void *, void *);

/* db_examine.c */
void db_examine_cmd(db_expr_t, int, db_expr_t, char *);
void db_examine(db_addr_t, char *, int);
void db_print_cmd(db_expr_t, int, db_expr_t, char *);
void db_print_loc_and_inst(db_addr_t);
void db_strcpy(char *, char *);
void db_search_cmd(db_expr_t, boolean_t, db_expr_t, char *);
void db_search(db_addr_t, int, db_expr_t, db_expr_t, unsigned int);

/* db_expr.c */
boolean_t db_term(db_expr_t *);
boolean_t db_unary(db_expr_t *);
boolean_t db_mult_expr(db_expr_t *);
boolean_t db_add_expr(db_expr_t *);
boolean_t db_shift_expr(db_expr_t *);
int db_expression(db_expr_t *);

/* db_input.c */
void db_putstring(char *, int);
void db_putnchars(int, int);
void db_delete(int, int);
void db_delete_line(void);
int db_inputchar(int);
int db_readline(char *, int);
void db_check_interrupt(void);

/* db_print.c */
void db_show_regs(db_expr_t, boolean_t, db_expr_t, char *);

/* db_trap.c */
void db_trap(int, int);

/* db_write_cmd.c */
void db_write_cmd(db_expr_t, boolean_t, db_expr_t, char *);

#endif /* _DDB_DB_EXTERN_H_ */
