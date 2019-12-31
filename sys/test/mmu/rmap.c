
/* 4.3BSD Reno Code */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/map.h>
#include <vm/vm.h>

void
rminit(mp, size, addr, name, mapsize)
	register struct map *mp;
	size_t size, addr;
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

size_t
rmget(mp, size, addr)
	register struct map *mp;
	size_t size, addr;
{
	register struct mapent *ep = (struct mapent *)(mp+1);
	register struct mapent *bp, *bp2;

	if(size <= 0) {
		panic("rmget");
	}
	if(mp == swapmap) {
		return (0);
	}

	/*
	 * Look for a map segment containing the requested address.
	 * If none found, return failure.
	 */
	for (bp = ep; bp->m_size; bp++) {
		if (bp->m_addr <= addr && bp->m_addr + bp->m_size > addr) {
			break;
		}
	}
	if (bp->m_size == 0) {
		return (0);
	}

	/*
	 * If segment is too small, return failure.
	 * If big enough, allocate the block, compressing or expanding
	 * the map as necessary.
	 */
	if (bp->m_addr + bp->m_size < addr + size) {
		return (0);
	}
	if (bp->m_addr == addr) {
		if (bp->m_addr + bp->m_size == addr + size) {
			/*
			 * Allocate entire segment and compress map
			 */
			bp2 = bp;
			while (bp2->m_size) {
				bp2++;
				(bp2-1)->m_addr = bp2->m_addr;
				(bp2-1)->m_size = bp2->m_size;
			}
		} else {
			/*
			 * Allocate first part of segment
			 */
			bp->m_addr += size;
			bp->m_size -= size;
		}
	} else {
		if (bp->m_addr + bp->m_size == addr + size) {
			/*
			 * Allocate last part of segment
			 */
			bp->m_size -= size;
		} else {
			/*
			 * Allocate from middle of segment, but only
			 * if table can be expanded.
			 */
			for (bp2=bp; bp2->m_size; bp2++) {
				;
			}
			if (bp2 + 1 >= mp->m_limit) {
				return (0);
			}
			while(bp2 > bp) {
				(bp2+1)->m_addr = bp2->m_addr;
				(bp2+1)->m_size = bp2->m_size;
				bp2--;
			}
			(bp+1)->m_addr = addr + size;
			(bp+1)->m_size = bp->m_addr + bp->m_size - (addr + size);
			bp->m_size = addr - bp->m_addr;
		}
	}
	return (addr);
}
