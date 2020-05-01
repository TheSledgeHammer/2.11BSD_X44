/*
 * consinit.c
 *
 *  Created on: 1 May 2020
 *      Author: marti
 */



/*
 * consinit:
 * initialize the system console.
 * XXX - shouldn't deal with this initted thing, but then,
 * it shouldn't be called from init386 either.
 */
void
consinit()
{
	struct bootinfo_console *consinfo;
	static int initted;

	if (initted)
		return;
	initted = 1;

#ifndef CONS_OVERRIDE
	consinfo = lookup_bootinfo(BTINFO_CONSOLE);
	if (!consinfo)
#endif
		consinfo = &default_consinfo;

#if (NPC > 0) || (NVT > 0)
	if(!strcmp(consinfo->devname, "pc")) {
		pccnattach();
		return;
	}
#endif
#if (NCOM > 0)
	if(!strcmp(consinfo->devname, "com")) {
		bus_space_tag_t tag = I386_BUS_SPACE_IO;

		if(comcnattach(tag, consinfo->addr, consinfo->speed,
			       COM_FREQ, comcnmode))
			panic("can't init serial console @%x", consinfo->addr);

		return;
	}
#endif
	panic("invalid console device %s", consinfo->devname);
}

#ifdef KGDB
void
kgdb_port_init()
{
#if (NCOM > 0)
	if(!strcmp(kgdb_devname, "com")) {
		bus_space_tag_t tag = I386_BUS_SPACE_IO;

		com_kgdb_attach(tag, comkgdbaddr, comkgdbrate, COM_FREQ,
		    comkgdbmode);
	}
#endif
}
#endif
