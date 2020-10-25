/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/witness.h>
#include <sys/user.h>

/* lock class */
struct lock_class lock_class_abql = {
    .lc_name = "abql",
	.lc_flags = LC_SPINLOCK,
	.lc_lock = abql_acquire,
	.lc_unlock = abql_release
};

/* Array-Based Queuing Lock (ABQL) */

struct abql_cpu {
	volatile u_int		abqlc_my_ticket;
};

struct abql {
	struct abql_cpu	 	abql_cpus[MAXCPUS];
	volatile u_int		abql_nxt_ticket;
	int					abql_can_serve;

	struct lock_object	abql_lockobject;
};

void
abql_init(abql, type)
	struct abql 			*abql;
	const struct lock_type 	*type;
{
	memset(abql->abql_cpus, 0, sizeof(abql->abql_cpus));

	abql->abql_nxt_ticket = 0;
	 for (int i = 1; i < cpu_number(); i++) {
		 abql->abql_can_serve[i] = 0;
	 }
	 abql->abql_can_serve[0] = 1;

	 lockobject_init(&abql->abql_lockobject, type, "abql");
}

void
abql_acquire(abql)
	struct abql *abql;
{
	struct abql_cpu *cpu = &abql->abql_cpus[cpu_number()];
	unsigned long s;

#ifdef WITNESS
	if (!cpu->abqlc_my_ticket == abql->abql_nxt_ticket)
			WITNESS_CHECKORDER(&abql->abql_lockobject, LOP_EXCLUSIVE | LOP_NEWORDER, NULL);
#endif

	s = intr_disable();
	cpu->abqlc_my_ticket = atomic_inc_int_nv(abql->abql_nxt_ticket);
	while(abql->abql_can_serve[&cpu->abqlc_my_ticket] != 1);
	intr_restore(s);
	lockobject_lock(&abql->abql_lockobject, LOP_EXCLUSIVE);
}

void
abql_release(abql)
	struct abql 	*abql;
{
	struct abql_cpu *cpu = &abql->abql_cpus[cpu_number()];
	unsigned long s;

	lockobject_unlock(&abql->abql_lockobject, LOP_EXCLUSIVE);

	s = intr_disable();
	abql->abql_can_serve[&cpu->abqlc_my_ticket + 1] = 1;
	abql->abql_can_serve[&cpu->abqlc_my_ticket] = 0;
	intr_restore(s);
}
