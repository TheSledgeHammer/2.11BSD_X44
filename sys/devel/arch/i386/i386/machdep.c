/*
 * machdep.c
 *
 *  Created on: 2 Mar 2021
 *      Author: marti
 */

#include <devel/arch/i386/include/mcontext.h>
/*
 * Get machine context.
 */
int
get_mcontext(struct proc *p, struct mcontext *mcp, int flags)
{
	struct trapframe *tp;
	struct segment_descriptor *sdp;

	tp = p->p_md.md_regs;

	mcp->mc_onstack = sigonstack(tp->tf_esp);

	mcp->mc_gs = p->td_pcb->pcb_gs;
	mcp->mc_fs = tp->tf_fs;
	mcp->mc_es = tp->tf_es;
	mcp->mc_ds = tp->tf_ds;
	mcp->mc_edi = tp->tf_edi;
	mcp->mc_esi = tp->tf_esi;
	mcp->mc_ebp = tp->tf_ebp;
	mcp->mc_isp = tp->tf_isp;
	mcp->mc_eflags = tp->tf_eflags;
	if (flags & GET_MC_CLEAR_RET) {
		mcp->mc_eax = 0;
		mcp->mc_edx = 0;
		mcp->mc_eflags &= ~PSL_C;
	} else {
		mcp->mc_eax = tp->tf_eax;
		mcp->mc_edx = tp->tf_edx;
	}
	mcp->mc_ebx = tp->tf_ebx;
	mcp->mc_ecx = tp->tf_ecx;
	mcp->mc_eip = tp->tf_eip;
	mcp->mc_cs = tp->tf_cs;
	mcp->mc_esp = tp->tf_esp;
	mcp->mc_ss = tp->tf_ss;
	mcp->mc_len = sizeof(*mcp);

	get_fpcontext(td, mcp, NULL, 0);
	sdp = &td->td_pcb->pcb_fsd;
	mcp->mc_fsbase = sdp->sd_hibase << 24 | sdp->sd_lobase;
	sdp = &td->td_pcb->pcb_gsd;
	mcp->mc_gsbase = sdp->sd_hibase << 24 | sdp->sd_lobase;
	mcp->mc_flags = 0;
	mcp->mc_xfpustate = 0;
	mcp->mc_xfpustate_len = 0;
	bzero(mcp->mc_spare2, sizeof(mcp->mc_spare2));
	return (0);
}

/*
 * Set machine context.
 *
 * However, we don't set any but the user modifiable flags, and we won't
 * touch the cs selector.
 */
int
set_mcontext(struct proc *p, mcontext_t *mcp)
{
	return (0);
}
