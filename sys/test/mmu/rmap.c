
/* 4.3BSD Reno Code */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/map.h>
#include <vm/vm.h>

/* Place in machine-dependent main() */
void
rminit(mp, size, addr, name, mapsize)
	register struct map *mp;
	long size, addr;
	char *name;
	int mapsize;
{
		register struct mapent *ep = (struct mapent *)(mp+1);

		mp->m_name = name;
		mp->m_limit = (struct mapent *)&mp[mapsize];
		ep->m_size = size;
		ep->m_addr = addr;
		(++ep)->m_size = 0;
		ep->m_addr = 0;
}

/*
 * Allocate 'size' units from the given map, starting at address 'addr'.
 * Return 'addr' if successful, 0 if not.
 * This may cause the creation or destruction of a resource map segment.
 *
 * This routine will return failure status if there is not enough room
 * for a required additional map segment.
 *
 * An attempt to use this on 'swapmap' will result in
 * a failure return.  This is due mainly to laziness and could be fixed
 * to do the right thing, although it probably will never be used.
 */
rmget(mp, size, addr)
	register struct map *mp;
{
	register struct mapent *ep = (struct mapent *)(mp+1);
	register struct mapent *bp, *bp2;
}

/*
 * mmu_init
 * tbs_init (tertiary buddy system)
 */

/*
 * rmalloc
 * rmfree
 * rmalloc3
 * rmget
 */
