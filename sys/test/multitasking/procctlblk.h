/*
 * procctlblk.h
 *
 *  Created on: 3 Jan 2020
 *      Author: marti
 */
/*
 * Process Control Block
 */

#ifndef SYS_PROCCTLBLK_H_
#define SYS_PROCCTLBLK_H_


struct procctlblk {

	struct proc *p;
	proc_state;
	proc_id;
	proc_cnt;
	flags;
	signals;

};


#endif /* SYS_PROCCTLBLK_H_ */
