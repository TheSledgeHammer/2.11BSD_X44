#	$NetBSD: Makefile,v 1.146 2019/06/04 15:07:55 hannken Exp $

#	@(#)Makefile	5.45.1.1 (Berkeley) 5/6/91

# Doing a make install builds /usr/include
#
# The ``rm -rf''s used below are safe because rm doesn't follow symbolic
# links.
# Doing a make includes builds /usr/include

NOOBJ=		# defined

INCS=		a.out.h ar.h assert.h atomic.h bitstring.h complex.h cpio.h ctype.h \
			db.h dirent.h disktab.h dlfcn.h encoding.h endian.h err.h errlst.h  \
			errno.h fmtmsg.h fnmatch.h fstab.h fts.h ftw.h getopt.h glob.h      \
			grp.h ident.h inttypes.h iso646.h kenv.h kvm.h langinfo.h 			\
			lastlog.h libgen.h limits.h link_aout.h link_elf.h 					\
			link.h locale.h login_cap.h math.h memory.h mpool.h mtab.h 			\
			nl_types.h ndbm.h netdb.h nlist.h paths.h pwd.h ranlib.h regex.c 	\
			regexp.h resolv.h rune.h runetype.h setjmp.h sgtty.h signal.h 		\
			stab.h stdio.h stdlib.h string.h stringlist.h strings.h struct.h 	\
			sysexits.h tar.h time.h ttyent.h tzfile.h ulimit.h unistd.h util.h 	\
			utime.h utmp.h uuid.h varargs.h vfont.h vis.h vmf.h wchar.h wctype.h
INCS+=		arpa/ftp.h arpa/inet.h arpa/nameser.h arpa/telnet.h arpa/tftp.h
INCS+=		hash/md2.h hash/md4.h hash/md5.h hash/rmd160.h hash/sha1.h 			\
			hash/sha2.h hash/sha3.h
INCS+=		OLD/dk.h OLD/psout.h
INCS+=		protocols/dumprestore.h protocols/routed.h protocols/rwhod.h 		\
			protocols/talkd.h protocols/timed.h
INCS+=		hesiod

.include <bsd.own.mk>

INCS+=  	ssp/ssp.h ssp/stdio.h ssp/string.h ssp/strings.h ssp/unistd.h

INCSDIR=  	/usr/include

SUBDIR=		rpc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
