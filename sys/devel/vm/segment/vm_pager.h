#ifndef	_VM_PAGER_
#define	_VM_PAGER_

#include <devel/vm/include/vm.h>

/*
 * forward structure declarations
 */
struct vm_faultinfo;

TAILQ_HEAD(pagerlst, pager_struct);
struct	pager_struct {
	TAILQ_ENTRY(pager_struct) 	pg_list;	/* links for list management */
	caddr_t			  			pg_handle;	/* ext. handle (vp, dev, fp) */
	int			  				pg_type;	/* type of pager */
	int			  				pg_flags;	/* flags */
	struct vm_pagerops			*pg_ops;	/* pager operations */
	void			  			*pg_data;	/* private pager data */
};

/* pager types */
#define PG_DFLT		-1
#define	PG_SWAP		0
#define	PG_VNODE	1
#define PG_DEVICE	2

/* flags */
#define PG_CLUSTERGET	1
#define PG_CLUSTERPUT	2

struct	vm_pagerops {
	/* Initialize pager. */
	void		(*pgo_init)(void);

	/* add reference to obj */
	void		(*pgo_reference)(struct vm_object *);

	/* drop reference to obj */
	void		(*pgo_detach)(struct vm_object *);

	/* special non-standard fault processing */
	//int	(*pgo_fault)(struct uvm_faultinfo *, vaddr_t, struct vm_page **, int, int, vm_fault_t, vm_prot_t, int);

	/* Allocate pager. */
	vm_pager_t	(*pgo_alloc)(caddr_t, vm_size_t, vm_prot_t, vm_offset_t);

	/* Disassociate. */
	void		(*pgo_dealloc)(vm_pager_t);

	/* Get (read) page. */
	int			(*pgo_getpages)(vm_pager_t, vm_page_t *, int, boolean_t);

	/* Put (write) page. */
	int			(*pgo_putpages)(vm_pager_t, vm_page_t *, int, boolean_t);

	/* Does pager have page? */
	boolean_t  	(*pgo_haspage)(vm_pager_t, vm_offset_t);

	/* Return range of cluster. */
	void		(*pgo_cluster)(vm_pager_t, vm_offset_t, vm_offset_t *, vm_offset_t *);


	int 		(*pgo_getsegment)(vm_pager_t, vm_segment_t *, int, boolean_t);

	int 		(*pgo_putsegment)(vm_pager_t, vm_segment_t *, int, boolean_t);
};

/*
 * get/put return values
 * OK	 operation was successful
 * BAD	 specified data was out of the accepted range
 * FAIL	 specified data was in range, but doesn't exist
 * PEND	 operations was initiated but not completed
 * ERROR error while accessing data that is in range and exists
 * AGAIN temporary resource shortage prevented operation from happening
 */
#define	VM_PAGER_OK		0
#define	VM_PAGER_BAD	1
#define	VM_PAGER_FAIL	2
#define	VM_PAGER_PEND	3
#define	VM_PAGER_ERROR	4
#define VM_PAGER_AGAIN	5

//#ifdef KERNEL
extern struct vm_pagerops *dfltpagerops;

vm_pager_t	 vm_pager_allocate (int, caddr_t, vm_size_t, vm_prot_t, vm_offset_t);
vm_page_t	 vm_pager_atop (vm_offset_t);
void		 vm_pager_cluster (vm_pager_t, vm_offset_t, vm_offset_t *, vm_offset_t *);
void		 vm_pager_clusternull (vm_pager_t, vm_offset_t, vm_offset_t *, vm_offset_t *);
void		 vm_pager_deallocate (vm_pager_t);
int		 	 vm_pager_get_pages (vm_pager_t, vm_page_t *, int, boolean_t);
boolean_t	 vm_pager_has_page (vm_pager_t, vm_offset_t);
void		 vm_pager_init (void);
vm_pager_t	 vm_pager_lookup (struct pagerlst *, caddr_t);
vm_offset_t	 vm_pager_map_pages (vm_page_t *, int, boolean_t);
int		 	 vm_pager_put_pages	(vm_pager_t, vm_page_t *, int, boolean_t);
void		 vm_pager_sync (void);
void		 vm_pager_unmap_pages (vm_offset_t, int);

#define vm_pager_cancluster(p, b)	((p)->pg_flags & (b))

/*
 * XXX compat with old interface
 */
int		 	vm_pager_get (vm_pager_t, vm_page_t, boolean_t);
int		 	vm_pager_put (vm_pager_t, vm_page_t, boolean_t);
//#endif

#endif	/* _VM_PAGER_ */
