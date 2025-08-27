#	$NetBSD: Makefile,v 1.85 2019/10/13 07:28:11 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such redistribution and
# use acknowledge that the software was developed by the University
# of California, Berkeley.  The name of the University may not be
# used to endorse or promote products derived from this software
# without specific prior written permission.  THIS SOFTWARE IS PROVIDED
# ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.
#
# @(#)Makefile	5.4.1 (2.11BSD GTE) 1995/04/13
#

USE_SHLIBDIR=	yes

.include <bsd.own.mk>

#WARNS=5
# Stops GCC from complaining about format-nonliteral (reason: snprintb.c)
WARNS?=	3
LIB=	util
CPPFLAGS+=-DLIBC_SCCS -I${.CURDIR}
SRCS+=	efun.c 														\
		getbootfile.c getmaxpartitions.c getrawpartition.c 			\
        if_media.c                                                  \
		login.c loginx.c login_cap.c login_tty.c logout.c logoutx.c \
		logwtmp.c logwtmpx.c parsedate.y pidfile.c pidlock.c pty.c 	\
		raise_default_signal.c secure_path.c snprintb.c 			\
		sockaddr_snprintf.c stat_flags.c ttyaction.c ttymsg.c

MAN=	efun.3 														\
		getbootfile.3 getmaxpartitions.3 getrawpartition.3 			\
		login.3 login_cap.3 loginx.3 parsedate.3 pidfile.3          \
        pidlock.3 raise_default_signal.3 secure_path.3 snprintb.3 	\
		sockaddr_snprintf.3 stat_flags.3 ttyaction.3 ttymsg.3

CPPFLAGS.sockaddr_snprintf.c+=-DHAVE_UTIL_H
#CPPFLAGS.sockaddr_snprintf.c+=-DHAVE_NETATALK_AT_H
CPPFLAGS.sockaddr_snprintf.c+=-DHAVE_NET_IF_DL_H

.PATH:	${NETBSDSRCDIR}/lib/libc/gen

MLINKS+=login.3 logout.3
MLINKS+=login.3 logwtmp.3
MLINKS+=login_cap.3 login_getclass.3
MLINKS+=login_cap.3 login_getcapbool.3
MLINKS+=login_cap.3 login_getcapnum.3
MLINKS+=login_cap.3 login_getcapsize.3
MLINKS+=login_cap.3 login_getcapstr.3
MLINKS+=login_cap.3 login_getcaptime.3
MLINKS+=login_cap.3 login_getpwclass.3
MLINKS+=login_cap.3 login_close.3
MLINKS+=login_cap.3 setclasscontext.3
MLINKS+=login_cap.3 setusercontext.3
MLINKS+=loginx.3 logoutx.3 loginx.3 logwtmpx.3
MLINKS+=efun.3 esetfunc.3
MLINKS+=efun.3 easprintf.3
MLINKS+=efun.3 estrlcpy.3
MLINKS+=efun.3 estrlcat.3
MLINKS+=efun.3 estrdup.3
MLINKS+=efun.3 estrndup.3
MLINKS+=efun.3 estrtoi.3
MLINKS+=efun.3 estrtou.3
MLINKS+=efun.3 emalloc.3
MLINKS+=efun.3 ecalloc.3
MLINKS+=efun.3 erealloc.3
MLINKS+=efun.3 ereallocarr.3
MLINKS+=efun.3 efopen.3
MLINKS+=efun.3 evasprintf.3
MLINKS+=stat_flags.3 string_to_flags.3
MLINKS+=stat_flags.3 flags_to_string.3

.include <bsd.lib.mk>

COPTS.logwtmp.c+=	${GCC_NO_STRINGOP_TRUNCATION} ${GCC_NO_STRINGOP_OVERFLOW}
COPTS.logwtmpx.c+=	${GCC_NO_STRINGOP_TRUNCATION} ${GCC_NO_STRINGOP_OVERFLOW}
