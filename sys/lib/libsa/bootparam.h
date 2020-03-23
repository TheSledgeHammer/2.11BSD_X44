/*
 * bootparam.h
 *
 *  Created on: 22 Mar 2020
 *      Author: marti
 */

#ifndef LIBSA_BOOTPARAM_H_
#define LIBSA_BOOTPARAM_H_

int bp_whoami(int);
int bp_getfile(int, char *, struct in_addr *, char *);

#endif /* LIBSA_BOOTPARAM_H_ */
