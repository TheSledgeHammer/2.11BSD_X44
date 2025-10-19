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

#ifndef _LIBC_H_
#define _LIBC_H_ 1

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * compiler directive on Plan 9
 */
#ifndef USED
#define USED(x) if(x); else
#endif

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

#if defined(__cplusplus)
}
#endif
#endif	/* _LIBC_H_ */
