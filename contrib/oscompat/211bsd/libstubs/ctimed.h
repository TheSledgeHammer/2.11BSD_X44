/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
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
 */

#ifndef _LIBSTUBS_CTIMED_H_
#define _LIBSTUBS_CTIMED_H_

/* ctimed command operations */
struct cmd_ops {
	char *(*cmd_ctime)(const time_t *);
	char *(*cmd_asctime)(const struct tm *);
	void (*cmd_tzset)(void);
	struct tm *(*cmd_localtime)(const time_t *);
	struct tm *(*cmd_gmtime)(const time_t *);
	struct tm *(*cmd_offtime)(const time_t *, long);
	struct passwd *(*cmd_getpwent)(void);
	struct passwd *(*cmd_getpwnam)(const char *);
	struct passwd *(*cmd_getpwuid)(uid_t);
	void (*cmd_setpwent)(void);
	int (*cmd_setpassent)(int);
	void (*cmd_endpwent)(void);
};

/* ctimed command table */
struct cmds {
    char arg;
    int	val;
	struct cmd_ops *ops;
};

/*
 * These should probably be placed in an include file.  If you add anything
 * here then you will also have to modify /usr/src/usr.lib/libstubs/stubs.c
 * (if for no other reason than to add the stub code).
*/

/* file send and recieve */
#define	SEND_FD	W[1]
#define	RECV_FD	R[0]

/* command arguments */
#define	CTIME_ARG			'c'
#define	ASCTIME_ARG			'a'
#define	TZSET_ARG			't'
#define	LOCALTIME_ARG 		'l'
#define	GMTIME_ARG			'g'
#define	OFFTIME_ARG			'o'

#define GETPWENT_ARG       	'e'
#define GETPWNAM_ARG        'n'
#define GETPWUID_ARG        'u'
#define SETPASSENT_ARG      'p'
#define ENDPWENT_ARG        'f'

/* command values */
#define	CTIME_VAL			1
#define	ASCTIME_VAL			2
#define	TZSET_VAL			3
#define	LOCALTIME_VAL 		4
#define	GMTIME_VAL			5
#define	OFFTIME_VAL			6
#define GETPWENT_VAL       	7
#define GETPWNAM_VAL        8
#define GETPWUID_VAL        9
#define SETPASSENT_VAL      10
#define ENDPWENT_VAL        11

#define NCMDS				11

extern struct cmds cmd_table[NCMDS];
extern struct cmd_ops ctimed_ops;
extern struct cmd_ops stub_ops;
extern struct passwd _pw;

void ctimed_cmds_setup(struct cmds *, struct cmd_ops *);
struct cmds	*ctimed_cmds_get(char, int);
void ctimed_cmds_set(char, int);
struct cmd_ops *ctimed_cmdops_get(char, int);
void ctimed_cmdops_set(struct cmds *, struct cmd_ops *);

/* cmd ops */
char *cmdop_ctime(struct cmd_ops *, const time_t *);
char *cmdop_asctime(struct cmd_ops *, const struct tm *);
void cmdop_tzset(struct cmd_ops *);
struct tm *cmdop_localtime(struct cmd_ops *, const time_t *);
struct tm *cmdop_gmtime(struct cmd_ops *, const time_t *);
struct tm *cmdop_offtime(struct cmd_ops *, const time_t *, long);
struct passwd *cmdop_getpwent(struct cmd_ops *);
struct passwd *cmdop_getpwnam(struct cmd_ops *, const char *);
struct passwd *cmdop_getpwuid(struct cmd_ops *, uid_t);
void cmdop_setpwent(struct cmd_ops *);
int cmdop_setpassent(struct cmd_ops *, int);
void cmdop_endpwent(struct cmd_ops *);

#endif /* _LIBSTUBS_CTIMED_H_ */
