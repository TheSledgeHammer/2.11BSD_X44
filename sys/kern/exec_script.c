/*	$NetBSD: exec_script.c,v 1.52.8.1 2006/12/21 14:25:45 tron Exp $	*/

/*
 * Copyright (c) 1993, 1994, 1996 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/resourcevar.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_script.h>
#include <sys/exec_elf.h>

int
exec_script_linker(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	int error, hdrlinelen, shellnamelen, shellarglen;
	char *hdrstr = elp->el_image_hdr;
	char *cp, *shellname, *shellarg, *oldpnbuf;
	char **shellargp, **tmpsap;
	struct vnode *scriptvp;

	uid_t script_uid = (uid_t) -1;
	gid_t script_gid = NOGROUP;
	u_short script_sbits;

	/*
	 * if the magic isn't that of a shell script, or we've already
	 * done shell script processing for this exec, punt on it.
	 */
	if ((elp->el_flags & EXEC_INDIR) != 0 ||
			elp->el_hdrvalid < EXEC_SCRIPT_MAGICLEN ||
			strncmp(hdrstr, EXEC_SCRIPT_MAGIC, EXEC_SCRIPT_MAGICLEN))
		return (ENOEXEC);

	/*
	 * check that the shell spec is terminated by a newline,
	 * and that it isn't too large.  Don't modify the
	 * buffer unless we're ready to commit to handling it.
	 * (The latter requirement means that we have to check
	 * for both spaces and tabs later on.)
	 */
	hdrlinelen = min(elp->el_hdrvalid, SCRIPT_HDR_SIZE);
	for (cp = hdrstr + EXEC_SCRIPT_MAGICLEN; cp < hdrstr + hdrlinelen; cp++) {
		if (*cp == '\n') {
			*cp = '\0';
			break;
		}
	}
	if (cp >= hdrstr + hdrlinelen)
		return (ENOEXEC);

	/*
	 * If the script has an ELF header, don't exec it.
	 */
	if (elp->el_hdrvalid >= sizeof(ELFMAG)-1 && memcmp(hdrstr, ELFMAG, sizeof(ELFMAG)-1) == 0)
		return (ENOEXEC);

	shellname = NULL;
	shellarg = NULL;
	shellarglen = 0;

	/* strip spaces before the shell name */
	for (cp = hdrstr + EXEC_SCRIPT_MAGICLEN; *cp == ' ' || *cp == '\t'; cp++)
		;

	/* collect the shell name; remember it's length for later */
	shellname = cp;
	shellnamelen = 0;
	if (*cp == '\0')
		goto check_shell;
	for ( /* cp = cp */ ; *cp != '\0' && *cp != ' ' && *cp != '\t'; cp++)
		shellnamelen++;
	if (*cp == '\0')
		goto check_shell;
	*cp++ = '\0';

	/* skip spaces before any argument */
	for ( /* cp = cp */ ; *cp == ' ' || *cp == '\t'; cp++)
		;
	if (*cp == '\0')
		goto check_shell;

	/*
	 * collect the shell argument.  everything after the shell name
	 * is passed as ONE argument; that's the correct (historical)
	 * behaviour.
	 */
	shellarg = cp;
	for ( /* cp = cp */ ; *cp != '\0'; cp++)
		shellarglen++;
	*cp++ = '\0';

check_shell:
#ifdef SETUIDSCRIPTS
	/*
	 * MNT_NOSUID has already taken care of by check_exec,
	 * so we don't need to worry about it now or later.  We
	 * will need to check P_TRACED later, however.
	 */
	script_sbits = elp->el_attr->va_mode & (S_ISUID | S_ISGID);
	if (script_sbits != 0) {
		script_uid = elp->el_attr->va_uid;
		script_gid = elp->el_attr->va_gid;
	}
#endif
#ifdef FDSCRIPTS
	/*
	 * if the script isn't readable, or it's set-id, then we've
	 * gotta supply a "/dev/fd/..." for the shell to read.
	 * Note that stupid shells (csh) do the wrong thing, and
	 * close all open fd's when the start.  That kills this
	 * method of implementing "safe" set-id and x-only scripts.
	 */
	vn_lock(elp->el_vnodep, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_ACCESS(elp->el_vnodep, VREAD, p->p_cred, p);
	VOP_UNLOCK(elp->el_vnodep, 0, p);
	if (error == EACCES
#ifdef SETUIDSCRIPTS
	    || script_sbits
#endif
	    ) {
		struct file *fp;

#if defined(DIAGNOSTIC) && defined(FDSCRIPTS)
		if (elp->el_flags & EXEC_HASFD)
			panic("exec_script_makecmds: epp already has a fd");
#endif

		/* falloc() will use the descriptor for us */
		if ((error = falloc(p, &fp, &elp->el_fd)) != 0) {
			scriptvp = NULL;
			shellargp = NULL;
			goto fail;
		}

		elp->el_flags |= EXEC_HASFD;
		fp->f_type = DTYPE_VNODE;
		fp->f_ops = &vnops;
		fp->f_data = (caddr_t) elp->el_vnodep;
		fp->f_flag = FREAD;
		FILE_SET_MATURE(fp);
		FILE_UNUSE(fp, p);
	}
#endif

	/* set up the parameters for the recursive check_exec() call */
	elp->el_ndp.ni_dirp = shellname;
	elp->el_ndp.ni_segflg = UIO_SYSSPACE;
	elp->el_flags |= EXEC_INDIR;

	/* and set up the fake args list, for later */
	MALLOC(shellargp, char **, 4 * sizeof(char *), M_EXEC, M_WAITOK);
	tmpsap = shellargp;
	*tmpsap = malloc(shellnamelen + 1, M_EXEC, M_WAITOK);
	strlcpy(*tmpsap++, shellname, shellnamelen + 1);
	if (shellarg != NULL) {
		*tmpsap = malloc(shellarglen + 1, M_EXEC, M_WAITOK);
		strlcpy(*tmpsap++, shellarg, shellarglen + 1);
	}
	MALLOC(*tmpsap, char *, MAXPATHLEN, M_EXEC, M_WAITOK);
#ifdef FDSCRIPTS
	if ((elp->el_flags & EXEC_HASFD) == 0) {
#endif
		/* normally can't fail, but check for it if diagnostic */
#ifdef SYSTRACE
		error = 1;
		if (ISSET(p->p_flag, P_SYSTRACE)) {
			error = systrace_scriptname(p, *tmpsap);
			if (error == 0)
				tmpsap++;
		}
		if (error) {
			/*
			 * Since systrace_scriptname() provides a
			 * convenience, not a security issue, we are
			 * safe to do this.
			 */
			error = copystr(elp->el_name, *tmpsap++, MAXPATHLEN, NULL);
		}
#else
		error = copyinstr(elp->el_name, *tmpsap++, MAXPATHLEN, (size_t *)0);
#endif /* SYSTRACE */
#ifdef DIAGNOSTIC
		if (error != 0)
			panic("exec_script: copyinstr couldn't fail");
#endif
#ifdef FDSCRIPTS
	} else {
		snprintf(*tmpsap++, MAXPATHLEN, "/dev/fd/%d", elp->el_fd);
	}
#endif
	*tmpsap = NULL;

	/*
	 * mark the header we have as invalid; check_exec will read
	 * the header from the new executable
	 */
	elp->el_hdrvalid = 0;

	/*
	 * remember the old vp and pnbuf for later, so we can restore
	 * them if check_exec() fails.
	 */
	scriptvp = elp->el_vnodep;
	oldpnbuf = elp->el_ndp.ni_cnd.cn_pnbuf;

	if ((error = check_exec(elp)) == 0) {
		/* note that we've clobbered the header */
		elp->el_flags |= EXEC_DESTR|EXEC_HASES;

		/*
		 * It succeeded.  Unlock the script and
		 * close it if we aren't using it any more.
		 * Also, set things up so that the fake args
		 * list will be used.
		 */
		if ((elp->el_flags & EXEC_HASFD) == 0) {
			vn_lock(scriptvp, LK_EXCLUSIVE | LK_RETRY, p);
			VOP_CLOSE(scriptvp, FREAD, p->p_ucred, p);
			vput(scriptvp);
		}

		elp->el_flags |= (EXEC_HASARGL | EXEC_SKIPARG);
		elp->el_fa = shellargp;
#ifdef SETUIDSCRIPTS
		/*
		 * set thing up so that set-id scripts will be
		 * handled appropriately.  P_TRACED will be
		 * checked later when the shell is actually
		 * exec'd.
		 */
		elp->el_attr->va_mode |= script_sbits;
		if (script_sbits & S_ISUID)
			elp->el_attr->va_uid = script_uid;
		if (script_sbits & S_ISGID)
			elp->el_attr->va_gid = script_gid;
#endif
		return (0);
	}

	/* XXX oldpnbuf not set for "goto fail" path */
	elp->el_ndp.ni_cnd.cn_pnbuf = oldpnbuf;
#ifdef FDSCRIPTS
fail:
#endif
	/* note that we've clobbered the header */
	elp->el_flags |= EXEC_DESTR;

	/* kill the opened file descriptor, else close the file */
    if (elp->el_flags & EXEC_HASFD) {
    	elp->el_flags &= ~EXEC_HASFD;
        (void) fdrelease(elp->el_fd);
    } else if (scriptvp) {
    	vn_lock(scriptvp, LK_EXCLUSIVE | LK_RETRY, p);
        VOP_CLOSE(scriptvp, FREAD, p->p_ucred, p);
        vput(scriptvp);
    }

	/* free the fake arg list, because we're not returning it */
	if ((tmpsap = shellargp) != NULL) {
		while (*tmpsap != NULL) {
			FREE(*tmpsap, M_EXEC);
			tmpsap++;
		}
		FREE(shellargp, M_EXEC);
	}

    /*
     * free any vmspace-creation commands,
     * and release their references
     */
    kill_vmcmds(&elp->el_vmcmds);

	return (error);
}
