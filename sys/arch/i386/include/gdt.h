/*
 * gdt.h
 *
 *  Created on: 9 Mar 2020
 *      Author: marti
 */

#ifndef _MACHINE_GDT_H_
#define _MACHINE_GDT_H_

void allocate_gdt(struct soft_segment_descriptor *gdt[]);
void allocate_ldt(struct soft_segment_descriptor *ldt[]);

#endif /* _MACHINE_GDT_H_ */
