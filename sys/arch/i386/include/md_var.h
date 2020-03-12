/*
 * md_var.h
 *
 *  Created on: 9 Mar 2020
 *      Author: marti
 */


#ifndef _MACHINE_MD_VAR_H_
#define	_MACHINE_MD_VAR_H_

void	set_fsbase(struct proc *p, uint32_t base);
void	set_gsbase(struct proc *p, uint32_t base);

#endif /* !_MACHINE_MD_VAR_H_ */
