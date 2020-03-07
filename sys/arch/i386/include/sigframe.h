/*
 * sigframe.h
 *
 *  Created on: 7 Mar 2020
 *      Author: marti
 */

#ifndef _MACHINE_SIGFRAME_H_
#define	_MACHINE_SIGFRAME_H_

struct sigframe {
	int					sf_signum;
	int					sf_code;
	struct	sigcontext 	*sf_scp;
	sig_t				sf_handler;
	int					sf_eax;
	int					sf_edx;
	int					sf_ecx;
	struct	sigcontext 	sf_sc;
};


#endif /* _MACHINE_SIGFRAME_H_ */
