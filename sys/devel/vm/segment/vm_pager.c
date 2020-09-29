/*
 * vm_pager.c
 *
 *  Created on: 28 Sep 2020
 *      Author: marti
 */


int
vm_pager_get_pages(pager, mlist, npages, sync)
	vm_pager_t	pager;
	vm_segment_t	*mlist;
	int		npages;
	boolean_t	sync;
{
	int rv;

	if (pager == NULL) {
		rv = VM_PAGER_OK;
		while (npages--)
			if (!vm_page_zero_fill(*mlist)) {
				rv = VM_PAGER_FAIL;
				break;
			} else
				mlist++;
		return (rv);
	}
	return ((*pager->pg_ops->pgo_getsegment)(pager, mlist, npages, sync));
}

int
vm_pager_put_pages(pager, mlist, npages, sync)
	vm_pager_t	pager;
	vm_segment_t	*mlist;
	int		npages;
	boolean_t	sync;
{
	if (pager == NULL)
		panic("vm_pager_put_pages: null pager");
	return ((*pager->pg_ops->pgo_putsegment)(pager, mlist, npages, sync));
}

/* XXX compatibility*/
int
vm_pager_get(pager, m, sync)
	vm_pager_t	pager;
	vm_segment_t	m;
	boolean_t	sync;
{
	return vm_pager_get_pages(pager, &m, 1, sync);
}

/* XXX compatibility*/
int
vm_pager_put(pager, m, sync)
	vm_pager_t	pager;
	vm_segment_t	m;
	boolean_t	sync;
{
	return vm_pager_put_pages(pager, &m, 1, sync);
}
