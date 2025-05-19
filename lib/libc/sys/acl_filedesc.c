/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/acl.h>

#include <unistd.h>

int
acl_filedesc(cmd, filedes, type, aclp)
	int cmd;
	int filedes;
	acl_type_t type;
	struct acl *aclp;
{
	return(__syscall((quad_t)SYS_acl_filedesc, cmd, filedes, type, aclp));
}

int
acl_get_fd(filedes, type, aclp)
	int filedes;
	acl_type_t type;
	struct acl *aclp;
{
	return (acl_filedesc(GETACL, filedes, type, aclp));
}

int
acl_set_fd(filedes, type, aclp)
	int filedes;
	acl_type_t type;
	struct acl *aclp;
{
	return (acl_filedesc(SETACL, filedes, type, aclp));
}

int
acl_delete_fd(filedes, type)
	int filedes;
	acl_type_t type;
{
	return (acl_filedesc(DELACL, filedes, type, NULL));
}

int
acl_aclcheck_fd(filedes, type, aclp)
	int filedes;
	acl_type_t type;
	struct acl *aclp;
{
	return (acl_filedesc(CHECKACL, filedes, type, aclp));
}
