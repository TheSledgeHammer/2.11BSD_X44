/*
 * temp.h
 *
 *  Created on: 4 Dec 2021
 *      Author: marti
 */

#ifndef SYS_DEVEL_ARCH_I386_INCLUDE_TEMP_H_
#define SYS_DEVEL_ARCH_I386_INCLUDE_TEMP_H_


/*
 *	u_intN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define	bus_space_read_1(t, h, o)									\
	((t) == I386_BUS_SPACE_IO ? (inb((h) + (o))) :\
	    (*(volatile u_int8_t *)((h) + (o))))

#define	bus_space_read_2(t, h, o)									\
	 (__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr"),	\
	  ((t) == I386_BUS_SPACE_IO ? (inw((h) + (o))) :		\
	    (*(volatile u_int16_t *)((h) + (o)))))

#define	bus_space_read_4(t, h, o)									\
	 (__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr"),	\
	  ((t) == I386_BUS_SPACE_IO ? (inl((h) + (o))) :		\
	    (*(volatile u_int32_t *)((h) + (o)))))

#define bus_space_read_stream_1 bus_space_read_1
#define bus_space_read_stream_2 bus_space_read_2
#define bus_space_read_stream_4 bus_space_read_4

#if 0	/* Cause a link error for bus_space_read_8 */
#define	bus_space_read_8(t, h, o)	!!! bus_space_read_8 unimplemented !!!
#define	bus_space_read_stream_8(t, h, o)							\
		!!! bus_space_read_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define	bus_space_read_multi_1(t, h, o, ptr, cnt)					\
do {																\
	if ((t) == I386_BUS_SPACE_IO) {									\
		insb((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	movb (%2),%%al										;	\
			stosb												;	\
			loop 1b"											: 	\
		    "=D" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o))       	:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_multi_2(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		insw((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	movw (%2),%%ax										;	\
			stosw												;	\
			loop 1b"											:	\
		    "=D" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o))       	:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_multi_4(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		insl((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	movl (%2),%%eax										;	\
			stosl												;	\
			loop 1b"											:	\
		    "=D" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o))       :       \
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_read_multi_stream_1 bus_space_read_multi_1
#define bus_space_read_multi_stream_2 bus_space_read_multi_2
#define bus_space_read_multi_stream_4 bus_space_read_multi_4

#if 0	/* Cause a link error for bus_space_read_multi_8 */
#define	bus_space_read_multi_8	!!! bus_space_read_multi_8 unimplemented !!!
#define	bus_space_read_multi_stream_8								\
		!!! bus_space_read_multi_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

#define	bus_space_read_region_1(t, h, o, ptr, cnt)					\
do {																\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	inb %w1,%%al										;	\
			stosb												;	\
			incl %1												;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=D" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsb"												:	\
		    "=S" (dummy1), "=D" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_region_2(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	inw %w1,%%ax										;	\
			stosw												;	\
			addl $2,%1											;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=D" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsw"												:	\
		    "=S" (dummy1), "=D" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_region_4(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	inl %w1,%%eax										;	\
			stosl												;	\
			addl $4,%1											;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=D" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsl"												:	\
		    "=S" (dummy1), "=D" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_read_region_stream_1 bus_space_read_region_1
#define bus_space_read_region_stream_2 bus_space_read_region_2
#define bus_space_read_region_stream_4 bus_space_read_region_4

#if 0	/* Cause a link error for bus_space_read_region_8 */
#define	bus_space_read_region_8	!!! bus_space_read_region_8 unimplemented !!!
#define	bus_space_read_region_stream_8								\
		!!! bus_space_read_region_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define	bus_space_write_1(t, h, o, v)								\
do {																\
	if ((t) == I386_BUS_SPACE_IO)									\
		outb((h) + (o), (v));										\
	else															\
		((void)(*(volatile u_int8_t *)((h) + (o)) = (v)));			\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_2(t, h, o, v)								\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO)									\
		outw((h) + (o), (v));										\
	else															\
		((void)(*(volatile u_int16_t *)((h) + (o)) = (v)));			\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_4(t, h, o, v)								\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO)									\
		outl((h) + (o), (v));										\
	else															\
		((void)(*(volatile u_int32_t *)((h) + (o)) = (v)));			\
} while (/* CONSTCOND */ 0)

#define bus_space_write_stream_1 bus_space_write_1
#define bus_space_write_stream_2 bus_space_write_2
#define bus_space_write_stream_4 bus_space_write_4

#if 0	/* Cause a link error for bus_space_write_8 */
#define	bus_space_write_8	!!! bus_space_write_8 not implemented !!!
#define	bus_space_write_stream_8									\
		!!! bus_space_write_stream_8 not implemented !!!
#endif

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define	bus_space_write_multi_1(t, h, o, ptr, cnt)					\
do {																\
	if ((t) == I386_BUS_SPACE_IO) {									\
		outsb((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsb												;	\
			movb %%al,(%2)										;	\
			loop 1b"											: 	\
		    "=S" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)));				\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_write_multi_2(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		outsw((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsw												;	\
			movw %%ax,(%2)										;	\
			loop 1b"											: 	\
		    "=S" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)));				\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_write_multi_4(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		outsl((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsl												;	\
			movl %%eax,(%2)										;	\
			loop 1b"											: 	\
		    "=S" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x)  : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)));				\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_write_multi_stream_1 bus_space_write_multi_1
#define bus_space_write_multi_stream_2 bus_space_write_multi_2
#define bus_space_write_multi_stream_4 bus_space_write_multi_4

#if 0	/* Cause a link error for bus_space_write_multi_8 */
#define	bus_space_write_multi_8(t, h, o, ptr, cnt)					\
			!!! bus_space_write_multi_8 unimplemented !!!
#define	bus_space_write_multi_stream_8(t, h, o, ptr, cnt)			\
			!!! bus_space_write_multi_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

#define	bus_space_write_region_1(t, h, o, ptr, cnt)					\
do {																\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsb												;	\
			outb %%al,%w1										;	\
			incl %1												;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=S" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsb"												:	\
		    "=D" (dummy1), "=S" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_region_2(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsw												;	\
			outw %%ax,%w1										;	\
			addl $2,%1											;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=S" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsw"												:	\
		    "=D" (dummy1), "=S" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)


#define	bus_space_write_region_4(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsl												;	\
			outl %%eax,%w1										;	\
			addl $4,%1											;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=S" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsl"												:	\
		    "=D" (dummy1), "=S" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_write_region_stream_1 bus_space_write_region_1
#define bus_space_write_region_stream_2 bus_space_write_region_2
#define bus_space_write_region_stream_4 bus_space_write_region_4

#if 0	/* Cause a link error for bus_space_write_region_8 */
#define	bus_space_write_region_8									\
			!!! bus_space_write_region_8 unimplemented !!!
#define	bus_space_write_region_stream_8								\
			!!! bus_space_write_region_stream_8 unimplemented !!!
#endif


#endif /* SYS_DEVEL_ARCH_I386_INCLUDE_TEMP_H_ */
