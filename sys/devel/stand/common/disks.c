/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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

#include <sys/reboot.h>

#include "bootstrap.h"

int
disk_device_type(uint32_t bootdev)
{
	return (B_TYPE(bootdev));
}

int
disk_device_adaptor(uint32_t bootdev)
{
	return (B_ADAPTOR(bootdev) << 4);
}

int
disk_device_controller(uint32_t bootdev)
{
	return (B_CONTROLLER(bootdev) - 1);
}

int
disk_device_slice(uint32_t bootdev)
{
#if DISK_SLICES
	return (B_SLICE(bootdev) - 1);
#else
	return (disk_device_adaptor(bootdev) + disk_device_controller(bootdev));
#endif
}

int
disk_device_partition(uint32_t bootdev)
{
	return (B_PARTITION(bootdev));
}

void
disk_format(uint32_t bootdev, int type, int adaptor, int controller, int slice, int partition)
{
	type = disk_device_type(bootdev);
	adaptor = disk_device_adaptor(bootdev);
	controller = disk_device_controller(bootdev);
	slice = disk_device_slice(bootdev);
	partition = disk_device_partition(bootdev);
}
