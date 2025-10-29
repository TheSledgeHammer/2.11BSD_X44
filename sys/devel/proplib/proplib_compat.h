/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 */

#ifndef _PROPLIB_COMPAT_H_
#define _PROPLIB_COMPAT_H_

/* prop type names */
#define PROP_NAME_BOOL					"PROP_BOOL"
#define PROP_NAME_NUMBER				"PROP_NUMBER"
#define	PROP_NAME_STRING				"PROP_STRING"
#define	PROP_NAME_DATA					"PROP_DATA"
#define PROP_NAME_DICTIONARY			"PROP_DICTIONARY"
#define PROP_NAME_DICT_KEYSYM			"PROP_DICT_KEYSYM"
#define PROP_NAME_SYMBOL				"PROP_SYMBOL"
#define PROP_NAME_ARRAY					"PROP_ARRAY"

/* prop types */
#define PROP_TYPE_UNKNOWN				PROP_UNKNOWN
#define	PROP_TYPE_BOOL					PROP_BOOL
#define	PROP_TYPE_NUMBER				PROP_NUMBER
#define	PROP_TYPE_STRING				PROP_STRING
#define	PROP_TYPE_DATA					PROP_DATA
#define PROP_TYPE_ARRAY					PROP_ARRAY
#define PROP_TYPE_DICTIONARY			0x070000000
#define PROP_TYPE_DICT_KEYSYM			0x080000000
#define PROP_TYPE_SYMBOL				0x090000000

/* prop tags */
#define PROP_TAG_TYPE_START				0
#define PROP_TAG_TYPE_END				1
#define PROP_TAG_TYPE_EITHER			2

/* prop malloc types */
#define M_PROP_DATA						M_PROP
#define M_PROP_DICT						M_PROP
#define M_PROP_STRING					M_PROP
#define M_PROP_ARRAY					M_PROP

#define	_PROP_ASSERT(x)					KASSERT(x)

#define	_PROP_MALLOC(s, t)				malloc((s), (t), M_WAITOK)
#define	_PROP_CALLOC(n, s, t)			calloc((n), (s), (t), M_WAITOK | M_ZERO)
#define	_PROP_REALLOC(v, s, t)			realloc((v), (s), (t), M_WAITOK)
#define	_PROP_FREE(v, t)				free((v), (t))

#define	_PROP_POOL_GET(p, t)			_PROP_MALLOC((p), (t), M_WAITOK)
#define	_PROP_POOL_PUT(p, v)			_PROP_FREE((p), (v))

#define	_PROP_RWLOCK_RDLOCK(x)			//rw_enter(&(x), RW_READER)
#define	_PROP_RWLOCK_WRLOCK(x)			//rw_enter(&(x), RW_WRITER)
#define	_PROP_RWLOCK_UNLOCK(x)			//rw_exit(&(x))

typedef void 							*prop_object_t;
typedef struct prop_array				*prop_array_t;
typedef struct prop_bool				*prop_bool_t;
typedef struct prop_number				*prop_number_t;
typedef struct prop_string				*prop_string_t;
typedef struct prop_data				*prop_data_t;
typedef struct prop_dictionary 			*prop_dictionary_t;
typedef struct prop_dictionary_keysym 	*prop_dictionary_keysym_t;

struct prop_number {
	propdb_t				pn_db;
	struct prop_object		pn_obj;
	struct rb_node			pn_link;
	struct prop_number_value {
		union {
			int64_t  		pnu_signed;
			uint64_t 		pnu_unsigned;
		} pnv_un;
#define	pnv_signed			pnv_un.pnu_signed
#define	pnv_unsigned		pnv_un.pnu_unsigned
		unsigned int		pnv_is_unsigned	:1, :31;
	} pn_value;
};

struct prop_string {
	propdb_t				ps_db;
	struct prop_object		ps_obj;
	union {
		char 				*psu_mutable;
		const char 			*psu_immutable;
	} ps_un;
#define	ps_mutable			ps_un.psu_mutable
#define	ps_immutable		ps_un.psu_immutable
	size_t					ps_size;	/* not including \0 */
	struct rb_node			ps_link;
	int						ps_flags;
};

struct prop_data {
	propdb_t				pd_db;
	struct prop_object		pd_obj;
	union {
		void 				*pdu_mutable;
		const void 			*pdu_immutable;
	} pd_un;
#define	pd_mutable			pd_un.pdu_mutable
#define	pd_immutable		pd_un.pdu_immutable
	size_t					pd_size;
	int						pd_flags;
};

struct prop_dictionary_keysym {
	propdb_t				pdk_db;
	struct prop_object		pdk_obj;
	size_t					pdk_size;
	struct rb_node			pdk_link;
	char 					pdk_key[1];
};

struct prop_dict_entry {
	prop_dictionary_keysym_t pde_key;
	opaque_t			 	 pde_objref;
};

struct prop_dictionary {
	propdb_t				pd_db;
	struct prop_object		pd_obj;
	struct prop_dict_entry 	*pd_array;
	u_int					pd_capacity;
	u_int					pd_count;
	int						pd_flags;

	u_int32_t				pd_version;
};

struct prop_symbol {
	propdb_t				ps_db;
	struct prop_object		ps_obj;
};

#endif /* _PROPLIB_COMPAT_H_ */
