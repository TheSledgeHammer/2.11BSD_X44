/*
 * subr_boot.c
 *
 *  Created on: 18 Jun 2020
 *      Author: marti
 */
#include <sys/bootinfo.h>
#include <machine/bootinfo.h>

struct bootinfo_bios *
get_bootinfo_bios(bootinfo)
	union bootinfo *bootinfo;
{
	return (bootinfo->bi_bios);
}

struct bootinfo_efi *
get_bootinfo_efi(bootinfo)
	union bootinfo *bootinfo;
{
	return (bootinfo->bi_efi);
}

struct bootinfo_enivronment *
get_bootinfo_environment(bootinfo)
	union bootinfo *bootinfo;
{
	return (bootinfo->bi_envp);
}

struct bootinfo_bootdisk *
get_bootinfo_bootdisk(bootinfo)
	union bootinfo *bootinfo;
{
	return (bootinfo->bi_disk);
}

struct bootinfo_netif *
get_bootinfo_netif(bootinfo)
	union bootinfo *bootinfo;
{
	return (bootinfo->bi_net);
}

struct bootinfo_console *
get_bootinfo_console(bootinfo)
	union bootinfo *bootinfo;
{
	return (bootinfo->bi_cons);
}

struct bootinfo_biogeom *
get_bootinfo_biogeom(bootinfo)
	union bootinfo *bootinfo;
{
	return (bootinfo->bi_geom);
}
