#	$NetBSD: Makefile,v 1.59 1996/05/15 21:36:43 jtc Exp $

#	@(#)Makefile	5.45.1.1 (Berkeley) 5/6/91

# Doing a make install builds /usr/include
#
# The ``rm -rf''s used below are safe because rm doesn't follow symbolic
# links.

.include <bsd.own.mk>

FILES=	a.out.h ar.h assert.h bitstring.h cpio.h ctype.h curses.h \ 
		db.h dbm.h dirent.h disktab.h dlfcn.h err.h errlst.h \ 
		errno.h fcntl.h frame.h fstab.h fts.h glob.h grp.h \
		ident.h kenv.h kvm.h lastlog.h limits.h link_aout.h link_elf.h \ 
		link.h math.h memory.h mp.h mpool.h mtab.h ndbm.h \
		netdb.h netgroup.h nlist.h paths.h pcc.h pwd.h ranlib.h \
		regexp.h resolv.h rune.h runetype.h setjmp.h sgtty.h signal.h \
		stab.h stddef.h stdio.h stdlib.h string.h strings.h struct.h \
		sysexits.h tar.h time.h ttyent.h tzfile.h unistd.h util.h \
		utime.h utmp.h varargs.h vfont.h vmf.h

MFILES=	float.h frame.h stdarg.h varargs.h
LFILES=	endian.h errno.h fcntl.h syslog.h termios.h

DIRS=	arpa protocols rpc
LDIRS=	ddb dev fs miscfs net netimp netinet netns sys ufs vm

NOOBJ=	noobj

# Change SYS_INCLUDE in bsd.own.mk to "symlinks" if you don't want copies
SYS_INCLUDE?=	copies

includes:
		@echo installing ${FILES}
		@-for i in ${FILES}; do \
			cmp -s $$i ${DESTDIR}/usr/include/$$i || \
			    install -c -m 444 $$i ${DESTDIR}/usr/include/$$i; \
		done
		@echo installing ${DIRS}
		@-for i in ${DIRS}; do \
			install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
				${DESTDIR}/usr/include/$$i; \
			(cd $$i; for j in *.[ih]; do \
				cmp -s $$j ${DESTDIR}/usr/include/$$i/$$j || \
				install -c -m 444 $$j ${DESTDIR}/usr/include/$$i/$$j; \
			done); \
		done
		@echo installing ${LFILES}
		@-for i in ${LFILES}; do \
			rm -f ${DESTDIR}/usr/include/$$i; \
			ln -s sys/$$i ${DESTDIR}/usr/include/$$i; \
		done
		@echo installing ${MFILES}
		@-for i in ${MFILES}; do \
			rm -f ${DESTDIR}/usr/include/$$i; \
			ln -s machine/$$i ${DESTDIR}/usr/include/$$i; \
		done
		chown -R ${BINOWN}:${BINGRP} ${DESTDIR}/usr/include
		find ${DESTDIR}/usr/include -type f | \
			xargs chmod a=r
		find ${DESTDIR}/usr/include -type d | \
			xargs chmod u=rwx,go=rx

copies:
		@echo copies: ${LDIRS}
		@-for i in ${LDIRS}; do \
			rm -rf ${DESTDIR}/usr/include/$$i; \
			install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
				${DESTDIR}/usr/include/$$i ; \
		done
		cd ../sys; \
		pax -rw -pa -L \
		    `find ${LDIRS} -follow -type f -name '*.h' '!' -path \
		    'netiso/xebec/*' -print` ${DESTDIR}/usr/include
		rm -rf ${DESTDIR}/usr/include/machine
		install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
			${DESTDIR}/usr/include/machine
		pax -rw -pa -s "|../sys/arch/${MACHINE}/include||" \
		    ../sys/arch/${MACHINE}/include/*.h \
		    ${DESTDIR}/usr/include/machine
		rm -rf ${DESTDIR}/usr/include/${MACHINE_ARCH}
		if test ${MACHINE} != ${MACHINE_ARCH} -a \
		    -d ../sys/arch/${MACHINE_ARCH}/include; then \
			install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
	    	    	    ${DESTDIR}/usr/include/${MACHINE_ARCH}; \
			pax -rw -pa -s "|../sys/arch/${MACHINE_ARCH}/include||" \
			    ../sys/arch/${MACHINE_ARCH}/include/*.h \
			    ${DESTDIR}/usr/include/${MACHINE_ARCH}; \
		else \
			ln -s machine ${DESTDIR}/usr/include/${MACHINE_ARCH}; \
		fi

symlinks:
		@echo symlinks: ${LDIRS}
		@for i in ${LDIRS}; do \
			rm -rf ${DESTDIR}/usr/include/$$i; \
			ln -s /sys/$$i ${DESTDIR}/usr/include/$$i; \
		done
		rm -rf ${DESTDIR}/usr/include/machine
		ln -s /sys/arch/${MACHINE}/include ${DESTDIR}/usr/include/machine
		rm -rf ${DESTDIR}/usr/include/${MACHINE_ARCH}
		if test ${MACHINE} != ${MACHINE_ARCH} -a \
		    -d ../sys/arch/${MACHINE_ARCH}/include ; then \
			ln -s /sys/arch/${MACHINE_ARCH}/include \
			    ${DESTDIR}/usr/include/${MACHINE_ARCH} ; \
		else \
			ln -s machine ${DESTDIR}/usr/include/${MACHINE_ARCH} ; \
		fi

tags:
		cwd=/usr/include; \
		for i in ${STD} ${LINKS}; do \
			ctags -a -f ${TAGSFILE} $$cwd/$$i; \
		done

clean:
		rm -f tags

includes: ${SYS_INCLUDE}

.PHONY: includes copies symlinks tags clean
.include <bsd.prog.mk>
