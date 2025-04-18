#	$NetBSD: Makefile,v 1.16 2020/06/13 10:49:17 lukem Exp $

.include <bsd.hostinit.mk>

HOSTPROG= 		host-mkdep
HOSTPROGNAME=	${_TOOL_PREFIX}host-mkdep
HOST_BINDIR=	${TOOLDIR}/bin

SRCS=	# empty

CLEANFILES+=	config.cache config.log config.status host-mkdep

.include <bsd.hostprog.mk>

# XXX: The configure script needs to use awk, but we can't pass
# AWK=${TOOL_AWK:Q} in CONFIGURE_ENV, because TOOL_AWK is not yet
# available at the time that host-mkdep is built.  The configure script
# will try to find some other version of awk.
#
CONFIGURE_ENV=	CC=${HOST_CC:Q}

CONFIGURE_ARGS=	--cache-file=config.cache
.if ${MAKEVERBOSE} == 0
CONFIGURE_ARGS+=--silent
.endif

realall: host-mkdep
host-mkdep: configure host-mkdep.in
	-rm -f $@
	${CONFIGURE_ENV} ${HOST_SH} ${.CURDIR}/configure ${CONFIGURE_ARGS}
	chmod +x $@

# Use uninstalled copy of the install program
INSTALL_OBJ!=	cd ${NETBSDSRCDIR}/tools/binstall && ${PRINTOBJDIR}
INSTALL=		${INSTALL_OBJ}/xinstall

# Install rule, copied from src/tools/Makefile.host.
# We can't include Makefile.host because there is no HOST_SRCDIR
# corresponding to host-mkdep.
#
realinstall: install.host
install.host: ${HOST_BINDIR}/${HOSTPROGNAME}
${HOST_BINDIR}/${HOSTPROGNAME}:: ${HOSTPROG}
	${_MKTARGET_INSTALL}
	mkdir -p ${HOST_BINDIR}
	${HOST_INSTALL_FILE} -m ${BINMODE} ${HOSTPROG}${HOSTEXEEXT} ${.TARGET}

.if ${MKUPDATE} == "no"
.PHONY:		${HOST_BINDIR}/${HOSTPROGNAME}
.endif

# Run by hand, then "configure" script committed:
regen:
	cd ${.CURDIR} && ${TOOLDIR}/bin/${_TOOL_PREFIX}autoconf
