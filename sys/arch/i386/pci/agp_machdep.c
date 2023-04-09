/*	$NetBSD: agp_machdep.c,v 1.4 2002/11/22 15:23:52 fvdl Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agp_machdep.c,v 1.4 2002/11/22 15:23:52 fvdl Exp $");

#include <sys/types.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pcireg.h>
#include <dev/video/agp/agpvar.h>
#include <dev/video/agp/agpreg.h>

#include <machine/cpufunc.h>

void
agp_flush_cache(void)
{
	wbinvd();
}
