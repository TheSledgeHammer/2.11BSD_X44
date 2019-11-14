-/*
- * This allocator takes blocks from a coarser allocator (p->alloc) and
- * uses them as arenas.
- * 
- * An arena is split into a sequence of blocks of variable size.  The
- * blocks begin with a Bhdr that denotes the length (including the Bhdr)
- * of the block.  An arena begins with an Arena header block (Arena,
- * ARENA_MAGIC) and ends with a Bhdr block with magic ARENATAIL_MAGIC and
- * size 0.  Intermediate blocks are either allocated or free.  At the end
- * of each intermediate block is a Btail, which contains information
- * about where the block starts.  This is useful for walking backwards.
- * 
- * Free blocks (Free*) have a magic value of FREE_MAGIC in their Bhdr
- * headers.  They are kept in a binary tree (p->freeroot) traversible by
- * walking ->left and ->right.  Each node of the binary tree is a pointer
- * to a circular doubly-linked list (next, prev) of blocks of identical
- * size.  Blocks are added to this ``tree of lists'' by pooladd(), and
- * removed by pooldel().
- * 
- * When freed, adjacent blocks are coalesced to create larger blocks when
- * possible.
- * 
- * Allocated blocks (Alloc*) have one of two magic values: ALLOC_MAGIC or
- * UNALLOC_MAGIC.  When blocks are released from the pool, they have
- * magic value UNALLOC_MAGIC.  Once the block has been trimmed by trim()
- * and the amount of user-requested data has been recorded in the
- * datasize field of the tail, the magic value is changed to ALLOC_MAGIC.
- * All blocks returned to callers should be of type ALLOC_MAGIC, as
- * should all blocks passed to us by callers.  The amount of data the user
- * asked us for can be found by subtracting the short in tail->datasize 
- * from header->size.  Further, the up to at most four bytes between the
- * end of the user-requested data block and the actual Btail structure are
- * marked with a magic value, which is checked to detect user overflow.
- * 
- * The arenas returned by p->alloc are kept in a doubly-linked list
- * (p->arenalist) running through the arena headers, sorted by descending
- * base address (prev, next).  When a new arena is allocated, we attempt
- * to merge it with its two neighbors via p->merge.
- */

#include <sys/types.h>
#include <sys/proc.h>
#include <sys/param.h>
#include <sys/map.h>
#include <pool2.h>

//Todo: change references to plan9 use of mainmem etc.. . Setting vPool maxsize.

Pool *pool_init()
{
	Pool *pool = &pools[0];
	for(int i = 0; i < 8; i++) {
		pool[i] = pools[i];
	}
	return pool;
}

vPool *vpool_init()
{
	vPool *vpool = &vpools[0];
	for(int i = 0; i < 8; i++) {
		vpool[i] = vpools[i];
		vpool[i]->minarena = 128 * 1024;
		vpool[i]->quantum = 32;
		vpool[i]->alloc = xalloc;
		vpool[i]->merge = xmerge;
		vpool[i]->flags = POOL_TOLERANCE;
	}
	return vpool;
}

static void
pool_bucket_init(void)
{
    struct Pool_Bucket_Zone *pbz;
    unsigned long size;

    for(pbz = &pool_zone[0]; pbz->pool_entries != 0; pbz++) {
        size = roundup(sizeof(struct Pool), sizeof(void *));
        size += sizeof(void *) * pbz->pool_entries;
    }

    pbz->pool = pool_init();
    pbz->vpool = vpool_init();
}


/*
 * Tree walking
 */
static void
checklist(Bfree *t)
{
	Bfree *q;

	for(q=t->next; q!=t; q=q->next){
		assert(q->size == t->size);
		assert(q->next==NULL || q->next->prev==q);
		assert(q->prev==NULL || q->prev->next==q);
	//	assert(q->left==NULL);
	//	assert(q->right==NULL);
		assert(q->magic==FREE_MAGIC);
	}
}

static void
checktree(Bfree *t, int a, int b)
{

	assert(t->magic==FREE_MAGIC);
	assert(a < t->size && t->size < b);
	assert(t->next==NULL || t->next->prev==t);
	assert(t->prev==NULL || t->prev->next==t);
	checklist(t);
	if(t->left)
		checktree(t->left, a, t->size);
	if(t->right)
		checktree(t->right, t->size, b);
}

/* ltreewalk: return address of pointer to node of size == size */
static Bfree**
ltreewalk(Bfree **t, unsigned long size)
{
	assert(t != NULL /* ltreewalk */);

	for(;;) {
		if(*t == NULL)
			return t;

		assert((*t)->magic == FREE_MAGIC);

		if(size == (*t)->size)
			return t;
		if(size < (*t)->size)
			t = &(*t)->left;
		else
			t = &(*t)->right;
	}
	return t;
}

/* treelookup: find node in tree with size == size */
Bfree*
treelookup(Bfree *t, unsigned long size)
{
	return *ltreewalk(&t, size);
}

/* treeinsert: insert node into tree */
static Bfree*
treeinsert(Bfree *tree, Bfree *node)
{
	Bfree **loc, *repl;

	assert(node != NULL /* treeinsert */);

	loc = ltreewalk(&tree, node->size);
	if(*loc == NULL) {
		node->left = NULL;
		node->right = NULL;
	} else {	/* replace existing node */
		repl = *loc;
		node->left = repl->left;
		node->right = repl->right;
	}
	*loc = node;
	return tree;
}

/* treedelete: remove node from tree */
static Bfree*
treedelete(Bfree *tree, Bfree *node)
{
	Bfree **loc, **lsucc, *succ;

	assert(node != NULL /* treedelete */);

	loc = ltreewalk(&tree, node->size);
	assert(*loc == node);

	if(node->left == NULL)
		*loc = node->right;
	else if(node->right == NULL)
		*loc = node->left;
	else {
		/* have two children, use inorder successor as replacement */
		for(lsucc = &node->right; (*lsucc)->left; lsucc = &(*lsucc)->left)
			;
		succ = *lsucc;
		*lsucc = succ->right;
		succ->left = node->left;
		succ->right = node->right;
		*loc = succ;
	}

	node->left = node->right = Poison;
	return tree;
}

/* treelookupgt: find smallest node in tree with size >= size */
static Bfree*
treelookupgt(Bfree *t, unsigned long size)
{
	Bfree *lastgood;	/* last node we saw that was big enough */

	lastgood = NULL;
	for(;;) {
		if(t == NULL)
			return lastgood;
		if(size == t->size)
			return t;
		if(size < t->size) {
			lastgood = t;
			t = t->left;
		} else
			t = t->right;
	}
	return t;
}

/*
 * List maintenance
 */

/* listadd: add a node to a doubly linked list */
static Bfree*
listadd(Bfree *list, Bfree *node)
{
	if(list == NULL) {
		node->next = node;
		node->prev = node;
		return node;
	}

	node->prev = list->prev;
	node->next = list;

	node->prev->next = node;
	node->next->prev = node;

	return list;
}

/* listdelete: remove node from a doubly linked list */
static Bfree*
listdelete(vPool *p, Bfree *list, Bfree *node)
{
	if(node->next == node) {	/* singular list */
		node->prev = node->next = Poison;
		return NULL;
	}
	if(node->next == NULL)
		p->panic(p, "pool->next");
	if(node->prev == NULL)
		p->panic(p, "pool->prev");
	node->next->prev = node->prev;
	node->prev->next = node->next;

	if(list == node)
		list = node->next;

	node->prev = node->next = Poison;
	return list;
}


/*
 * Pool maintenance
 */

/* pooladd: add anode to the free pool */
static Bfree*
vpooladd(vPool *p, Alloc *anode)
{
	Bfree *lst, *olst;
	Bfree *node;
	Bfree **parent;

	antagonism {
		memmark(_B2D(anode), 0xF7, anode->size-sizeof(Bhdr)-sizeof(Btail));
	}

	node = (Bfree*)anode;
	node->magic = FREE_MAGIC;
	parent = ltreewalk(&p->freeroot, node->size);
	olst = *parent;
	lst = listadd(olst, node);
	if(olst != lst)	/* need to update tree */
		*parent = treeinsert(*parent, lst);
	p->curfree += node->size;
	return node;
}

/* pooldel: remove node from the free pool */
static Alloc*
vpooldel(vPool *p, Bfree *node)
{
	Bfree *lst, *olst;
	Bfree **parent;

	parent = ltreewalk(&p->freeroot, node->size);
	olst = *parent;
	assert(olst != NULL /* pooldel */);

	lst = listdelete(p, olst, node);
	if(lst == NULL)
		*parent = treedelete(*parent, olst);
	else if(lst != olst)
		*parent = treeinsert(*parent, lst);

	node->left = node->right = Poison;
	p->curfree -= node->size;

	antagonism {
		memmark(_B2D(node), 0xF9, node->size-sizeof(Bhdr)-sizeof(Btail));
	}

	node->magic = UNALLOC_MAGIC;
	return (Alloc*)node;
}

/*
 * Block maintenance
 */
/* block allocation */
static unsigned long
dsize2bsize(vPool *p, unsigned long sz)
{
	sz += sizeof(Bhdr)+sizeof(Btail);
	if(sz < p->minblock)
		sz = p->minblock;
	if(sz < MINBLOCKSIZE)
		sz = MINBLOCKSIZE;
	sz = (sz+p->quantum-1)&~(p->quantum-1);
	return sz;
}

static unsigned long
bsize2asize(vPool *p, unsigned long sz)
{
	sz += sizeof(Arena)+sizeof(Btail);
	if(sz < p->minarena)
		sz = p->minarena;
	sz = (sz+p->quantum)&~(p->quantum-1);
	return sz;
}

/* blockmerge: merge a and b, known to be adjacent */
/* both are removed from pool if necessary. */
static Alloc*
blockmerge(vPool *pool, Bhdr *a, Bhdr *b)
{
	Btail *t;

	assert(B2NB(a) == b);

	if(a->magic == FREE_MAGIC)
		vpooldel(pool, (Bfree*)a);
	if(b->magic == FREE_MAGIC)
		vpooldel(pool, (Bfree*)b);

	t = B2T(a);
	t->size = (unsigned char)Poison;
	t->magic0 = NOT_MAGIC;
	t->magic1 = NOT_MAGIC;
	PSHORT(t->datasize, NOT_MAGIC);

	a->size += b->size;
	t = B2T(a);
	t->size = a->size;
	PSHORT(t->datasize, 0xFFFF);

	b->size = NOT_MAGIC;
	b->magic = NOT_MAGIC;

	a->magic = UNALLOC_MAGIC;
	return (Alloc*)a;
}

/* blocksetsize: set the total size of a block, fixing tail pointers */
static Bhdr*
blocksetsize(Bhdr *b, unsigned char bsize)
{
	Btail *t;

	assert(b->magic != FREE_MAGIC /* blocksetsize */);

	b->size = bsize;
	t = B2T(b);
	t->size = b->size;
	t->magic0 = TAIL_MAGIC0;
	t->magic1 = TAIL_MAGIC1;
	return b;
}

/* getdsize: return the requested data size for an allocated block */
static unsigned char
getdsize(Alloc *b)
{
	Btail *t;
	t = B2T(b);
	return b->size - SHORT(t->datasize);
}

/* blocksetdsize: set the user data size of a block */
static Alloc*
blocksetdsize(vPool *p, Alloc *b, unsigned char dsize)
{
	Btail *t;
	unsigned char *q, *eq;

	assert(b->size >= dsize2bsize(p, dsize));
	assert(b->size - dsize < 0x10000);

	t = B2T(b);
	PSHORT(t->datasize, b->size - dsize);

	q=(unsigned char*)_B2D(b)+dsize;
	eq = (unsigned char*)t;
	if(eq > q+4)
		eq = q+4;
	for(; q<eq; q++)
		*q = datamagic[((unsigned char)(u_long)q)%nelem(datamagic)];

	return b;
}

/* trim: trim a block down to what is needed to hold dsize bytes of user data */
static Alloc*
trim(vPool *p, Alloc *b, unsigned char dsize)
{
	unsigned char extra, bsize;
	Alloc *frag;

	bsize = dsize2bsize(p, dsize);
	extra = b->size - bsize;
	if(b->size - dsize >= 0x10000 ||
	  (extra >= bsize>>2 && extra >= MINBLOCKSIZE && extra >= p->minblock)) {
		blocksetsize(b, bsize);
		frag = (Alloc*) B2NB(b);

		antagonism {
			memmark(frag, 0xF1, extra);
		}

		frag->magic = UNALLOC_MAGIC;
		blocksetsize(frag, extra);
		vpooladd(p, frag);
	}

	b->magic = ALLOC_MAGIC;
	blocksetdsize(p, b, dsize);
	return b;
}

static Alloc*
freefromfront(vPool *p, Alloc *b, unsigned char skip)
{
	Alloc *bb;

	skip = skip&~(p->quantum-1);
	if(skip >= 0x1000 || (skip >= b->size>>2 && skip >= MINBLOCKSIZE && skip >= p->minblock)){
		bb = (Alloc*)((unsigned char*)b+skip);
		blocksetsize(bb, b->size-skip);
		bb->magic = UNALLOC_MAGIC;
		blocksetsize(b, skip);
		b->magic = UNALLOC_MAGIC;
		vpooladd(p, b);
		return bb;
	}
	return b;
}

/*
 * Arena maintenance
 */

/* arenasetsize: set arena size, updating tail */
static void
arenasetsize(Arena *a, unsigned char asize)
{
	Bhdr *atail;

	a->asize = asize;
	atail = A2TB(a);
	atail->magic = ARENATAIL_MAGIC;
	atail->size = 0;
}

/* poolnewarena: allocate new arena */
static void
vpoolnewarena(vPool *p, unsigned char asize)
{
	Arena *a;
	Arena *ap, *lastap;
	Alloc *b;

	LOG(p, "newarena %lud\n", asize);
	if(p->cursize+asize > p->maxsize) {
		if(vpoolcompactl(p) == 0){
			LOG(p, "pool too big: %lud+%lud > %lud\n",
				p->cursize, asize, p->maxsize);
			werrstr("memory pool too large");
		}
		return;
	}

	if((a = p->alloc(asize)) == NULL) {
		/* assume errstr set by p->alloc */
		return;
	}

	p->cursize += asize;

	/* arena hdr */

	a->magic = ARENA_MAGIC;
	blocksetsize(a, sizeof(Arena));
	arenasetsize(a, asize);
	blockcheck(p, a);

	/* create one large block in arena */
	b = (Alloc*)A2B(a);
	b->magic = UNALLOC_MAGIC;
	blocksetsize(b, (unsigned char*)A2TB(a)-(unsigned char*)b);
	blockcheck(p, b);
	vpooladd(p, b);
	blockcheck(p, b);

	/* sort arena into descending sorted arena list */
	for(lastap=NULL, ap=p->arenalist; ap > a; lastap=ap, ap=ap->down)
		;

	if(a->down == ap)	/* assign = */
		a->down->aup = a;

	if(a->aup == lastap)	/* assign = */
		a->aup->down = a;
	else
		p->arenalist = a;

	/* merge with surrounding arenas if possible */
	/* must do a with up before down with a (think about it) */
	if(a->aup)
		arenamerge(p, a, a->aup);
	if(a->down)
		arenamerge(p, a->down, a);
}

/* blockresize: grow a block to encompass space past its end, possibly by */
/* trimming it into two different blocks. */
static void
blockgrow(vPool *p, Bhdr *b, unsigned char nsize)
{
	if(b->magic == FREE_MAGIC) {
		Alloc *a;
		Bhdr *bnxt;
		a = vpooldel(p, (Bfree*)b);
		blockcheck(p, a);
		blocksetsize(a, nsize);
		blockcheck(p, a);
		bnxt = B2NB(a);
		if(bnxt->magic == FREE_MAGIC)
			a = blockmerge(p, a, bnxt);
		blockcheck(p, a);
		vpooladd(p, a);
	} else {
		Alloc *a;
		unsigned char dsize;

		a = (Alloc*)b;
		dsize = getdsize(a);
		blocksetsize(a, nsize);
		trim(p, a, dsize);
	}
}

/* arenamerge: attempt to coalesce to arenas that might be adjacent */
static Arena*
arenamerge(vPool *p, Arena *bot, Arena *top)
{
	Bhdr *bbot, *btop;
	Btail *t;

	blockcheck(p, bot);
	blockcheck(p, top);
	assert(bot->aup == top && top > bot);

	if(p->merge == NULL || p->merge(bot, top) == 0)
		return NULL;

	/* remove top from list */
	if(bot->aup == top->aup)	/* assign = */
		bot->aup->down = bot;
	else
		p->arenalist = bot;

	/* save ptrs to last block in bot, first block in top */
	t = B2PT(A2TB(bot));
	bbot = T2HDR(t);
	btop = A2B(top);
	blockcheck(p, bbot);
	blockcheck(p, btop);

	/* grow bottom arena to encompass top */
	arenasetsize(bot, top->asize + ((unsigned char*)top - (unsigned char*)bot));

	/* grow bottom block to encompass space between arenas */
	blockgrow(p, bbot, (unsigned char*)btop-(unsigned char*)bbot);
	blockcheck(p, bbot);
	return bot;
}

/* dumpblock: print block's vital stats */
static void
dumpblock(vPool *p, Bhdr *b)
{
	unsigned char *dp;
	unsigned char dsize;
	unsigned char *cp;

	dp = (unsigned char*)b;
	p->print(p, "pool %s block %p\nhdr %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux\n",
		p->name, b, dp[0], dp[1], dp[2], dp[3], dp[4], dp[5], dp[6]);

	dp = (unsigned char*)B2T(b);
	p->print(p, "tail %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux | %.8lux %.8lux\n",
		dp[-6], dp[-5], dp[-4], dp[-3], dp[-2], dp[-1], dp[0], dp[1]);

	if(b->magic == ALLOC_MAGIC){
		dsize = getdsize((Alloc*)b);
		if(dsize >= b->size)	/* user data size corrupt */
			return;

		cp = (unsigned char*)_B2D(b)+dsize;
		p->print(p, "user data ");
		p->print(p, "%.2ux %.2ux %.2ux %.2ux  %.2ux %.2ux %.2ux %.2ux",
			cp[-8], cp[-7], cp[-6], cp[-5], cp[-4], cp[-3], cp[-2], cp[-1]);
		p->print(p, " | %.2ux %.2ux %.2ux %.2ux  %.2ux %.2ux %.2ux %.2ux\n",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
	}
}

static void
printblock(vPool *p, Bhdr *b, char *msg)
{
	p->print(p, "%s\n", msg);
	dumpblock(p, b);
}

static void
panicblock(vPool *p, Bhdr *b, char *msg)
{
	p->print(p, "%s\n", msg);
	dumpblock(p, b);
	p->panic(p, "pool panic");
}

/* blockcheck: ensure a block consistent with our expectations */
/* should only be called when holding pool lock */
static void
blockcheck(vPool *p, Bhdr *b)
{
	Alloc *a;
	Btail *t;
	int i, n;
	unsigned char *q, *bq, *eq;
	unsigned char dsize;

	switch(b->magic) {
	default:
		panicblock(p, b, "bad magic");
		break;
	case FREE_MAGIC:
	case UNALLOC_MAGIC:
	 	t = B2T(b);
		if(t->magic0 != TAIL_MAGIC0 || t->magic1 != TAIL_MAGIC1)
			panicblock(p, b, "corrupt tail magic");
		if(T2HDR(t) != b)
			panicblock(p, b, "corrupt tail ptr");
		break;
	case DEAD_MAGIC:
	 	t = B2T(b);
		if(t->magic0 != TAIL_MAGIC0 || t->magic1 != TAIL_MAGIC1)
			panicblock(p, b, "corrupt tail magic");
		if(T2HDR(t) != b)
			panicblock(p, b, "corrupt tail ptr");
		n = getdsize((Alloc*)b);
		q = _B2D(b);
		q += 8;
		for(i=8; i<n; i++)
			if(*q++ != 0xDA)
				panicblock(p, b, "dangling pointer write");
		break;
	case ARENA_MAGIC:
		b = A2TB((Arena*)b);
		if(b->magic != ARENATAIL_MAGIC)
			panicblock(p, b, "bad arena size");

		break;
		/* fall through */
	case ARENATAIL_MAGIC:
		if(b->size != 0)
			panicblock(p, b, "bad arena tail size");
		break;
	case ALLOC_MAGIC:
		a = (Alloc*)b;
	 	t = B2T(b);
		dsize = getdsize(a);
		bq = (unsigned char*)_B2D(a)+dsize;
		eq = (unsigned char*)t;

		if(t->magic0 != TAIL_MAGIC0){
			/* if someone wrote exactly one byte over and it was a NUL, we sometimes only complain. */
			if((p->flags & POOL_TOLERANCE) && bq == eq && t->magic0 == 0)
				printblock(p, b, "mem user overflow (magic0)");
			else
				panicblock(p, b, "corrupt tail magic0");
		}

		if(t->magic1 != TAIL_MAGIC1)
			panicblock(p, b, "corrupt tail magic1");
		if(T2HDR(t) != b)
			panicblock(p, b, "corrupt tail ptr");

		if(dsize2bsize(p, dsize)  > a->size)
			panicblock(p, b, "too much block data");

		if(eq > bq+4)
			eq = bq+4;
		for(q=bq; q<eq; q++){
			if(*q != datamagic[((u_long)q)%nelem(datamagic)]){
				if(q == bq && *q == 0 && (p->flags & POOL_TOLERANCE)){
					printblock(p, b, "mem user overflow");
					continue;
				}
				panicblock(p, b, "mem user overflow");
			}
		}
		break;
	}
}

/*
 * compact an arena by shifting all the free blocks to the end.
 * assumes pool lock is held.
 */
enum {
	FLOATING_MAGIC = 0xCBCBCBCB,	/* temporarily neither allocated nor in the free tree */
};

static int
arenacompact(vPool *p, Arena *a)
{
	Bhdr *b, *wb, *eb, *nxt;
	int compacted;

	if(p->move == NULL)
		p->panic(p, "don't call me when pool->move is NULL\n");

	vpoolcheckarena(p, a);
	eb = A2TB(a);
	compacted = 0;
	for(b=wb=A2B(a); b && b < eb; b=nxt) {
		nxt = B2NB(b);
		switch(b->magic) {
		case FREE_MAGIC:
			vpooldel(p, (Bfree*)b);
			b->magic = FLOATING_MAGIC;
			break;
		case ALLOC_MAGIC:
			if(wb != b) {
				memmove(wb, b, b->size);
				p->move(_B2D(b), _B2D(wb));
				compacted = 1;
			}
			wb = B2NB(wb);
			break;
		}
	}

	/*
	 * the only free data is now at the end of the arena, pointed
	 * at by wb.  all we need to do is set its size and get out.
	 */
	if(wb < eb) {
		wb->magic = UNALLOC_MAGIC;
		blocksetsize(wb, (unsigned char*)eb-(unsigned char*)wb);
		vpooladd(p, (Alloc*)wb);
	}

	return compacted;
}

/*
 * compact a pool by compacting each individual arena.
 * 'twould be nice to shift blocks from one arena to the
 * next but it's a pain to code.
 */
static int
vpoolcompactl(vPool *pool)
{
	Arena *a;
	int compacted;

	if(pool->move == NULL || pool->lastcompact == pool->nfree)
		return 0;

	pool->lastcompact = pool->nfree;
	compacted = 0;
	for(a=pool->arenalist; a; a=a->down)
		compacted |= arenacompact(pool, a);
	return compacted;
}

static void*
B2D(vPool *p, Alloc *a)
{
	if(a->magic != ALLOC_MAGIC)
		p->panic(p, "B2D called on unworthy block");
	return _B2D(a);
}

static Alloc*
D2B(vPool *p, void *v)
{
	Alloc *a;
	unsigned char *u;

	if((u_long)v&(sizeof(unsigned char)-1))
		v = (char*)v - ((u_long)v&(sizeof(unsigned char)-1));
	u = v;
	while(u[-1] == ALIGN_MAGIC)
		u--;
	a = _D2B(u);
	if(a->magic != ALLOC_MAGIC)
		p->panic(p, "D2B called on non-block %p (double-free?)", v);
	return a;
}

/* poolallocl: attempt to allocate block to hold dsize user bytes; assumes lock held */
static void*
vpoolallocl(vPool *p, unsigned long dsize)
{
	unsigned long bsize;
	Bfree *fb;
	Alloc *ab;

	if(dsize >= 0x80000000UL){	/* for sanity, overflow */
		werrstr("invalid allocation size");
		return NULL;
	}

	bsize = dsize2bsize(p, dsize);

	fb = treelookupgt(p->freeroot, bsize);
	if(fb == NULL) {
		vpoolnewarena(p, bsize2asize(p, bsize));
		if((fb = treelookupgt(p->freeroot, bsize)) == NULL) {
			/* assume poolnewarena failed and set %r */
			return NULL;
		}
	}

	ab = trim(p, vpooldel(p, fb), dsize);
	p->curalloc += ab->size;
	antagonism {
		memset(B2D(p, ab), 0xDF, dsize);
	}
	return B2D(p, ab);
}

/* poolreallocl: attempt to grow v to ndsize bytes; assumes lock held */
static void*
vpoolreallocl(vPool *p, void *v, unsigned long ndsize)
{
	Alloc *a;
	Bhdr *left, *right, *newb;
	Btail *t;
	unsigned long nbsize;
	unsigned long odsize;
	unsigned long obsize;
	void *nv;

	if(v == NULL)	/* for ANSI */
		return vpoolallocl(p, ndsize);
	if(ndsize == 0) {
		vpoolfreel(p, v);
		return NULL;
	}
	a = D2B(p, v);
	blockcheck(p, a);
	odsize = getdsize(a);
	obsize = a->size;

	/* can reuse the same block? */
	nbsize = dsize2bsize(p, ndsize);
	if(nbsize <= a->size) {
	Returnblock:
		if(v != _B2D(a))
			memmove(_B2D(a), v, odsize);
		a = trim(p, a, ndsize);
		p->curalloc -= obsize;
		p->curalloc += a->size;
		v = B2D(p, a);
		return v;
	}

	/* can merge with surrounding blocks? */
	right = B2NB(a);
	if(right->magic == FREE_MAGIC && a->size+right->size >= nbsize) {
		a = blockmerge(p, a, right);
		goto Returnblock;
	}

	t = B2PT(a);
	left = T2HDR(t);
	if(left->magic == FREE_MAGIC && left->size+a->size >= nbsize) {
		a = blockmerge(p, left, a);
		goto Returnblock;
	}

	if(left->magic == FREE_MAGIC && right->magic == FREE_MAGIC
	&& left->size+a->size+right->size >= nbsize) {
		a = blockmerge(p, blockmerge(p, left, a), right);
		goto Returnblock;
	}

	if((nv = vpoolallocl(p, ndsize)) == NULL)
		return NULL;

	/* maybe the new block is next to us; if so, merge */
	left = T2HDR(B2PT(a));
	right = B2NB(a);
	newb = D2B(p, nv);
	if(left == newb || right == newb) {
		if(left == newb || left->magic == FREE_MAGIC)
			a = blockmerge(p, left, a);
		if(right == newb || right->magic == FREE_MAGIC)
			a = blockmerge(p, a, right);
		assert(a->size >= nbsize);
		goto Returnblock;
	}

	/* enough cleverness */
	memmove(nv, v, odsize);
	antagonism {
		memset((char*)nv+odsize, 0xDE, ndsize-odsize);
	}
	vpoolfreel(p, v);
	return nv;
}

static void*
alignptr(void *v, unsigned long align, long offset)
{
	char *c;
	unsigned long off;

	c = v;
	if(align){
		off = (u_long)c%align;
		if(off != offset){
			c += offset - off;
			if(off > offset)
				c += align;
		}
	}
	return c;
}

/* poolallocalignl: allocate as described below; assumes pool locked */
static void*
vpoolallocalignl(vPool *p, unsigned long dsize, unsigned long align, long offset, unsigned long span)
{
	unsigned long asize;
	void *v;
	char *c;
	unsigned long *u;
	int skip;
	Alloc *b;

	/*
	 * allocate block
	 * 	dsize bytes
	 *	addr == offset (modulo align)
	 *	does not cross span-byte block boundary
	 *
	 * to satisfy alignment, just allocate an extra
	 * align bytes and then shift appropriately.
	 *
	 * to satisfy span, try once and see if we're
	 * lucky.  the second time, allocate 2x asize
	 * so that we definitely get one not crossing
	 * the boundary.
	 */
	if(align){
		if(offset < 0)
			offset = align - ((-offset)%align);
		else
			offset %= align;
	}
	asize = dsize+align;
	v = vpoolallocl(p, asize);
	if(v == NULL)
		return NULL;
	if(span && (u_long)v/span != ((u_long)v+asize)/span){
		/* try again */
		vpoolfreel(p, v);
		v = vpoolallocl(p, 2*asize);
		if(v == NULL)
			return NULL;
	}

	/*
	 * figure out what pointer we want to return
	 */
	c = alignptr(v, align, offset);
	if(span && (u_long)c/span != (u_long)(c+dsize-1)/span){
		c += span - (u_long)c%span;
		c = alignptr(c, align, offset);
		if((u_long)c/span != (u_long)(c+dsize-1)/span){
			vpoolfreel(p, v);
			werrstr("cannot satisfy dsize %lud span %lud with align %lud+%ld", dsize, span, align, offset);
			return NULL;
		}
	}
	skip = c - (char*)v;

	/*
	 * free up the skip bytes before that pointer
	 * or mark it as unavailable.
	 */
	b = _D2B(v);
	b = freefromfront(p, b, skip);
	v = _B2D(b);
	skip = c - (char*)v;
	if(c > (char*)v){
		u = v;
		while(c >= (char*)u+sizeof(unsigned long))
			*u++ = ALIGN_MAGIC;
	}
	trim(p, b, skip+dsize);
	assert(D2B(p, c) == b);
	antagonism {
		memset(c, 0xDD, dsize);
	}
	return c;
}

/* poolfree: free block obtained from poolalloc; assumes lock held */
static void
vpoolfreel(vPool *p, void *v)
{
	Alloc *ab;
	Bhdr *back, *fwd;

	if(v == NULL)	/* for ANSI */
		return;

	ab = D2B(p, v);
	blockcheck(p, ab);

	if(p->flags&POOL_NOREUSE){
		int n;

		ab->magic = DEAD_MAGIC;
		n = getdsize(ab)-8;
		if(n > 0)
			memset((unsigned char*)v+8, 0xDA, n);
		return;
	}

	p->nfree++;
	p->curalloc -= ab->size;
	back = T2HDR(B2PT(ab));
	if(back->magic == FREE_MAGIC)
		ab = blockmerge(p, back, ab);

	fwd = B2NB(ab);
	if(fwd->magic == FREE_MAGIC)
		ab = blockmerge(p, ab, fwd);

	vpooladd(p, ab);
}

void*
vpoolalloc(vPool *p, unsigned long n)
{
	void *v;

	p->lock(p);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	v = vpoolallocl(p, n);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	if(p->logstack && (p->flags & POOL_LOGGING)) p->logstack(p);
	LOG(p, "vpoolalloc %p %lud = %p\n", p, n, v);
	p->unlock(p);
	return v;
}

void*
vpoolallocalign(vPool *p, unsigned long n, unsigned long align, long offset, unsigned long span)
{
	void *v;

	p->lock(p);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	v = vpoolallocalignl(p, n, align, offset, span);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	if(p->logstack && (p->flags & POOL_LOGGING)) p->logstack(p);
	LOG(p, "poolalignspanalloc %p %lud %lud %lud %ld = %p\n", p, n, align, span, offset, v);
	p->unlock(p);
	return v;
}

int
vpoolcompact(vPool *p)
{
	int rv;

	p->lock(p);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	rv = poolcompactl(p);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	LOG(p, "poolcompact %p\n", p);
	p->unlock(p);
	return rv;
}

void*
vpoolrealloc(vPool *p, void *v, unsigned long n)
{
	void *nv;

	p->lock(p);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	nv = poolreallocl(p, v, n);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	if(p->logstack && (p->flags & POOL_LOGGING)) p->logstack(p);
	LOG(p, "poolrealloc %p %p %ld = %p\n", p, v, n, nv);
	p->unlock(p);
	return nv;
}

void
vpoolfree(vPool *p, void *v)
{
	p->lock(p);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	vpoolfreel(p, v);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	if(p->logstack && (p->flags & POOL_LOGGING)) p->logstack(p);
	LOG(p, "poolfree %p %p\n", p, v);
	p->unlock(p);
}

/*
 * Return the real size of a block, and let the user use it.
 */
unsigned long
vpoolmsize(vPool *p, void *v)
{
	Alloc *b;
	unsigned long dsize;

	p->lock(p);
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	if(v == NULL)	/* consistency with other braindead ANSI-ness */
		dsize = 0;
	else {
		b = D2B(p, v);
		dsize = (b->size&~(p->quantum-1)) - sizeof(Bhdr) - sizeof(Btail);
		assert(dsize >= getdsize(b));
		blocksetdsize(p, b, dsize);
	}
	paranoia {
		vpoolcheckl(p);
	}
	verbosity {
		vpooldumpl(p);
	}
	if(p->logstack && (p->flags & POOL_LOGGING)) p->logstack(p);
	LOG(p, "poolmsize %p %p = %ld\n", p, v, dsize);
	p->unlock(p);
	return dsize;
}

/*
 * Debugging
 */

static void
vpoolcheckarena(vPool *p, Arena *a)
{
	Bhdr *b;
	Bhdr *atail;

	atail = A2TB(a);
	for(b=a; b->magic != ARENATAIL_MAGIC && b<atail; b=B2NB(b))
		blockcheck(p, b);
	blockcheck(p, b);
	if(b != atail)
		p->panic(p, "found wrong tail");
}

static void
vpoolcheckl(vPool *p)
{
	Arena *a;

	for(a=p->arenalist; a; a=a->down)
		vpoolcheckarena(p, a);
	if(p->freeroot)
		checktree(p->freeroot, 0, 1<<30);
}

void
vpoolcheck(vPool *p)
{
	p->lock(p);
	vpoolcheckl(p);
	p->unlock(p);
}

void
vpoolblockcheck(vPool *p, void *v)
{
	if(v == NULL)
		return;

	p->lock(p);
	blockcheck(p, D2B(p, v));
	p->unlock(p);
}

static void
vpooldumpl(vPool *p)
{
	Arena *a;

	p->print(p, "pool %p %s\n", p, p->name);
	for(a=p->arenalist; a; a=a->down)
		vpooldumparena(p, a);
}

void
vpooldump(vPool *p)
{
	p->lock(p);
	vpooldumpl(p);
	p->unlock(p);
}

static void
vpooldumparena(vPool *p, Arena *a)
{
	Bhdr *b;

	for(b=a; b->magic != ARENATAIL_MAGIC; b=B2NB(b))
		p->print(p, "(%p %.8lux %lud)", b, b->magic, b->size);
	p->print(p, "\n");
}

/*
 * mark the memory in such a way that we know who marked it
 * (via the signature) and we know where the marking started.
 */
static void
memmark(void *v, int sig, unsigned long size)
{
	unsigned char *p, *ep;
	unsigned long *lp, *elp;
	lp = v;
	elp = lp+size/4;
	while(lp < elp)
		*lp++ = (sig<<24) ^ ((u_long)lp-(u_long)v);
	p = (unsigned char*)lp;
	ep = (unsigned char*)v+size;
	while(p<ep)
		*p++ = sig;
}
