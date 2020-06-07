/*
 * htbc_ops.h
 *
 *  Created on: 7 Jun 2020
 *      Author: marti
 */

#ifndef SYS_HTBC_OPS_H_
#define SYS_HTBC_OPS_H_

struct htbc_ops {
	int (*ho_htbc_begin)(struct htbc *, const char *, int);
	void (*ho_htbc_end)(struct htbc *);
};

#endif /* SYS_HTBC_OPS_H_ */
