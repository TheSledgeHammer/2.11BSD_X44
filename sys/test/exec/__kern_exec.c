/*
 * __kern_exec.c
 *
 *  Created on: 31 Jan 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <exec/__exec.h>
#include <exec/__exec_linker.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/signalvar.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <vm/include/vm_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>

extern const struct execsw	execsw_builtin[];
extern int			nexecs_builtin;
static const struct execsw	**execsw = NULL;
static int			nexecs;

static void link_ex(struct execsw_entry **listp, const struct execsw *exp);

int
check_exec(struct proc *p, struct exec_linker *elp)
{
	int					error, i;
	struct vnode		*vp;
	struct nameidata 	*ndp;
	size_t				resid;

	ndp = elp->el_ndp;
	ndp->ni_cnd.cn_nameiop = LOOKUP;
	ndp->ni_cnd.cn_flags = FOLLOW | LOCKLEAF | SAVENAME;
	/* first get the vnode */
	if ((error = namei(ndp)) != 0)
		return error;
	elp->el_vnodep = vp = ndp->ni_vp;

	/* check access and type */
	if (vp->v_type != VREG) {
		error = EACCES;
		goto bad1;
	}
	if ((error = VOP_ACCESS(vp, VEXEC, p->p_cred, p)) != 0)
		goto bad1;

	/* get attributes */
	if ((error = VOP_GETATTR(vp, elp->el_vnodep, p->p_cred, p)) != 0)
		goto bad1;

	/* Check mount point */
	if (vp->v_mount->mnt_flag & MNT_NOEXEC) {
		error = EACCES;
		goto bad1;
	}
	if (vp->v_mount->mnt_flag & MNT_NOSUID)
		elp->el_attr->va_mode &= ~(S_ISUID | S_ISGID);

	/* try to open it */
	if ((error = VOP_OPEN(vp, FREAD, p->p_cred, p)) != 0)
		goto bad1;

	/* unlock vp, since we need it unlocked from here on out. */
	VOP_UNLOCK(vp, 0);

	error = vn_rdwr(UIO_READ, vp, elp->el_image_hdr, elp->el_hdrlen, 0,
				UIO_SYSSPACE, 0, p->p_cred, &resid, NULL);

	if (error)
		goto bad2;
	elp->el_hdrvalid = elp->el_hdrlen - resid;

	/*
	 * Set up default address space limits.  Can be overridden
	 * by individual exec packages.
	 *
	 * XXX probably should be all done in the exec pakages.
	 */
	elp->el_vm_minaddr = VM_MIN_ADDRESS;
	elp->el_vm_maxaddr = VM_MAXUSER_ADDRESS;
	/*
	 * set up the vmcmds for creation of the process
	 * address space
	 */
	error = ENOEXEC;
	for (i = 0; i < nexecs && error != 0; i++) {
		int newerror;

		elp->el_esch = execsw[i];
		newerror = (*execsw[i]->ex_makecmds)(p, elp);
		/* make sure the first "interesting" error code is saved. */
		if (!newerror || error == ENOEXEC)
			error = newerror;

		/* if es_makecmds call was successful, update epp->ep_es */
		if (!newerror && (elp->el_flags & EXEC_HASES) == 0)
			elp->el_ex = execsw[i];

		if ((elp->el_flags & EXEC_DESTR) && error != 0)
			return error;
	}
	if (!error) {
		/* check that entry point is sane */
		if (elp->el_entry > VM_MAXUSER_ADDRESS)
			error = ENOEXEC;

		/* check limits */
		if ((elp->el_tsize > MAXTSIZ) ||
				(elp->el_dsize > (u_quad_t)p->p_rlimit[RLIMIT_DATA].rlim_cur))
			error = ENOMEM;

		if (!error)
			return (0);
	}
	/*
	 * free any vmspace-creation commands,
	 * and release their references
	 */
	kill_vmcmds(&elp->el_vmcmds);

bad2:
	/*
	 * close and release the vnode, restore the old one, free the
	 * pathname buf, and punt.
	 */
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(vp, FREAD, p->p_cred, p);
	vput(vp);
	return error;

bad1:
	/*
	 * free the namei pathname buffer, and put the vnode
	 * (which we don't yet have open).
	 */
	vput(vp);				/* was still locked */
	return error;
}

/*
 * Add execsw[] entry.
 */
int
exec_add(struct proc *p, struct execsw *exp, const char *e_name)
{
	struct exec_entry	*it;
	int					error;

	error = 0;
	//lockmgr(&exec_lock, LK_EXCLUSIVE, NULL, p);

	LIST_FOREACH(it, &ex_head, ex_list) {
		if(it->ex->ex_makecmds == exp->ex_makecmds && it->ex->u.elf_probe_func == exp->u.elf_probe_func) {
			error = EEXIST;
			return error;
		}
	}

	/* if we got here, the entry doesn't exist yet */

	it = MALLOC(sizeof(struct exec_entry), M_EXEC, M_WAITOK);
	it->ex = exp;
	LIST_INSERT_HEAD(&ex_head, it, ex_list);

	exec_init(0);

//out:
	//lockmgr(&exec_lock, LK_RELEASE, NULL, p);
	return error;
}

/*
 * Remove execsw[] entry.
 */
int
exec_remove(const struct execsw *exp)
{
	struct exec_entry	*it;
	int					error;

	error = 0;
	//lockmgr(&exec_lock, LK_EXCLUSIVE, NULL, p);

	LIST_FOREACH(it, &ex_head, ex_list) {
		if(it->ex->ex_makecmds == exp->ex_makecmds && it->ex->u.elf_probe_func == exp->u.elf_probe_func) {
			break;
		}
	}
	if (!it) {
		error = ENOENT;
		return error;
	}

	/* remove item from list and free resources */
	LIST_REMOVE(it, ex_list);
	FREE(it, M_EXEC);

	/* update execsw[] */
	exec_init(0);

//out:
	//lockmgr(&exec_lock, LK_RELEASE, NULL);
	return error;
}

static void
link_ex(struct execsw_entry **listp, const struct execsw *exp)
{
	struct execsw_entry *et, *e1;

	et = (struct execsw_entry *) malloc(sizeof(struct execsw_entry), M_TEMP, M_WAITOK);
	et->next = NULL;
	et->ex = exp;
	if (*listp == NULL) {
		*listp = et;
		return;
	}
	switch(et->ex->ex_prio) {
	case EXECSW_PRIO_FIRST:
		/* put new entry as the first */
		et->next = *listp;
		*listp = et;
		break;
	case EXECSW_PRIO_ANY:
		/* put new entry after all *_FIRST and *_ANY entries */
		for(e1 = *listp; e1->next
			&& e1->next->ex->ex_prio != EXECSW_PRIO_LAST;
			e1 = e1->next);
		et->next = e1->next;
		e1->next = et;
		break;
	case EXECSW_PRIO_LAST:
		/* put new entry as the last one */
		for(e1 = *listp; e1->next; e1 = e1->next);
		e1->next = et;
		break;
	default:
#ifdef DIAGNOSTIC
		panic("execw[] entry with unknown priority %d found",
			et->ex->ex_prio);
#else
		free(et, M_TEMP);
#endif
		break;
	}
}

int
exec_init(int init_boot)
{
	const struct execsw	**new_ex, * const *old_ex;
	struct execsw_entry	*list, *e1;
	struct exec_entry	*e2;
	int					i, ex_sz;

	if (init_boot) {
#ifdef DIAGNOSTIC
		if (i == 0)
			panic("no emulations found in execsw_builtin[]");
#endif
	}
	/*
	 * Build execsw[] array from builtin entries and entries added
	 * at runtime.
	 */
	list = NULL;
	for(i=0; i < nexecs_builtin; i++)
		link_es(&list, &execsw_builtin[i]);

	/* Add dynamically loaded entries */
	ex_sz = nexecs_builtin;
	LIST_FOREACH(e2, &ex_head, ex_list) {
		link_es(&list, e2->ex);
		ex_sz++;
	}

	/*
	 * Now that we have sorted all execw entries, create new execsw[]
	 * and free no longer needed memory in the process.
	 */
	new_ex = malloc(ex_sz * sizeof(struct execsw *), M_EXEC, M_WAITOK);
	for(i=0; list; i++) {
		new_ex[i] = list->ex;
		e1 = list->next;
		free(list, M_TEMP);
		list = e1;
	}

	/*
	 * New execsw[] array built, now replace old execsw[] and free
	 * used memory.
	 */
	old_ex = execsw;
	execsw = new_ex;
	nexecs = ex_sz;
	if (old_ex)
		free(old_ex, M_EXEC);

	/*
	 * Figure out the maximum size of an exec header.
	 */
	exec_maxhdrsz = 0;
	for (i = 0; i < nexecs; i++) {
		if (execsw[i]->ex_hdrsz > exec_maxhdrsz)
			exec_maxhdrsz = execsw[i]->ex_hdrsz;
	}
	return 0;
}
