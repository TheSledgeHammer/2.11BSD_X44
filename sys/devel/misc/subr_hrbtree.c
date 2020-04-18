/*
 * subr_hrbtree.c
 *
 *  Created on: 18 Apr 2020
 *      Author: marti
 */

#include <hrbtree.h>
struct hrbh hrbh_root_base[NBUCKETS];
struct hrbe hrbe_entry_base[NBUCKETS];

struct hrbh *
hrbh_root(head)
	struct hrbh *head;
{
	struct hrbh *root = RB_ROOT(&head->hrbh_root);
	return (root);
}

struct hrbe *
hrbe_entry(type)
	struct hrbe *type;
{
	return ((type)->hrbe_entry);
}

struct hrbe *
hrbe_left(entry)
	struct hrbe *entry;
{
	struct hrbe left = RB_LEFT(entry, hrbe_entry);
	return (left);
}

struct hrbe *
hrbe_right(entry)
	struct hrbe *entry;
{
	struct hrbe right = RB_RIGHT(entry, hrbe_entry);
	return (right);
}

struct hrbe *
hrbe_parent(entry)
	struct hrbe *entry;
{
	struct hrbe parent = RB_PARENT(entry, hrbe_entry);
	return (parent);
}

int
hrbe_color(entry)
	struct hrbe *entry;
{
	int color = RB_COLOR(entry, hrbe_entry);
	return (color);
}

void
hrbe_hash(entry, elm)
	struct hrbe *entry;
	uint32_t elm;
{
	entry->hrbe_hash = hrbt_hash(elm);
}

u_long
hrbt_hash(elm)
	uint32_t elm;
{
	unsigned long hash = prospector32(elm);
	return (hash);
}

int
hrb_hash_compare(a, b)
	struct hrbe a, b;
{
	if(a->hrbe_hash < b->hrbe_hash) {
		return (-1);
	} else if(a->hrbe_hash > b->hrbe_hash) {
		return (1);
	}
	return (0);
}

hrbt_insert(entry, elm)
	struct hrbe *entry;
	uint32_t elm;
{
	entry = hrbe_entry_base[hrbt_hash(elm)];
	entry->hrbe_hash = hrbt_hash(elm);
}

int
main()
{
	struct hrbh t;
}
