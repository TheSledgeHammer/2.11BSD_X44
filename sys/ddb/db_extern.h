/*	$NetBSD: db_extern.h,v 1.2 1997/06/26 01:14:16 thorpej Exp $	*/

/*
 * Copyright (c) 1995 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */
#ifndef _DDB_DB_EXTERN_H_
#define _DDB_DB_EXTERN_H_

/* db_aout.c and db_elf.c */
void X_db_sym_init (void *, void *, char *);
db_sym_t X_db_lookup (db_symtab_t *, char *);
db_sym_t X_db_search_symbol (db_symtab_t *, db_addr_t, db_strategy_t, db_expr_t *);
void X_db_symbol_values (db_symtab_t *, db_sym_t, char **, db_expr_t *);
void db_printsym (db_expr_t, db_strategy_t);
boolean_t X_db_line_at_pc (db_symtab_t *, db_sym_t, char **, int *, db_expr_t);
int X_db_sym_numargs (db_symtab_t *, db_sym_t, int *, char **);
#ifdef DB_ELF_SYMBOLS	/* XXX */
void ddb_init (void *, void *);
#else
void ddb_init (void);
#endif

/* db_examine.c */
void db_examine_cmd (db_expr_t, int, db_expr_t, char *);
void db_examine (db_addr_t, char *, int);
void db_print_cmd (db_expr_t, int, db_expr_t, char *);
void db_print_loc_and_inst (db_addr_t);
void db_strcpy (char *, char *);
void db_search_cmd (db_expr_t, boolean_t, db_expr_t, char *);
void db_search (db_addr_t, int, db_expr_t, db_expr_t, unsigned int);

/* db_expr.c */
boolean_t db_term (db_expr_t *);
boolean_t db_unary (db_expr_t *);
boolean_t db_mult_expr (db_expr_t *);
boolean_t db_add_expr (db_expr_t *);
boolean_t db_shift_expr (db_expr_t *);
int db_expression (db_expr_t *);

/* db_input.c */
void db_putstring (char *, int);
void db_putnchars (int, int);
void db_delete (int, int);
int db_inputchar (int);
int db_readline (char *, int);
void db_check_interrupt (void);

/* db_print.c */
void db_show_regs (db_expr_t, boolean_t, db_expr_t, char *);

/* db_trap.c */
void db_trap(int, int);

/* db_write_cmd.c */
void db_write_cmd (db_expr_t, boolean_t, db_expr_t, char *);

#endif /* _DDB_DB_EXTERN_H_ */
