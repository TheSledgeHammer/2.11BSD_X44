/*
 * vm_aobj.c
 *
 *  Created on: 10 Mar 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <dev/vm/uvm.h>

/*
 * uao_swhash: the swap hash table structure
 */

LIST_HEAD(uao_swhash, uao_swhash_elt);

/*
 * uao_swhash_elt_pool: pool of uao_swhash_elt structures.
 * Note: pages for this pool must not come from a pageable kernel map.
 */
static struct pool	uao_swhash_elt_pool;//	__cacheline_aligned;

/*
 * aobj_pager
 *
 * note that some functions (e.g. put) are handled elsewhere
 */

const struct pagerops aobj_pager = {
	.pgo_reference = uao_reference,
	.pgo_detach = uao_detach,
	.pgo_get = uao_get,
	.pgo_put = uao_put,
};

/*
 * uao_list: global list of active aobjs, locked by uao_list_lock
 */

static LIST_HEAD(aobjlist, uvm_aobj) uao_list;	//__cacheline_aligned;
static kmutex_t						 uao_list_lock;//		__cacheline_aligned;

/*
 * hash table/array related functions
 */

#if defined(VMSWAP)

/*
 * uao_find_swhash_elt: find (or create) a hash table entry for a page
 * offset.
 *
 * => the object should be locked by the caller
 */


static struct uao_swhash_elt *
uao_find_swhash_elt(aobj, pageidx, create)
	struct uvm_aobj *aobj;
	int pageidx;
	boolean_t create;
{
	struct uao_swhash *swhash;
	struct uao_swhash_elt *elt;
	voff_t page_tag;

	swhash = UAO_SWHASH_HASH(aobj, pageidx);
	page_tag = UAO_SWHASH_ELT_TAG(pageidx);

	/*
	 * now search the bucket for the requested tag
	 */

	LIST_FOREACH(elt, swhash, list)
	{
		if (elt->tag == page_tag) {
			return elt;
		}
	}
	if (!create) {
		return NULL;
	}

	/*
	 * allocate a new entry for the bucket and init/insert it in
	 */

	elt = pool_get(&uao_swhash_elt_pool, M_NOWAIT);
	if (elt == NULL) {
		return NULL;
	}
	LIST_INSERT_HEAD(swhash, elt, list);
	elt->tag = page_tag;
	elt->count = 0;
	memset(elt->slots, 0, sizeof(elt->slots));
	return elt;
}
