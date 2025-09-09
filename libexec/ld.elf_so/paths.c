/*	$NetBSD: paths.c,v 1.39 2008/06/05 00:03:20 ad Exp $	 */

/*
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * Copyright 2002 Charles M. Hannum <root@ihack.net>
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: paths.c,v 1.39 2008/06/05 00:03:20 ad Exp $");
#endif /* not lint */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/gmon.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/resource.h>
#include <machine/cpu.h>

#include "debug.h"
#include "rtld.h"

static Search_Path *_rtld_find_path(Search_Path *, const char *, size_t);
static Search_Path **_rtld_append_path(Search_Path **, Search_Path **,
    const char *, const char *, const char *);
static void _rtld_process_mapping(Library_Xform **, const char *,
    const char *);
static char *exstrdup(const char *, const char *);
static const char *getstr(const char **, const char *, const char *);
static const char *getcstr(const char **, const char *, const char *);
static const char *getword(const char **, const char *, const char *);
static int matchstr(const char *, const char *, const char *);

static const char WS[] = " \t\n";

/*
 * Like xstrdup(), but takes end of string as a argument.
 */
static char *
exstrdup(const char *bp, const char *ep)
{
	char *cp;
	size_t len = ep - bp;

	cp = xmalloc(len + 1);
	memcpy(cp, bp, len);
	cp[len] = '\0';
	return (cp);
}

/*
 * Like strsep(), but takes end of string and doesn't put any NUL.  To
 * detect empty string, compare `*p' and return value.
 */
static const char *
getstr(const char **p, const char *ep, const char *delim)
{
	const char *cp = *p, *q, *r;

	if (ep < cp)
		/* End of string */
		return (NULL);

	for (q = cp; q < ep; q++)
		for (r = delim; *r != 0; r++)
			if (*r == *q)
				goto done;

done:
	*p = q;
	return (cp);
}

/*
 * Like getstr() above, but delim[] is complemented.
 */
static const char *
getcstr(const char **p, const char *ep, const char *delim)
{
	const char *cp = *p, *q, *r;

	if (ep < cp)
		/* End of string */
		return (NULL);

	for (q = cp; q < ep; q++)
		for (r = delim; *r != *q; r++)
			if (*r == 0)
				goto done;

done:
	*p = q;
	return (cp);
}

static const char *
getword(const char **p, const char *ep, const char *delim)
{

	(void)getcstr(p, ep, delim);

	/*
	 * Now, we're looking non-delim, or end of string.
	 */

	return (getstr(p, ep, delim));
}

/*
 * Match `bp' against NUL terminated string pointed by `p'.
 */
static int
matchstr(const char *p, const char *bp, const char *ep)
{
	int c;

	while (bp < ep)
		if ((c = *p++) == 0 || c != *bp++)
			return (0);

	return (*p == 0);
}

static Search_Path *
_rtld_find_path(Search_Path *path, const char *pathstr, size_t pathlen)
{

	for (; path != NULL; path = path->sp_next) {
		if (pathlen == path->sp_pathlen &&
		    memcmp(path->sp_path, pathstr, pathlen) == 0)
			return path;
	}
	return NULL;
}

static Search_Path **
_rtld_append_path(Search_Path **head_p, Search_Path **path_p,
    const char *execname, const char *bp, const char *ep)
{
	Search_Path *path;
	char epath[MAXPATHLEN];
	size_t len;

	len = _rtld_expand_path(epath, sizeof(epath), execname, bp, ep);
	if (len == 0)
		return path_p;

	if (_rtld_find_path(*head_p, bp, ep - bp) != NULL)
		return path_p;

	path = NEW(Search_Path);
	path->sp_pathlen = len;
	path->sp_path = exstrdup(epath, epath + len);
	path->sp_next = (*path_p);
	(*path_p) = path;
	path_p = &path->sp_next;

	dbg((" added path \"%s\"", path->sp_path));
	return path_p;
}

void
_rtld_add_paths(const char *execname, Search_Path **path_p, const char *pathstr)
{
	Search_Path **head_p = path_p;

	if (pathstr == NULL)
		return;

	if (pathstr[0] == ':') {
		/*
		 * Leading colon means append to current path
		 */
		while ((*path_p) != NULL)
			path_p = &(*path_p)->sp_next;
		pathstr++;
	}

	for (;;) {
		const char *bp = pathstr;
		const char *ep = strchr(bp, ':');
		if (ep == NULL)
			ep = &pathstr[strlen(pathstr)];

		path_p = _rtld_append_path(head_p, path_p, execname, bp, ep);

		if (ep[0] == '\0')
			break;
		pathstr = ep + 1;
	}
}

/*
 * Process library mappings of the form:
 *	<library_name>	<machdep_variable> <value,...:library_name,...> ... 
 */
static void
_rtld_process_mapping(Library_Xform **lib_p, const char *bp, const char *ep)
{
	Library_Xform *hwptr = NULL;
	const char *ptr, *key, *ekey, *lib, *elib, *l;
	int i, j;
	
	dbg((" processing mapping \"%.*s\"", (int)(ep - bp), bp));

	if ((ptr = getword(&bp, ep, WS)) == NULL || ptr == bp)
		return;

	dbg((" library \"%.*s\"", (int)(bp - ptr), ptr));

	hwptr = xmalloc(sizeof(*hwptr));
	memset(hwptr, 0, sizeof(*hwptr));
	hwptr->name = exstrdup(ptr, bp);

	bp++;

	if ((ptr = getword(&bp, ep, WS)) == NULL || ptr == bp) {
		xwarnx("missing sysctl variable name");
		goto cleanup;
	}

	dbg((" sysctl \"%.*s\"", (int)(bp - ptr), ptr));

	hwptr->ctlname = exstrdup(ptr, bp);

	for (i = 0; bp++, (ptr = getword(&bp, ep, WS)) != NULL;) {
		dbg((" ptr = %.*s", (int)(bp - ptr), ptr));
		if (ptr == bp)
			continue;

		if (i == RTLD_MAX_ENTRY) {
no_more:
			xwarnx("maximum library entries exceeded `%s'",
			    hwptr->name);
			goto cleanup;
		}
		if ((key = getstr(&ptr, bp, ":")) == NULL) {
			xwarnx("missing sysctl variable value for `%s'",
			    hwptr->name);
			goto cleanup;
		}
		ekey = ptr++;
		if ((lib = getstr(&ptr, bp, ":")) == NULL) {
			xwarnx("missing sysctl library list for `%s'",
			    hwptr->name);
			goto cleanup;
		}
		elib = ptr;		/* No need to advance */
		for (j = 0; (l = getstr(&lib, elib, ",")) != NULL;
		    j++, lib++) {
			if (j == RTLD_MAX_LIBRARY) {
				xwarnx("maximum library entries exceeded `%s'",
				    hwptr->name);
				goto cleanup;
			}
			dbg((" library \"%.*s\"", (int)(lib - l), l));
			hwptr->entry[i].library[j] = exstrdup(l, lib);
		}
		if (j == 0) {
			xwarnx("No library map entries for `%s/%.*s'",
			    hwptr->name, (int)(bp - ptr), ptr);
			goto cleanup;
		}
		j = i;
		for (; (l = getstr(&key, ekey, ",")) != NULL; i++, key++) {
			/*
			 * Allow empty key (it is valid as string
			 * value).  Thus, we loop at least once and
			 * `i' is incremented.
			 */

			dbg((" key \"%.*s\"", (int)(key - l), l));
			if (i == RTLD_MAX_ENTRY)
				goto no_more;
			if (i != j)
				(void)memcpy(hwptr->entry[i].library, 
				    hwptr->entry[j].library,
				    sizeof(hwptr->entry[j].library));
			hwptr->entry[i].value = exstrdup(l, key);
		}
	}

	if (i == 0) {
		xwarnx("No library entries for `%s'", hwptr->name);
		goto cleanup;
	}

	hwptr->next = *lib_p;
	*lib_p = hwptr;

	return;

cleanup:
	if (hwptr->name)
		xfree(hwptr->name);
	xfree(hwptr);
}

void
_rtld_process_hints(const char *execname, Search_Path **path_p,
    Library_Xform **lib_p, const char *fname)
{
	int fd;
	char *buf, small[128];
	const char *b, *ep, *ptr;
	struct stat st;
	ssize_t sz;
	Search_Path **head_p = path_p;

	if ((fd = open(fname, O_RDONLY)) == -1) {
		/* Don't complain */
		return;
	}

	/* Try to avoid mmap/stat on the file. */
	buf = small;
	buf[0] = '\0';
	sz = read(fd, buf, sizeof(small));
	if (sz == -1) {
		xwarn("read: %s", fname);
		(void)close(fd);
		return;
	}
	if (sz >= (ssize_t)sizeof(small)) {
		if (fstat(fd, &st) == -1) {
			/* Complain */
			xwarn("fstat: %s", fname);
			(void)close(fd);
			return;
		}

		sz = (ssize_t) st.st_size;

		buf = mmap(0, sz, PROT_READ, MAP_SHARED|MAP_FILE, fd, 0);
		if (buf == MAP_FAILED) {
			xwarn("mmap: %s", fname);
			(void)close(fd);
			return;
		}
	}
	(void)close(fd);

	while ((*path_p) != NULL)
		path_p = &(*path_p)->sp_next;

	for (b = buf, ep = buf + sz; b < ep; b++) {
		(void)getcstr(&b, ep, WS);
		if (b == ep)
			break;

		ptr = getstr(&b, ep, "\n#");
		if (*ptr == '/') {
			/*
			 * Since '/' != '\n' and != '#', we know ptr <
			 * b.  And we will stop when b[-1] == '/'.
			 */
			while (b[-1] == ' ' || b[-1] == '\t')
				b--;
			path_p = _rtld_append_path(head_p, path_p, execname,
			    ptr, b);
		} else
			_rtld_process_mapping(lib_p, ptr, b);

		/*
		 * b points one of ' ', \t, \n, # or equal to ep.  So,
		 * make sure we are at newline or end of string.
		 */
		(void)getstr(&b, ep, "\n");
	}

	if (buf != small)
		(void)munmap(buf, sz);
}

struct sysctl_list {
	struct ctlname *ctl;
	int numentries;
};

#ifdef CTL_MACHDEP_NAMES
static struct ctlname ctl_machdep[] = CTL_MACHDEP_NAMES;
#endif
static struct ctlname ctl_toplevel[] = CTL_NAMES;
static struct ctlname ctl_kern[] = CTL_KERN_NAMES;
static struct ctlname ctl_vm[] = CTL_VM_NAMES;
#ifdef	CTL_VFS_NAMES
static struct ctlname ctl_vfs[] = CTL_VFS_NAMES;
#endif
#ifdef	CTL_NET_NAMES
static struct ctlname ctl_net[] = CTL_NET_NAMES;
#endif
static struct ctlname ctl_debug[CTL_DEBUG_MAXID];
static struct ctlname ctl_hw[] = CTL_HW_NAMES;
static struct ctlname ctl_user[] = CTL_USER_NAMES;
static struct ctlname ctl_ddb[] = CTL_DDB_NAMES;

const struct sysctl_list toplevel[] = {
		{ 0, 0 },
		{ ctl_toplevel, CTL_MAXID },
		{ 0, -1 },
};

const struct sysctl_list secondlevel[] = {
		{ 0, 0 },						/* CTL_UNSPEC */
		{ ctl_kern, KERN_MAXID },		/* CTL_KERN */
		{ ctl_vm, VM_MAXID },			/* CTL_VM */
		{ 0, 0 },						/* CTL_VFS */
#ifdef	CTL_NET_NAMES
		{ ctl_net, NET_MAXID },			/* CTL_NET */
#else
		{ 0, 0 },						/* CTL_NET */
#endif
		{ ctl_debug, CTL_DEBUG_MAXID },	/* CTL_DEBUG */
		{ ctl_hw, HW_MAXID },			/* CTL_HW */
#ifdef CTL_MACHDEP_NAMES
		{ ctl_machdep, CPU_MAXID },		/* CTL_MACHDEP */
#else
		{ 0, 0 },						/* CTL_MACHDEP */
#endif
		{ ctl_user, USER_MAXID },		/* CTL_USER_NAMES */
		{ ctl_ddb, DDBCTL_MAXID },		/* CTL_DDB_NAMES */
		{ 0, 2 },						/* dummy name */
		{ 0, -1 },
};

#define CTL_MACHDEP_SIZE 	(sizeof(ctl_machdep) / sizeof(ctl_machdep[0]))
#define CTL_TOPLEVEL_SIZE 	(sizeof(toplevel)/sizeof(toplevel[0]))

/* Basic name -> sysctl MIB translation */
int
_rtld_sysctl(const char *name, void *oldp, size_t *oldlen)
{
	const char *node, *ep;
	int mib[CTL_MAXNAME], i, j, r;
	u_int miblen = 0;

	ep = name + strlen(name);
	node = getstr(&name, ep, ".");
	if (node == NULL) {
		return (-1);
	}

	for (i = 0; i < CTL_TOPLEVEL_SIZE; i++) {
		if (toplevel[i].numentries > 0) {
			for (j = 0; j < toplevel[i].numentries; j++) {
				if (secondlevel[j].numentries > 0) {
					if (toplevel[i].ctl == NULL) {
						continue;
					}
					if (matchstr(toplevel[i].ctl[j].ctl_name, node, name)) {
						break;
					}
					mib[j] = j;
					miblen++;
				} else {
					goto bad;
				}
			}
		} else {
			goto bad;
		}
	}
	if (miblen > CTL_MAXNAME) {
		return (-1);
	}
	r = sysctl(mib, miblen + 1, oldp, oldlen, NULL, 0);
	if (r == -1) {
		return (-1);
	}
	return (r);

bad:
	return (-1);
}
