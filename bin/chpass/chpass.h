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
	const char *prompt;
	int  (*func)(const char *, struct passwd *, struct entry *);
	int  restricted;
	int  len;
	const char *except;
	const char *save;
};

extern uid_t uid;

/* field.c */
int p_login(const char *, struct passwd *, struct entry *);
int p_passwd(const char *, struct passwd *, struct entry *);
int p_uid(const char *, struct passwd *, struct entry *);
int p_gid(const char *, struct passwd *, struct entry *);
int p_class(const char *, struct passwd *, struct entry *);
int p_change(const char *, struct passwd *, struct entry *);
int p_expire(const char *, struct passwd *, struct entry *);
int p_gecos(const char *, struct passwd *, struct entry *);
int p_hdir(const char *, struct passwd *, struct entry *);
int p_shell(const char *, struct passwd *, struct entry *);

/* util.c */
char *ttoa(time_t);
int  atot(char *, time_t *);
void print(FILE *, struct passwd *);
