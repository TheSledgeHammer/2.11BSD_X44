/*
 * kern_lock2.c
 *
 *  Created on: 8 Oct 2020
 *      Author: marti
 */
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/witness.h>
#include <sys/user.h>

struct abql_cpu {
	volatile u_int		abqlc_my_ticket;
};

struct abql {
	struct abql_cpu	 	abql_cpus[MAXCPUS];
	volatile u_int		abql_nxt_ticket;
	int					abql_can_serve;

#ifdef WITNESS
	struct lock_object	abql_lock_obj;
#endif
};

/* Array-Based Queuing Lock (ABQL) */
void
abql_init(abql, type)
	struct abql *abql;
	const struct lock_type *type;
{
	memset(abql->abql_cpus, 0, sizeof(abql->abql_cpus));

	abql->abql_nxt_ticket = 0;
	 for (int i = 1; i < cpu_number(); i++) {
		 abql->abql_can_serve[i] = 0;
	 }
	 abql->abql_can_serve[0] = 1;

#ifdef WITNESS
	abql->abql_lock_obj.lo_name = type->lt_name;
	abql->abql_lock_obj.lo_type = type;
	if (abql == &kernel_lock)
		abql->abql_lock_obj.lo_flags = LO_WITNESS | LO_INITIALIZED | LO_SLEEPABLE | (LO_CLASS_KERNEL_LOCK << LO_CLASSSHIFT);
	else if (abql == &sched_lock)
		abql->abql_lock_obj.lo_flags = LO_WITNESS | LO_INITIALIZED | LO_RECURSABLE | (LO_CLASS_SCHED_LOCK << LO_CLASSSHIFT);
	WITNESS_INIT(&abql->abql_lock_obj, type);
#endif
}

void
abql_acquire(lock)
	struct abql *lock;
{
	struct abql_cpu *cpu = &lock->abql_cpus[cpu_number()];
	unsigned long s;

#ifdef WITNESS
	if (!cpu->abqlc_my_ticket == lock->abql_nxt_ticket)
			WITNESS_CHECKORDER(&lock->abql_lock_obj, LOP_EXCLUSIVE | LOP_NEWORDER, NULL);
#endif

	s = intr_disable();
	cpu->abqlc_my_ticket = atomic_inc_int_nv(lock->abql_nxt_ticket);
	while(lock->abql_can_serve[&cpu->abqlc_my_ticket] != 1);
	intr_restore(s);

	WITNESS_LOCK(&lock->abql_lock_obj, LOP_EXCLUSIVE);
}

void
abql_release(lock)
	struct abql *lock;
{
	struct abql_cpu *cpu = &lock->abql_cpus[cpu_number()];
	unsigned long s;

	WITNESS_UNLOCK(&lock->abql_lock_obj, LOP_EXCLUSIVE);

	s = intr_disable();
	lock->abql_can_serve[&cpu->abqlc_my_ticket + 1] = 1;
	lock->abql_can_serve[&cpu->abqlc_my_ticket] = 0;
	intr_restore(s);
}
