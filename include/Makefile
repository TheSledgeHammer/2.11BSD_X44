#	$NetBSD: Makefile,v 1.146 2019/06/04 15:07:55 hannken Exp $

#	@(#)Makefile	5.45.1.1 (Berkeley) 5/6/91

# Doing a make install builds /usr/include
#
# The ``rm -rf''s used below are safe because rm doesn't follow symbolic
# links.
# Doing a make includes builds /usr/include

NOOBJ=		# defined

INCS=	a.out.h ar.h assert.h atomic.h bitstring.h complex.h cpio.h ctype.h \
		db.h dirent.h dlfcn.h encoding.h endian.h err.h errlst.h  			\
		errno.h fmtmsg.h fnmatch.h fstab.h fts.h ftw.h getopt.h glob.h      \
		grp.h ident.h inttypes.h iso646.h kenv.h kvm.h langinfo.h 			\
		lastlog.h libgen.h limits.h link_aout.h link_elf.h 					\
		link.h locale.h login_cap.h math.h memory.h mp.h mpool.h mtab.h 	\
		nl_types.h ndbm.h netdb.h nlist.h paths.h pwd.h ranlib.h 			\
		regex.c regexp.h resolv.h rune.h runetype.h setjmp.h sgtty.h 		\
		signal.h stab.h stdio.h stdlib.h string.h stringlist.h strings.h 	\
		struct.h sysexits.h tar.h time.h ttyent.h tzfile.h ucontext.h 		\
		ulimit.h unistd.h util.h utime.h utmp.h uuid.h varargs.h vfont.h 	\
		vis.h vmf.h wchar.h wctype.h

.include "arpa/Makefile.inc"
.include "hash/Makefile.inc"
.include "OLD/Makefile.inc"
.include "protocols/Makefile.inc"

INCS+=	hesiod.h

.include <bsd.own.mk>

.include "ssp/Makefile.inc"

INCSDIR= /usr/include

SUBDIR=	 rpc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
