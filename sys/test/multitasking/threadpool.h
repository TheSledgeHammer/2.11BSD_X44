/*
 * threadpool.h
 *
 *  Created on: 3 Jan 2020
 *      Author: marti
 */

#ifndef SYS_THREADPOOL_H_
#define SYS_THREADPOOL_H_

/*
 * Two Threadpools:
 * - Kernel Thread pool
 * - User Thread pool
 *
 * primary jobs:
 * - dispatch and hold x number of threads
 * - send & receive jobs between user & kernel threads when applicable
 *
 * IPC:
 * - Use thread pool/s as the ipc. (bi-directional fifo queue)
 * 	- i.e. all requests go through the associated thread pool.
 * 	- requests sent in order received. (no priorities)
 * 	- confirmation: to prevent thread pools requesting the same task
 */

#endif /* SYS_THREADPOOL_H_ */
