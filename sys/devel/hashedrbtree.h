/*	$NetBSD: tree.h,v 1.13 2006/08/27 22:32:38 christos Exp $	*/
/*	$OpenBSD: tree.h,v 1.7 2002/10/17 21:51:54 art Exp $	*/
/*
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
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

/*
 * A hashed red-black tree is a binary search tree with the node color and node hash as
 * extra attributes.  It fulfills a set of conditions:
 *	- every search path from the root to a leaf consists of the
 *	  same number of black nodes,
 *	- each red node (except for the root) has a black parent,
 *	- each leaf node is black.
 *	Hashing Extra Attributes:
 *	- Double hash: prospector32 & lowbias32 (refer to: "lib/libkern/prospectorhash.c")
 *	- node hash follows the same rules as node color
 *	- each black node is equivalent to a node hashed using prospector32
 *	- each red node is equivalent to a node hashed using lowbias32
 *	- the hash attribute is always inserted or removed after a color attribute
 *
 * Every operation on a red-black tree is bounded as O(lg n).
 * The maximum height of a red-black tree is 2lg (n+1).
 */

#ifndef _SYS_HASHEDRBTREE_H_
#define _SYS_HASHEDRBTREE_H_

/* Macros that define a hashed red-black tree */
#define HRB_HEAD(name, type)						                    \
struct name {								                            \
	struct type *hrbh_root; /* root of the tree */			            \
}
#define HRB_INITIALIZER(root)						                    \
	{ NULL }

#define HRB_INIT(root) do {						                        \
	(root)->hrbh_root = NULL;					                        \
} while (0)


#define HRB_BLACK	    0
#define HRB_RED		    1
#define HRB_HASH1(elm)  prospector32((unsigned long)(elm))
#define HRB_HASH2(elm)  lowbias32((unsigned long)(elm))

#define HRB_ENTRY(type)							                        \
struct {								                                \
    struct type     *hrbe_left;        /* left element */               \
    struct type     *hrbe_right;       /* right element */              \
    struct type     *hrbe_parent;      /* parent element */             \
    int              hrbe_color;       /* node color */                	\
    unsigned int     hrbe_hash;        /* node hash */                 	\
}

#define HRB_LEFT(elm, field)		(elm)->field.hrbe_left
#define HRB_RIGHT(elm, field)	    (elm)->field.hrbe_right
#define HRB_PARENT(elm, field)	    (elm)->field.hrbe_parent
#define HRB_COLOR(elm, field)	    (elm)->field.hrbe_color
#define HRB_HASH(elm, field)        (elm)->field.hrbe_hash
#define HRB_ROOT(head)		        (head)->hrbh_root
#define HRB_EMPTY(head)		        (HRB_ROOT(head) == NULL)

#define HRB_SET(elm, parent, field) do {                                \
    HRB_PARENT(elm, field) = parent;					                \
	HRB_LEFT(elm, field) = HRB_RIGHT(elm, field) = NULL;	            \
	HRB_COLOR(elm, field) = HRB_RED;					                \
	HRB_HASH(elm, field) = HRB_HASH1(elm);                              \
} while (0)

#define HRB_SET_BLACKRED(black, red, field) do {			            \
	HRB_COLOR(black, field) = HRB_BLACK;				                \
	HRB_COLOR(red, field) = HRB_RED;					                \
} while (0)

#define HRB_SET_HASH(hash1, hash2, elm, field) do {			            \
	HRB_HASH(hash1, field) = HRB_HASH1(elm);			                \
	HRB_HASH(hash2, field) = HRB_HASH2(elm);				            \
} while (0)

#ifndef HRB_AUGMENT
#define HRB_AUGMENT(x) (void)(x)
#endif

#define HRB_ROTATE_LEFT(head, elm, tmp, field) do {						\
	(tmp) = HRB_RIGHT(elm, field);										\
	if ((HRB_RIGHT(elm, field) = HRB_LEFT(tmp, field)) != NULL) {		\
		HRB_PARENT(HRB_LEFT(tmp, field), field) = (elm);				\
	}																	\
	HRB_AUGMENT(elm);													\
	if ((HRB_PARENT(tmp, field) = HRB_PARENT(elm, field)) != NULL) {	\
		if ((elm) == HRB_LEFT(HRB_PARENT(elm, field), field))			\
			HRB_LEFT(HRB_PARENT(elm, field), field) = (tmp);			\
		else															\
			HRB_RIGHT(HRB_PARENT(elm, field), field) = (tmp);			\
	} else																\
		(head)->hrbh_root = (tmp);										\
	HRB_LEFT(tmp, field) = (elm);										\
	HRB_PARENT(elm, field) = (tmp);										\
	HRB_AUGMENT(tmp);													\
	if ((HRB_PARENT(tmp, field)))										\
		HRB_AUGMENT(HRB_PARENT(tmp, field));							\
} while (0)

#define HRB_ROTATE_RIGHT(head, elm, tmp, field) do {					\
	(tmp) = HRB_LEFT(elm, field);										\
	if ((HRB_LEFT(elm, field) = HRB_RIGHT(tmp, field)) != NULL) {		\
		HRB_PARENT(HRB_RIGHT(tmp, field), field) = (elm);				\
	}																	\
	HRB_AUGMENT(elm);													\
	if ((HRB_PARENT(tmp, field) = HRB_PARENT(elm, field)) != NULL) {	\
		if ((elm) == HRB_LEFT(HRB_PARENT(elm, field), field))			\
			HRB_LEFT(HRB_PARENT(elm, field), field) = (tmp);			\
		else															\
			HRB_RIGHT(HRB_PARENT(elm, field), field) = (tmp);			\
	} else																\
		(head)->hrbh_root = (tmp);										\
	HRB_RIGHT(tmp, field) = (elm);										\
	HRB_PARENT(elm, field) = (tmp);										\
	HRB_AUGMENT(tmp);													\
	if ((HRB_PARENT(tmp, field)))										\
		HRB_AUGMENT(HRB_PARENT(tmp, field));							\
} while (0)

/* Generates prototypes and inline functions */
#define HRB_PROTOTYPE(name, type, field, cmp)							\
void name##_HRB_INSERT_COLOR(struct name *, struct type *);				\
void name##_HRB_REMOVE_COLOR(struct name *, struct type *, struct type *);\
void name##_HRB_INSERT_HASH(struct name *, struct type *);				\
void name##_HRB_REMOVE_HASH(struct name *, struct type *, struct type *);\
struct type *name##_HRB_REMOVE(struct name *, struct type *);			\
struct type *name##_HRB_INSERT(struct name *, struct type *);			\
struct type *name##_HRB_FIND(struct name *, struct type *);				\
struct type *name##_HRB_NEXT(struct type *);							\
struct type *name##_HRB_PREV(struct type *);							\
struct type *name##_HRB_MINMAX(struct name *, int);						\

/* Main rb operation.
 * Moves node close to the key of elm to top
 */
#define HRB_GENERATE(name, type, field, cmp)							\
void																	\
name##_HRB_INSERT_COLOR(struct name *head, struct type *elm)			\
{																		\
	struct type *parent, *gparent, *tmp;								\
	while ((parent = HRB_PARENT(elm, field)) != NULL &&					\
	    HRB_COLOR(parent, field) == HRB_RED) {							\
		gparent = HRB_PARENT(parent, field);							\
		if (parent == HRB_LEFT(gparent, field)) {						\
			tmp = HRB_RIGHT(gparent, field);							\
			if (tmp && HRB_COLOR(tmp, field) == HRB_RED) {				\
				HRB_COLOR(tmp, field) = HRB_BLACK;						\
				HRB_SET_BLACKRED(parent, gparent, field);				\
				elm = gparent;											\
				continue;												\
			}															\
			if (HRB_RIGHT(parent, field) == elm) {						\
				HRB_ROTATE_LEFT(head, parent, tmp, field);				\
				tmp = parent;											\
				parent = elm;											\
				elm = tmp;												\
			}															\
			HRB_SET_BLACKRED(parent, gparent, field);					\
			HRB_ROTATE_RIGHT(head, gparent, tmp, field);				\
		} else {														\
			tmp = HRB_LEFT(gparent, field);								\
			if (tmp && HRB_COLOR(tmp, field) == HRB_RED) {				\
				HRB_COLOR(tmp, field) = HRB_BLACK;						\
				HRB_SET_BLACKRED(parent, gparent, field);				\
				elm = gparent;											\
				continue;												\
			}															\
			if (HRB_LEFT(parent, field) == elm) {						\
				HRB_ROTATE_RIGHT(head, parent, tmp, field);				\
				tmp = parent;											\
				parent = elm;											\
				elm = tmp;												\
			}															\
			HRB_SET_BLACKRED(parent, gparent, field);					\
			HRB_ROTATE_LEFT(head, gparent, tmp, field);					\
		}																\
	}																	\
	HRB_COLOR(head->hrbh_root, field) = HRB_BLACK;						\
}																		\
																		\
void																	\
name##_HRB_REMOVE_COLOR(struct name *head, struct type *parent, struct type *elm) \
{																		\
	struct type *tmp;													\
	while ((elm == NULL || HRB_COLOR(elm, field) == HRB_BLACK) &&		\
	    elm != HRB_ROOT(head)) {										\
		if (HRB_LEFT(parent, field) == elm) {							\
			tmp = HRB_RIGHT(parent, field);								\
			if (HRB_COLOR(tmp, field) == HRB_RED) {						\
				HRB_SET_BLACKRED(tmp, parent, field);					\
				HRB_ROTATE_LEFT(head, parent, tmp, field);				\
				tmp = HRB_RIGHT(parent, field);							\
			}															\
			if ((HRB_LEFT(tmp, field) == NULL ||						\
			    HRB_COLOR(HRB_LEFT(tmp, field), field) == HRB_BLACK) &&	\
			    (HRB_RIGHT(tmp, field) == NULL ||						\
			    HRB_COLOR(HRB_RIGHT(tmp, field), field) == HRB_BLACK)) {\
				HRB_COLOR(tmp, field) = HRB_RED;						\
				elm = parent;											\
				parent = HRB_PARENT(elm, field);						\
			} else {													\
				if (HRB_RIGHT(tmp, field) == NULL ||					\
				    HRB_COLOR(HRB_RIGHT(tmp, field), field) == HRB_BLACK) {\
					struct type *oleft;									\
					if ((oleft = HRB_LEFT(tmp, field)) 					\
					    != NULL)										\
						HRB_COLOR(oleft, field) = HRB_BLACK;			\
					HRB_COLOR(tmp, field) = HRB_RED;					\
					HRB_ROTATE_RIGHT(head, tmp, oleft, field);			\
					tmp = HRB_RIGHT(parent, field);						\
				}														\
				HRB_COLOR(tmp, field) = HRB_COLOR(parent, field);		\
				HRB_COLOR(parent, field) = HRB_BLACK;					\
				if (HRB_RIGHT(tmp, field))								\
					HRB_COLOR(HRB_RIGHT(tmp, field), field) = HRB_BLACK;\
				HRB_ROTATE_LEFT(head, parent, tmp, field);				\
				elm = HRB_ROOT(head);									\
				break;													\
			}															\
		} else {														\
			tmp = HRB_LEFT(parent, field);								\
			if (HRB_COLOR(tmp, field) == HRB_RED) {						\
				HRB_SET_BLACKRED(tmp, parent, field);					\
				HRB_ROTATE_RIGHT(head, parent, tmp, field);				\
				tmp = HRB_LEFT(parent, field);							\
			}															\
			if ((HRB_LEFT(tmp, field) == NULL ||						\
			    HRB_COLOR(HRB_LEFT(tmp, field), field) == HRB_BLACK) &&	\
			    (HRB_RIGHT(tmp, field) == NULL ||						\
			    HRB_COLOR(HRB_RIGHT(tmp, field), field) == HRB_BLACK)) {\
				HRB_COLOR(tmp, field) = HRB_RED;						\
				elm = parent;											\
				parent = HRB_PARENT(elm, field);						\
			} else {													\
				if (HRB_LEFT(tmp, field) == NULL ||						\
				    HRB_COLOR(HRB_LEFT(tmp, field), field) == HRB_BLACK) {\
					struct type *oright;								\
					if ((oright = HRB_RIGHT(tmp, field)) 				\
					    != NULL)										\
						HRB_COLOR(oright, field) = HRB_BLACK;			\
					HRB_COLOR(tmp, field) = HRB_RED;					\
					HRB_ROTATE_LEFT(head, tmp, oright, field);			\
					tmp = HRB_LEFT(parent, field);						\
				}														\
				HRB_COLOR(tmp, field) = HRB_COLOR(parent, field);		\
				HRB_COLOR(parent, field) = HRB_BLACK;					\
				if (HRB_LEFT(tmp, field))								\
					HRB_COLOR(HRB_LEFT(tmp, field), field) = HRB_BLACK;	\
				HRB_ROTATE_RIGHT(head, parent, tmp, field);				\
				elm = HRB_ROOT(head);									\
				break;													\
			}															\
		}																\
	}																	\
	if (elm)															\
		HRB_COLOR(elm, field) = HRB_BLACK;								\
}																		\
                                                                        \
void																	\
name##_HRB_INSERT_HASH(struct name *head, struct type *elm)             \
{                                                                       \
    struct type *parent, *gparent, *tmp;                                \
    while ((parent = HRB_PARENT(elm, field)) != NULL &&                 \
        HRB_HASH(parent, field) == HRB_HASH1(elm)) {                    \
        gparent = HRB_PARENT(parent, field);							\
		if (parent == HRB_LEFT(gparent, field)) {						\
			tmp = HRB_RIGHT(gparent, field);							\
            if (tmp && HRB_HASH(tmp, field) == HRB_HASH1(elm)) {        \
                HRB_HASH(tmp, field) = HRB_HASH2(elm);                  \
                HRB_SET_HASH(parent, gparent, elm, field);              \
                elm = gparent;											\
				continue;												\
            }                                                           \
            if (HRB_RIGHT(parent, field) == elm) {						\
				HRB_ROTATE_LEFT(head, parent, tmp, field);				\
				tmp = parent;											\
				parent = elm;											\
				elm = tmp;												\
			}															\
            HRB_SET_HASH(parent, gparent, elm, field);                  \
            HRB_ROTATE_RIGHT(head, gparent, tmp, field);				\
        } else {														\
			tmp = HRB_LEFT(gparent, field);								\
            if (tmp && HRB_HASH(tmp, field) == HRB_HASH1(elm)) {		\
                HRB_HASH(tmp, field) = HRB_HASH2(elm);                  \
                 HRB_SET_HASH(parent, gparent, elm, field);             \
                elm = gparent;											\
				continue;												\
            }                                                           \
            if (HRB_LEFT(parent, field) == elm) {						\
				HRB_ROTATE_RIGHT(head, parent, tmp, field);				\
				tmp = parent;											\
				parent = elm;											\
				elm = tmp;												\
			}															\
            HRB_SET_HASH(parent, gparent, elm, field);                  \
            HRB_ROTATE_LEFT(head, gparent, tmp, field);					\
        }				                                                \
    }                                                                   \
    HRB_HASH(head->hrbh_root, field) = HRB_HASH2(elm);                  \
}                                                                       \
																		\
void																	\
name##_HRB_REMOVE_HASH(struct name *head, struct type *parent, struct type *elm) \
{																		\
	struct type *tmp;													\
	while ((elm == NULL || HRB_HASH(elm, field) == HRB_HASH2(elm)) &&	\
	    elm != HRB_ROOT(head)) {										\
		if (HRB_LEFT(parent, field) == elm) {							\
			tmp = HRB_RIGHT(parent, field);								\
			if (HRB_HASH(tmp, field) == HRB_HASH1(elm)) {				\
				HRB_SET_HASH(tmp, parent, elm, field);					\
				HRB_ROTATE_LEFT(head, parent, tmp, field);				\
				tmp = HRB_RIGHT(parent, field);							\
			}															\
			if ((HRB_LEFT(tmp, field) == NULL ||						\
			    HRB_HASH(HRB_LEFT(tmp, field), field) == HRB_HASH2(elm)) &&	\
			    (HRB_RIGHT(tmp, field) == NULL ||						\
			    HRB_HASH(HRB_RIGHT(tmp, field), field) == HRB_HASH2(elm))) {	\
				HRB_HASH(tmp, field) = HRB_HASH1(elm);					\
				elm = parent;											\
				parent = HRB_PARENT(elm, field);						\
			} else {													\
				if (HRB_RIGHT(tmp, field) == NULL ||					\
				    HRB_HASH(HRB_RIGHT(tmp, field), field) == HRB_HASH2(elm)) {\
					struct type *oleft;									\
					if ((oleft = HRB_LEFT(tmp, field)) 					\
					    != NULL)										\
						HRB_HASH(oleft, field) = HRB_HASH2(elm);		\
					HRB_HASH(tmp, field) = HRB_HASH1(elm);				\
					HRB_ROTATE_RIGHT(head, tmp, oleft, field);			\
					tmp = HRB_RIGHT(parent, field);						\
				}														\
				HRB_HASH(tmp, field) = HRB_HASH(parent, field);		    \
				HRB_HASH(parent, field) = HRB_HASH2(elm);				\
				if (HRB_RIGHT(tmp, field))								\
					HRB_HASH(HRB_RIGHT(tmp, field), field) = HRB_HASH2(elm);\
				HRB_ROTATE_LEFT(head, parent, tmp, field);				\
				elm = HRB_ROOT(head);									\
				break;													\
			}															\
		} else {														\
			tmp = HRB_LEFT(parent, field);								\
			if (HRB_HASH(tmp, field) == HRB_HASH1(elm)) {				\
				HRB_SET_HASH(tmp, parent, elm, field);					\
				HRB_ROTATE_RIGHT(head, parent, tmp, field);				\
				tmp = HRB_LEFT(parent, field);							\
			}															\
			if ((HRB_LEFT(tmp, field) == NULL ||						\
			    HRB_HASH(HRB_LEFT(tmp, field), field) == HRB_HASH2(elm)) &&	\
			    (HRB_RIGHT(tmp, field) == NULL ||						\
			    HRB_HASH(HRB_RIGHT(tmp, field), field) == HRB_HASH2(elm))) {\
				HRB_HASH(tmp, field) = HRB_HASH1(elm);					\
				elm = parent;											\
				parent = HRB_PARENT(elm, field);						\
			} else {													\
				if (HRB_LEFT(tmp, field) == NULL ||						\
				    HRB_HASH(HRB_LEFT(tmp, field), field) == HRB_HASH2(elm)) {\
					struct type *oright;								\
					if ((oright = HRB_RIGHT(tmp, field)) 				\
					    != NULL)										\
						HRB_HASH(oright, field) = HRB_HASH2(elm);		\
					HRB_HASH(tmp, field) = HRB_HASH1(elm);				\
					HRB_ROTATE_LEFT(head, tmp, oright, field);			\
					tmp = HRB_LEFT(parent, field);						\
				}														\
				HRB_HASH(tmp, field) = HRB_HASH(parent, field);		    \
				HRB_HASH(parent, field) = HRB_HASH2(elm);				\
				if (HRB_LEFT(tmp, field))								\
					HRB_HASH(HRB_LEFT(tmp, field), field) = HRB_HASH2(elm);\
				HRB_ROTATE_RIGHT(head, parent, tmp, field);				\
				elm = HRB_ROOT(head);									\
				break;													\
			}															\
		}																\
	}																	\
	if (elm)															\
		HRB_HASH(elm, field) = HRB_HASH2(elm);							\
}																		\
                                                                        \
struct type *															\
name##_HRB_REMOVE(struct name *head, struct type *elm)					\
{																		\
	struct type *child, *parent, *old = elm;							\
	int color;                                                          \
	unsigned int hash;                                                  \
	if (HRB_LEFT(elm, field) == NULL)									\
		child = HRB_RIGHT(elm, field);									\
	else if (HRB_RIGHT(elm, field) == NULL)								\
		child = HRB_LEFT(elm, field);									\
	else {																\
		struct type *left;												\
		elm = HRB_RIGHT(elm, field);									\
		while ((left = HRB_LEFT(elm, field)) != NULL)					\
			elm = left;													\
		child = HRB_RIGHT(elm, field);									\
		parent = HRB_PARENT(elm, field);								\
		color = HRB_COLOR(elm, field);									\
		hash = HRB_HASH(elm, field);                                    \
		if (child)														\
			HRB_PARENT(child, field) = parent;							\
		if (parent) {													\
			if (HRB_LEFT(parent, field) == elm)							\
				HRB_LEFT(parent, field) = child;						\
			else														\
				HRB_RIGHT(parent, field) = child;						\
			HRB_AUGMENT(parent);										\
		} else															\
			HRB_ROOT(head) = child;										\
		if (HRB_PARENT(elm, field) == old)								\
			parent = elm;												\
		(elm)->field = (old)->field;									\
		if (HRB_PARENT(old, field)) {									\
			if (HRB_LEFT(HRB_PARENT(old, field), field) == old)			\
				HRB_LEFT(HRB_PARENT(old, field), field) = elm;			\
			else														\
				HRB_RIGHT(HRB_PARENT(old, field), field) = elm;			\
			HRB_AUGMENT(HRB_PARENT(old, field));						\
		} else															\
			HRB_ROOT(head) = elm;										\
		HRB_PARENT(HRB_LEFT(old, field), field) = elm;					\
		if (HRB_RIGHT(old, field))										\
			HRB_PARENT(HRB_RIGHT(old, field), field) = elm;				\
		if (parent) {													\
			left = parent;												\
			do {														\
				HRB_AUGMENT(left);										\
			} while ((left = HRB_PARENT(left, field)) != NULL); 		\
		}																\
		goto color;														\
	}																	\
	parent = HRB_PARENT(elm, field);									\
	color = HRB_COLOR(elm, field);										\
	hash = HRB_HASH(elm, field);                                        \
	if (child)															\
		HRB_PARENT(child, field) = parent;								\
	if (parent) {														\
		if (HRB_LEFT(parent, field) == elm)								\
			HRB_LEFT(parent, field) = child;							\
		else															\
			HRB_RIGHT(parent, field) = child;							\
		HRB_AUGMENT(parent);											\
	} else																\
		HRB_ROOT(head) = child;											\
color:																	\
	if (color == HRB_BLACK)					                            \
		name##_HRB_REMOVE_COLOR(head, parent, child);					\
	else if (hash == HRB_HASH2(elm))                                    \
	    name##_HRB_REMOVE_HASH(head, parent, child);                    \
	return (old);														\
}																		\
																		\
/* Inserts a node into the RB tree */									\
struct type *															\
name##_HRB_INSERT(struct name *head, struct type *elm)					\
{																		\
	struct type *tmp;													\
	struct type *parent = NULL;											\
	int comp = 0;														\
	tmp = HRB_ROOT(head);												\
	while (tmp) {														\
		parent = tmp;													\
		comp = (cmp)(elm, parent);										\
		if (comp < 0)													\
			tmp = HRB_LEFT(tmp, field);									\
		else if (comp > 0)												\
			tmp = HRB_RIGHT(tmp, field);								\
		else															\
			return (tmp);												\
	}																	\
	HRB_SET(elm, parent, field);										\
	if (parent != NULL) {												\
		if (comp < 0)													\
			HRB_LEFT(parent, field) = elm;								\
		else															\
			HRB_RIGHT(parent, field) = elm;								\
		HRB_AUGMENT(parent);											\
	} else																\
		HRB_ROOT(head) = elm;											\
	name##_HRB_INSERT_COLOR(head, elm);									\
	name##_HRB_INSERT_HASH(head, elm);									\
	return (NULL);														\
}																		\
																		\
/* Finds the node with the same key as elm */							\
struct type *															\
name##_HRB_FIND(struct name *head, struct type *elm)					\
{																		\
	struct type *tmp = HRB_ROOT(head);									\
	int comp;															\
	while (tmp) {														\
		comp = cmp(elm, tmp);											\
		if (comp < 0)													\
			tmp = HRB_LEFT(tmp, field);									\
		else if (comp > 0)												\
			tmp = HRB_RIGHT(tmp, field);								\
		else															\
			return (tmp);												\
	}																	\
	return (NULL);														\
}																		\
																		\
/* ARGSUSED */															\
struct type *															\
name##_HRB_NEXT(struct type *elm)										\
{																		\
	if (HRB_RIGHT(elm, field)) {										\
		elm = HRB_RIGHT(elm, field);									\
		while (HRB_LEFT(elm, field))									\
			elm = HRB_LEFT(elm, field);									\
	} else {															\
		if (HRB_PARENT(elm, field) &&									\
		    (elm == HRB_LEFT(HRB_PARENT(elm, field), field)))			\
			elm = HRB_PARENT(elm, field);								\
		else {															\
			while (HRB_PARENT(elm, field) &&							\
			    (elm == HRB_RIGHT(HRB_PARENT(elm, field), field)))		\
				elm = HRB_PARENT(elm, field);							\
			elm = HRB_PARENT(elm, field);								\
		}																\
	}																	\
	return (elm);														\
}																		\
																		\
struct type *															\
name##_HRB_PREV(struct type *elm)										\
{																		\
	if (HRB_LEFT(elm, field)) {											\
		elm = HRB_LEFT(elm, field);										\
		while (HRB_RIGHT(elm, field))									\
			elm = HRB_RIGHT(elm, field);								\
	} else {															\
		if (HRB_PARENT(elm, field) &&									\
				(elm == HRB_RIGHT(HRB_PARENT(elm, field), field)))		\
			elm = HRB_PARENT(elm, field);								\
		else {															\
			while (HRB_PARENT(elm, field) &&							\
					(elm == HRB_LEFT(HRB_PARENT(elm, field), field)))	\
				elm = HRB_PARENT(elm, field);							\
			elm = HRB_PARENT(elm, field);								\
		}																\
	}																	\
	return (elm);														\
}																		\
																		\
struct type *															\
name##_HRB_MINMAX(struct name *head, int val)							\
{																		\
	struct type *tmp = HRB_ROOT(head);									\
	struct type *parent = NULL;											\
	while (tmp) {														\
		parent = tmp;													\
		if (val < 0)													\
			tmp = HRB_LEFT(tmp, field);									\
		else															\
			tmp = HRB_RIGHT(tmp, field);								\
	}																	\
	return (parent);													\
}

#define HRB_NEGINF	-1
#define HRB_INF		1

#define HRB_INSERT(name, x, y)	    name##_HRB_INSERT(x, y)
#define HRB_REMOVE(name, x, y)	    name##_HRB_REMOVE(x, y)
#define HRB_FIND(name, x, y)		name##_HRB_FIND(x, y)
#define HRB_NEXT(name, x, y)		name##_HRB_NEXT(y)
#define HRB_PREV(name, x, y)	    name##_HRB_PREV(y)
#define HRB_MIN(name, x)			name##_HRB_MINMAX(x, HRB_NEGINF)
#define HRB_MAX(name, x)			name##_HRB_MINMAX(x, HRB_INF)

#define HRB_FOREACH(x, name, head)										\
	for ((x) = HRB_MIN(name, head);										\
	     (x) != NULL;													\
	     (x) = name##_HRB_NEXT(x))

#define HRB_FOREACH_SAFE(x, name, head, y)				        		\
	for ((x) = HRB_MIN(name, head);					            		\
	    ((x) != NULL) && ((y) = name##_HRB_NEXT(x), 1);		    		\
	     (x) = (y))

#define HRB_FOREACH_REVERSE(x, name, head)				        		\
	for ((x) = HRB_MAX(name, head);					            		\
	     (x) != NULL;						                    		\
	     (x) = name##_HRB_PREV(x))

#define HRB_FOREACH_REVERSE_SAFE(x, name, head, y)			    		\
	for ((x) = HRB_MAX(name, head);					            		\
	    ((x) != NULL) && ((y) = name##_HRB_PREV(x), 1);		    		\
	     (x) = (y))

#endif /* _SYS_HASHEDRBTREE_H_ */
