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

#include <sys/cdefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ctimed.h"

void
ctimed_cmds_set(char arg, int val)
{
    struct cmds *cmd = &cmd_table[val];
    cmd->val = val;
    cmd->arg = arg;
}

struct cmds *
ctimed_cmds_get(char arg, int val)
{
    struct cmds *cmd = &cmd_table[val];
    if (cmd != NULL) {
        if ((cmd->arg == arg) && (cmd->val == val)) {
            return (cmd);
        }
    }
    return (NULL);
}

void
ctimed_cmdops_set(struct cmds *cmd, struct cmd_ops *ops)
{
	struct cmd_ops *cmdops;
	struct cmds *isvalid;

	if (cmd != NULL) {
		isvalid = ctimed_cmds_get(cmd->arg, cmd->val);
		if (isvalid == NULL) {
			/* command operation invalid */
			return;
		}
	} else {
		/* command table is empty */
		return;
	}

	cmdops = &ops[cmd->val];
	if (cmdops != NULL) {
		cmd->ops = cmdops;
	}
}

struct cmd_ops *
ctimed_cmdops_get(char arg, int val)
{
    struct cmds *cmd = ctimed_cmds_get(arg, val);
    return (cmd->ops);
}

void
ctimed_cmds_setup(struct cmds *cmd, struct cmd_ops *ops)
{
    int i = 0;
    char ch = '0';

    for (i = 0; i < NCMDS; i++) {
        ch = cmd[i].arg;
        switch (ch) {
            case 'c':	/* CTIME */
                ctimed_cmds_set(CTIME_ARG, i);
                break;
            case 'a':	/* ASCTIME */
                ctimed_cmds_set(ASCTIME_ARG, i);
                break;
            case 't':	/* TZSET */
                ctimed_cmds_set(TZSET_ARG, i);
                break;
            case 'l':	/* LOCALTIME */
                ctimed_cmds_set(LOCALTIME_ARG, i);
                break;
            case 'g':	/* GMTIME */
                ctimed_cmds_set(GMTIME_ARG, i);
                break;
            case 'o':	/* OFFTIME */
                ctimed_cmds_set(OFFTIME_ARG, i);
                break;
            case 'e':	/* GETPWENT */
                ctimed_cmds_set(GETPWENT_ARG, i);
                break;
            case 'n':	/* GETPWNAM */
                ctimed_cmds_set(GETPWNAM_ARG, i);
                break;
            case 'u':	/* GETPWUID */
                ctimed_cmds_set(GETPWUID_ARG, i);
                break;
            case 'p':	/* SETPASSENT */
                ctimed_cmds_set(SETPASSENT_ARG, i);
                break;
            case 'f':	/* ENDPWENT */
                ctimed_cmds_set(ENDPWENT_ARG, i);
                break;
            default:
                break;
        }
    }
    ctimed_cmdops_set(cmd, ops);
}

char *
cmdop_ctime(struct cmd_ops *ops, const time_t *t)
{
	return ((*ops->cmd_ctime)(t));
}

char *
cmdop_asctime(struct cmd_ops *ops, const struct tm *tp)
{
	return ((*ops->cmd_asctime)(tp));
}

void
cmdop_tzset(struct cmd_ops *ops)
{
	(*ops->cmd_tzset)();
}

struct tm *
cmdop_localtime(struct cmd_ops *ops, const time_t *tp)
{
	return ((*ops->cmd_localtime)(tp));
}

struct tm *
cmdop_gmtime(struct cmd_ops *ops, const time_t *tp)
{
	return ((*ops->cmd_gmtime)(tp));
}

struct tm *
cmdop_offtime(struct cmd_ops *ops, const time_t *clock, long offset)
{
	return ((*ops->cmd_offtime)(clock, offset));
}

struct passwd *
cmdop_getpwent(struct cmd_ops *ops)
{
	return ((*ops->cmd_getpwent)());
}

struct passwd *
cmdop_getpwnam(struct cmd_ops *ops, const char *nam)
{
	return ((*ops->cmd_getpwnam)(nam));
}

struct passwd *
cmdop_getpwuid(struct cmd_ops *ops, uid_t uid)
{
	return ((*ops->cmd_getpwuid)(uid));
}

void
cmdop_setpwent(struct cmd_ops *ops)
{
	(*ops->cmd_setpwent)();
}

int
cmdop_setpassent(struct cmd_ops *ops, int stayopen)
{
	return ((*ops->cmd_setpassent)(stayopen));
}

void
cmdop_endpwent(struct cmd_ops *ops)
{
	(*ops->cmd_endpwent)();
}
