/*-
 * Copyright (c) 1999, 2000 Robert N. M. Watson
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sys/acl.h,v 1.8 2000/01/28 15:22:51 rwatson Exp $
 */

/*
 * Userland/kernel interface for Access Control Lists.
 *
 * The POSIX.1e implementation page may be reached at:
 * http://www.watson.org/fbsd-hardening/posix1e/
 */

#ifndef _SYS_ACL_H_
#define	_SYS_ACL_H_

#include <sys/param.h>
#include <sys/queue.h>

/*
 * POSIX.1e and NFSv4 ACL types and related constants.
 */

typedef uint32_t	acl_tag_t;
typedef uint32_t	acl_perm_t;
typedef uint16_t	acl_flag_t;
typedef int		    acl_type_t;

#define	ACL_MAX_ENTRIES		254

struct acl_entry {
	acl_tag_t	ae_tag;
	uid_t		ae_id;
	acl_perm_t	ae_perm;
};
typedef struct acl_entry 	*acl_entry_t;

struct acl {
	int		acl_cnt;
	struct acl_entry	acl_entry[ACL_MAX_ENTRIES];
};
typedef struct acl 	*acl_t;

/*
 * cmd options for acl syscalls
 * - acl_file: set, get, delete, check
 * - acl_filedesc: set, get, delete, check
 */
enum acl_cmdops {
	GETACL,
	SETACL,
	DELACL,
	CHECKACL,
};

/*
 * Possible valid values for a_tag of acl_entry_t
 */
#define	ACL_USER_OBJ	0x00000001
#define	ACL_USER	0x00000002
#define	ACL_GROUP_OBJ	0x00000004
#define	ACL_GROUP	0x00000008
#define	ACL_MASK	0x00000010
#define	ACL_OTHER	0x00000020
#define	ACL_OTHER_OBJ	ACL_OTHER

/*
 * Possible valid values a_type_t arguments
 */
#define	ACL_TYPE_ACCESS		0x00000000
#define	ACL_TYPE_DEFAULT	0x00000001
#define	ACL_TYPE_AFS		0x00000002
#define	ACL_TYPE_CODA		0x00000003
#define	ACL_TYPE_NTFS		0x00000004

/*
 * Possible flags in a_perm field
 */
#define	ACL_PERM_EXEC	0x0001
#define	ACL_PERM_WRITE	0x0002
#define	ACL_PERM_READ	0x0004
#define	ACL_PERM_NONE	0x0000
#define	ACL_PERM_BITS	(ACL_PERM_EXEC | ACL_PERM_WRITE | ACL_PERM_READ)
#define	ACL_POSIX1E_BITS	(ACL_PERM_EXEC | ACL_PERM_WRITE | ACL_PERM_READ)

#ifndef _KERNEL
#if defined(_ACL_PRIVATE)
__BEGIN_DECLS
/* kernel acl syscall callback */
int acl_file(int, char *, acl_type_t, struct acl *);
int acl_filedesc(int, int, acl_type_t, struct acl *);

/* file */
int acl_get_file(char *, acl_type_t, struct acl *);
int acl_set_file(char *, acl_type_t, struct acl *);
int acl_delete_file(char *, acl_type_t);
int acl_aclcheck_file(char *, acl_type_t, struct acl *);

/* file descriptor */
int acl_get_fd(int, acl_type_t, struct acl *);
int acl_set_fd(int, acl_type_t, struct acl *);
int acl_delete_fd(int, acl_type_t);
int acl_aclcheck_fd(int, acl_type_t, struct acl *);
__END_DECLS
#endif /* !_KERNEL */
#endif /* _ACL_PRIVATE */

#endif /* _SYS_ACL_H_ */
