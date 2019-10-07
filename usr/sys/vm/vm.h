/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm.h	7.1 (Berkeley) 6/4/86
 */

/*
 *	#include "../h/vm.h"
 * or	#include <vm.h>		 in a user program
 * is a quick way to include all the vm header files.
 */
#ifdef KERNEL
#include "../vm/vmparam.h"
#include "../vm/vmmac.h"
#include "../vm/vmmeter.h"
#include "../vm/vmsystm.h"
#else
#include "../vm/vmparam.h"
#include "../vm/vmmac.h"
#include "../vm/vmmeter.h"
#include "../vm/vmsystm.h"
#endif
