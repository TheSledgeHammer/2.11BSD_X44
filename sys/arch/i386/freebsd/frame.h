/* to go in frame.h when ready */

/* Superset of trap frame, for traps from virtual-8086 mode */

struct trapframe_vm86 {
	int	tf_fs;
	int	tf_es;
	int	tf_ds;
	int	tf_edi;
	int	tf_esi;
	int	tf_ebp;
	int	tf_isp;
	int	tf_ebx;
	int	tf_edx;
	int	tf_ecx;
	int	tf_eax;
	int	tf_trapno;
	/* below portion defined in 386 hardware */
	int	tf_err;
	int	tf_eip;
	int	tf_cs;
	int	tf_eflags;
	/* below only when crossing rings (user (including vm86) to kernel) */
	int	tf_esp;
	int	tf_ss;
	/* below only when crossing from vm86 mode to kernel */
	int	tf_vm86_es;
	int	tf_vm86_ds;
	int	tf_vm86_fs;
	int	tf_vm86_gs;
};
