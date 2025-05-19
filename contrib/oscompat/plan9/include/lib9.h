/*
 * Copyright © 2021 Plan 9 Foundation
 * Portions Copyright © 2001-2008 Russ Cox
 * Portions Copyright © 2008-2009 Google Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _LIB9_H_
#define _LIB9_H_ 1

#if defined(__cplusplus)
extern "C" {
#endif

/*
#include <inttypes.h>

#include <utf.h>
#include <fmt.h>

#include <fcntl.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
*/

/*
 * compiler directive on Plan 9
 */
#ifndef USED
#define USED(x) if(x); else
#endif

/*
 * Plan 9 Type definitions
 * easiest way to make sure these are defined
 */
typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int    uint;
typedef unsigned long	ulong;
typedef unsigned long long uvlong;
typedef long long 	    vlong;
typedef uintptr_t 	    uintptr;

typedef uvlong 		    u64int;
typedef vlong 		    s64int;
typedef uint8_t 	    u8int;
typedef int8_t 		    s8int;
typedef uint16_t 	    u16int;
typedef int16_t 	    s16int;
typedef uintptr_t 	    uintptr;
typedef intptr_t 	    intptr;
typedef uint 		    u32int;
typedef int 		    s32int;

typedef u32int 		    uint32;
typedef s32int 		    int32;
typedef u16int 		    uint16;
typedef s16int 		    int16;
typedef u64int 		    uint64;
typedef s64int 		    int64;
typedef u8int 		    uint8;
typedef s8int 		    int8;

/*
 * nil cannot be ((void*)0) on ANSI C,
 * because it is used for function pointers
 */
#ifndef	nil
#define nil 		0
#endif

#ifndef	nelem
#define	nelem(x)	(sizeof (x)/sizeof (x)[0])
#endif

#define OREAD		O_RDONLY
#define OWRITE		O_WRONLY

#define	OCEXEC 		0
#define	ORCLOSE		0
#define	OTRUNC		0

#define seek(fd, offset, whence) lseek(fd, offset, whence)
#define create(name, mode, perm) creat(name, perm)
#define	exits(x)		 exit(x && *x ? 1 : 0)

#if defined(__cplusplus)
}
#endif
#endif	/* _LIB9_H_ */
