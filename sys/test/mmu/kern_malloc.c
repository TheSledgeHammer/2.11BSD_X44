
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <test/mmu/malloc.h>

#include <vm/vm.h>
#include <vm/include/vm_kern.h>

struct kmemtree_entry tree_bucket_entry[MINBUCKET + 16];
struct kmembuckets bucket[MINBUCKET + 16];
struct kmemstats kmemstats[M_LAST];
struct kmemusage *kmemusage;
char *kmembase, *kmemlimit;


void *
malloc(size, type, flags)
    unsigned long size;
    int type, flags;
{
    	register struct kmemtree_entry *ktep;
    	register struct kmemtree *ktp;
        register struct kmembuckets *kbp;
        register struct kmemusage *kup;
        //register freelist *freep;
        long indx, npg, allocsize;
        int s;
        char va, cp, savedlist;

        indx = BUCKETINDX(size);
        kbp = &bucket[indx];
        ktep = &tree_bucket_entry[indx];
        s = splimp();

        if (kbp->kb_next == NULL) {
        	kbp->kb_last = NULL;
        	kmemtree_entry(ktep, kbp->kb_next, kbp->kb_last);
        	ktp = kmemtree_init(&ktep[indx], size);

        	if(ktp->kt_parent == NULL) {
        		kmemtree_create(ktp, TRUE);
        	} else {
        		/* find tree and insert if possible */
        	}
        	/* start of section to move into trealloc */
            if (size > MAXALLOCSAVE) {
                allocsize = roundup(size, CLBYTES);
            } else {
                allocsize = 1 << indx;
            }
            npg = clrnd(btoc(allocsize));
            va = (caddr_t) kmem_malloc(kmem_map, (vm_size_t)ctob(npg),
            					   !(flags & M_NOWAIT));
            if (va == NULL) {
            	splx(s);
            }
            /* end of section to move to trealloc */
        	goto out;
        }

        out:
        return ((void *) va);
}

void
kmeminit()
{
	register long indx;
	int npg;
#if	((MAXALLOCSAVE & (MAXALLOCSAVE - 1)) != 0)
		ERROR!_kmeminit:_MAXALLOCSAVE_not_power_of_2
#endif
#if	(MAXALLOCSAVE > MINALLOCSIZE * 32768)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_big
#endif
#if	(MAXALLOCSAVE < CLBYTES)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_small
#endif
		npg = VM_KMEM_SIZE / NBPG;
		kmemusage = (struct kmemusage *) kmem_alloc(kernel_map, (vm_size_t)(npg * sizeof(struct kmemusage)));
		kmem_map = kmem_suballoc(kernel_map, (vm_offset_t *)&kmembase, (vm_offset_t *)&kmemlimit, (vm_size_t)(npg * NBPG), FALSE);
		rminit(coremap, (long)npg, (long)CLSIZE, "malloc map", npg);
#ifdef KMEMSTATS
		for(indx = 0; indx < MINBUCKET + 16; indx++) {
			if (1 << indx >= CLBYTES) {
				bucket[indx].kb_elmpercl = 1;
			} else {
				bucket[indx].kb_elmpercl = CLBYTES / (1 << indx);
			}
			bucket[indx].kb_highwat = 5 * bucket[indx].kb_elmpercl;
		}
		for (indx = 0; indx < M_LAST; indx++) {
			kmemstats[indx].ks_limit = npg * NBPG * 6 / 10;
		}
#endif
}
