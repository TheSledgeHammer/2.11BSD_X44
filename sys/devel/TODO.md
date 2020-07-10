UFML:
	- Fix to match current vfs
	- Fix-up ufml_subr.c

- user.h:
	- short	u_uisa[16];					/* segmentation address prototypes */
	- short	u_uisd[16];					/* segmentation descriptor prototypes */
	- char	u_sep;						/* flag for I and D separation */
	- struct u_ovd						/* automatic overlay data */

NetBSD 5.0.2: Threads & Multitasking
- kern_proc.c
- kern_lwp.c
- kern_rwlock.c
- kern_runq.c
- subr_threadpool.c
- subr_workqueue.c

FreeBSD 3.0.0:
- sys_pipe.c & pipe.h

Memory Segmentation (Hardware): CPU Registers
Memory Segmentation (Software):
Seperate Process Segments: text, data, stack
Seperate Instruction & Data Spaces
