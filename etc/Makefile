#	$NetBSD: Makefile,v 1.449 2020/12/29 16:46:44 martin Exp $
#	from: @(#)Makefile	8.7 (Berkeley) 5/25/95

# Environment variables without default values:
#   DESTDIR must be set before anything in this file will work.
#   RELEASEDIR is where the tarred up stuff for a snapshot or
#	release will be placed.
#
# Environment variables with default values:
#   LOCALTIME will set the default local time for the system you
#	build; it determines what /etc/localtime is symlink'd to.
#   KERNSRCDIR points to kernel source; it is set by default to ../sys,
#	but can be overridden.
#   KERNOBJDIR is the kernel build directory, it defaults to
#	${KERNSRCDIR}/arch/${MACHINE}/compile, but can be overridden.
#   KERNCONFDIR is where the configuration files for kernels are found;
#	default is ${KERNSRCDIR}/arch/${MACHINE}/conf but can be overridden.
#   MKPOSTFIX; if not `no', install postfix configuration
#   MKUNPRIVED; if not `no', allow non-root installs.
#   MKUPDATE; if not `no', don't do a 'make clean' before kernel compile
#
# Targets:
#    distribution: makes a full NetBSD distribution in DESTDIR. If
#	INSTALL_DONE is set, it will not do a `make install.'
#	if DISTRIBUTION_DONE is set, it will not do anything.
#    distrib-dirs: creates an empty NetBSD directory tree in DESTDIR.
#	Called by distribution.
#    snapshot: calls distribution, above, and then tars up the files
#	into a release(7) format in RELEASEDIR/${RELEASEMACHINEDIR}.
#	Any port-dependent stuff for this target is found in
#	etc.${MACHINE}/Makefile.inc.
#    release: a synonym for `snapshot'
#

# For MK* vars
.include <bsd.own.mk>

.include <bsd.kernobj.mk>	# For KERNSRCDIR, KERNOBJDIR, ...
.include <bsd.endian.mk>	# For TARGET_ENDIANNESS

.MAKEOVERRIDES+=	USETOOLS

TZDIR=				/usr/share/zoneinfo
LOCALTIME?=			UTC
CKSUM?=				${TOOL_CKSUM}
MAKESUMS=			MAKE=${MAKE:Q} CKSUM=${CKSUM:Q} ${HOST_SH} ${NETBSDSRCDIR}/distrib/sets/makesums
DISTRIBVER!=		${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh

GZIP_FLAGS= -9 ${GZIP_N_FLAG}

# Flags for creating ISO CDROM image
# mkisofs is expected to be in $PATH, install via pkgsrc/sysutils/cdrtools
# Note: At least mkisofs 2.0 should be used.
#
.if !defined(MKISOFS)
MKISOFS!=       (which mkisofs || echo true) 2>/dev/null
.endif
DISTRIBREV!=	${HOST_SH} ${KERNSRCDIR}/conf/osrelease.sh -s
# ISO 9660 volume ID.  Note that this can only contain [A-Z0-9_].
ISO_VOLID!=	echo 211BSD_${DISTRIBREV} | tr a-z A-Z
MKISOFS_FLAGS+= -J -l -hide-joliet-trans-tbl -r -T \
				-V ${ISO_VOLID} \
				-publisher "The 2.11BSD Project" \
				-m "${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/cdrom"
.if ${MKISOFS_FLAGS:N-v}
MKISOFS_FLAGS+=	-quiet
.endif

# MD Makefile.inc may append MD targets to BIN[123].  Make sure all
# are empty, to preserve the old semantics of setting them below with "+=".
#
BIN1=
BIN2=
BIN3=

# Directories to build in ${RELEASEDIR}/${RELEASEMACHINEDIR}.
# MD Makefile.inc files can add to this.
# NOTE: Parent directories must be listed before subdirectories.
#
INSTALLATION_DIRS=	binary binary/sets binary/kernel installation

.if exists(etc.${RELEASEMACHINE}/Makefile.inc)
.include "etc.${RELEASEMACHINE}/Makefile.inc"
.endif

# -rw-r--r--
BINOWN= root
BINGRP= wheel
UTMPGRP= utmp
BIN1+=	csh.cshrc csh.login csh.logout gettytab group hosts	locate.conf 	\
		login.conf man.conf passwd.conf phones profile 	protocols rc 		\
		rc.firewall rc.network rc.resume rc.shutdown rc.subr rc.suspend 	\
		rc.sysctl services shells shrc syslog.conf wscons.conf 	

# Use machine-specific disktab if it exists, or the MI one otherwise
.if exists(etc.${MACHINE}/disktab)
BIN1+=	etc.${MACHINE}/disktab
.else
BIN1+=	disktab
.endif

.if exists(etc.${MACHINE}/ld.so.conf) && \
    empty(MACHINE_ARCH:M*arm*hf*) && empty(MACHINE_ARCH:Maarch64*)
BIN1+=	etc.${MACHINE}/ld.so.conf
.endif

.if exists(etc.${MACHINE}/ttyaction)
BIN1+=	etc.${MACHINE}/ttyaction
.endif

# -rw-rw-r--
BIN2+=	motd

# -rw-------
BIN3+=	hosts.equiv

#SYSPKG=	etc
#ETC_PKG=-T etc_pkg
#BASE_PKG=-T base_pkg
#ETC_INSTALL_FILE=cd ${.CURDIR} && ${INSTALL_FILE} ${ETC_PKG}
#ETC_INSTALL_OBJ_FILE=cd ${.OBJDIR} && ${INSTALL_FILE} ${ETC_PKG}

.if ${TARGET_ENDIANNESS} == "1234"
PWD_MKDB_ENDIAN=	-L
.elif ${TARGET_ENDIANNESS} == "4321"
PWD_MKDB_ENDIAN=	-B
.else
PWD_MKDB_ENDIAN=
.endif

.include "${NETBSDSRCDIR}/etc/Makefile.params"

CLEANFILES+=	etc-release
etc-release: .EXEC .MAKE
	${_MKTARGET_CREATE}
	@(	echo "211BSD ${DISTRIBVER}/${MACHINE}"; \
		echo ; \
		cat ${NETBSDSRCDIR}/sys/conf/copyright; \
		echo ; \
		echo "Build information:"; \
		printf "%20s   %s\n" "Build date" "${BUILD_DATE}"; \
		printf "%20s   %s\n"  "Built by" "${BUILDER}"; \
		if [ -n "${BUILDID}" ]; then \
		    printf "%20s   %s\n"  "Build ID" "${BUILDID}" ; \
		fi ; \
		if [ -n "${BUILDINFO}" ]; then \
		    echo ; \
		    info="$$(printf "%b" ${BUILDINFO:Q})" ; \
		    printf "%s\n" "$${info}" \
		    | ${TOOL_SED} -e 's/^/        /' ; \
		fi ; \
		echo ; \
		echo "Build settings:"; \
		echo ; \
		${PRINT_PARAMS} ; \
	) >${.OBJDIR}/${.TARGET}

install-etc-release: .PHONY etc-release
	${_MKMSG_INSTALL} etc/release
	${ETC_INSTALL_OBJ_FILE} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    etc-release ${DESTDIR}/etc/release
	    
FILESDIR=		/etc
CONFIGFILES=
CONFIGSYMLINKS=

.for file in ${BIN1}
CONFIGFILES+=		${file}
FILESMODE_${file:T}=	644
.endfor

.for file in ${BIN2}
CONFIGFILES+=		${file}
FILESMODE_${file:T}=	664
.endfor

.for file in ${BIN3}
CONFIGFILES+=		${file}
FILESMODE_${file:T}=	600
.endfor

.if (${MKPOSTFIX} != "no")
CONFIGFILES+=		aliases
FILESDIR_aliases=	/etc/mail
FILESMODE_aliases=	644
.endif

CONFIGFILES+=		crontab
FILESDIR_crontab=	/var/cron/tabs
FILESNAME_crontab=	root
FILESMODE_crontab=	600

CONFIGFILES+=		minfree
FILESDIR_minfree=	/var/crash
FILESMODE_minfree=	600

CONFIGSYMLINKS+=	${TZDIR}/${LOCALTIME}	/etc/localtime \
					/usr/sbin/rmt		/etc/rmt
						    
# install-etc-files --
#	Install etc (config) files; not performed by "make build"
#
install-etc-files: .PHONY .MAKE check_DESTDIR
	${_MKMSG_INSTALL} ${DESTDIR}/etc/master.passwd
	${ETC_INSTALL_FILE} -o root -g wheel -m 600 \
	    master.passwd ${DESTDIR}/etc
	${TOOL_PWD_MKDB} -p ${PWD_MKDB_ENDIAN} -d ${DESTDIR}/ \
	    ${DESTDIR}/etc/master.passwd
.if ${MKUNPRIVED} != "no"
	( \
		mode=0600; \
		for metaent in spwd.db passwd pwd.db; do \
	    		echo "./etc/$${metaent} type=file mode=$${mode} uname=root gname=wheel tags=etc_pkg"; \
			mode=0644; \
		done; \
	) | ${METALOG.add}
.endif	# MKUNPRIVED != no
	${_MKMSG_INSTALL} ${DESTDIR}/etc/ttys
.if exists(${.CURDIR}/etc.${MACHINE}/ttys)
	${ETC_INSTALL_OBJ_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${.CURDIR}/etc.${MACHINE}/ttys ${DESTDIR}/etc
.else
	${ETC_INSTALL_OBJ_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${.CURDIR}/etc.${MACHINE_CPU}/ttys ${DESTDIR}/etc
.endif
	${ETC_INSTALL_OBJ_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
		${.CURDIR}/MAKEDEV.local ${.CURDIR}/etc.${MACHINE_CPU}/MAKEDEV ${DESTDIR}/dev
.for owner group mode file in 									\
		${BINOWN} operator		664	/etc/dumpdates  			\
		${BINOWN} operator		600	/etc/skeykeys 				\
		root wheel				600	/var/at/at.deny 			\
		root wheel				644	/var/db/locate.database 	\
		${BINOWN} ${BINGRP}		600	/var/log/authlog 			\
		root wheel				600	/var/log/cron 				\
		${BINOWN} ${UTMPGRP}	664	/var/log/lastlog 			\
		${BINOWN} ${UTMPGRP}	664	/var/log/lastlogx 			\
		${BINOWN} ${BINGRP}		640	/var/log/lpd-errs 			\
		${BINOWN} ${BINGRP}		600	/var/log/maillog 			\
		${BINOWN} ${BINGRP}		644	/var/log/messages 			\
		${BINOWN} ${BINGRP}		600	/var/log/secure 			\
		${BINOWN} ${UTMPGRP}	664	/var/log/wtmp 				\
		${BINOWN} ${UTMPGRP}	664	/var/log/wtmpx 				\
		${BINOWN} ${BINGRP}		600	/var/log/xferlog 			\
		daemon staff			664	/var/msgs/bounds 			\
		${BINOWN} ${UTMPGRP}	664	/var/run/utmp 				\
		${BINOWN} ${UTMPGRP}	664	/var/run/utmpx 				\	
		games games				664	/var/games/atc_score 		\
		games games				664	/var/games/battlestar.log 	\
		games games				664	/var/games/cfscores 		\
		games games				664	/var/games/criblog 			\
		games games				660	/var/games/hackdir/perm 	\
		games games				660	/var/games/hackdir/record 	\
		games games				664	/var/games/larn/llog12.0 	\
		games games				664	/var/games/larn/lscore12.0 	\
		games games				664	/var/games/larn/playerids 	\
		games games				664	/var/games/robots_roll 		\
		games games				664	/var/games/rogue.scores 	\
		games games				664	/var/games/saillog 			\
		games games				664	/var/games/snakerawscores 	\
		games games				664	/var/games/snake.log 		\
		games games				664	/var/games/tetris.scores
	${_MKMSG_INSTALL} ${DESTDIR}${file}
	if [ ! -e ${DESTDIR}${file} -o -s ${DESTDIR}${file} ]; then \
		${ETC_INSTALL_FILE} -o ${owner} -g ${group} -m ${mode} 	\
			/dev/null ${DESTDIR}${file}; \
	else true; fi
.endfor
.for subdir in . defaults mtree periodic rc.d root
	${MAKEDIRTARGET} ${subdir} configinstall
.endfor
.if (${MKPOSTFIX} != "no")
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/contrib/postfix configinstall
.endif

# distrib-dirs --
#	Populate $DESTDIR with directories needed by NetBSD
#
.if ${MKUNPRIVED} == "no"
TOOL_MTREE.unpriv=
.else
TOOL_MTREE.unpriv=	-W
.endif

distrib-dirs: .PHONY check_DESTDIR
.if !defined(DISTRIBUTION_DONE)
	${INSTALL_DIR} -o root -g wheel -m 755 ${BASE_PKG} ${DESTDIR}
.endif
	${TOOL_MTREE} -def ${.CURDIR}/mtree/211BSD.dist -N ${.CURDIR} \
	    -p ${DESTDIR}/ -U ${TOOL_MTREE.unpriv}
	    
COMPRESS_PROGRAM=${"${USE_XZ_SETS:Uno}"!="no":?${TOOL_XZ}:${TOOL_GZIP}}
XZ_OPT=-9
TAR_SUFF=${"${USE_XZ_SETS:Uno}"!="no":?tar.xz:tgz}

# TODO: Complete when ready

SUBDIR=	defaults rc.d mtree

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
