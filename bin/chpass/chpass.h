/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)chpass.h	5.1 (Berkeley) 2/22/89
 */

struct entry {
	char *prompt;
	int  (*func)(char *, struct passwd *, struct entry *);
	int  restricted;
	int  len;
	char *except;
	char *save;
};

extern uid_t uid;

/* field.c */
int p_login(char *, struct passwd *, struct entry *);
int p_passwd(char *, struct passwd *, struct entry *);
int p_uid(char *, struct passwd *, struct entry *);
int p_gid(char *, struct passwd *, struct entry *);
int p_class(char *, struct passwd *, struct entry *);
int p_change(char *, struct passwd *, struct entry *);
int p_expire(char *, struct passwd *, struct entry *);
int p_gecos(char *, struct passwd *, struct entry *);
int p_hdir(char *, struct passwd *, struct entry *);
int p_shell(char *, struct passwd *, struct entry *);

/* util.c */
char *ttoa(time_t);
int  atot(char *, time_t *);
void print(FILE *, struct passwd *);
