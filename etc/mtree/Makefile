#	$NetBSD: Makefile,v 1.45 2021/04/25 23:40:52 christos Exp $
.include <bsd.own.mk>

CONFIGFILES=	211BSD.dist special
FILESDIR=		/etc/mtree

# distrib-dirs --
#	Populate $DESTDIR with directories needed by NetBSD
#
.if ${MKUNPRIVED} == "no"
TOOL_MTREE.unpriv=
.else
TOOL_MTREE.unpriv=	-W
.endif

# postinstall(8) invokes this target to produce the right
# /etc/mtree/211BSD.dist content without duplicating logic from
# the Makefile.
#
distrib-dirs: .PHONY check_DESTDIR
.if !defined(DISTRIBUTION_DONE)						# {
# Create DESTDIR using HOST_INSTALL_DIR, not INSTALL_DIR, because
# INSTALL_DIR would want to write to the metalog, and it can't do that
# if the metalog is inside DESTDIR but DESTDIR doesn't yet exist.
	${HOST_INSTALL_DIR} -m 755 ${DESTDIR}
# Invoke mtree to create the directories listed in 211BSD.dist;
# then invoke mtree again to register those directories in the metalog.
	${TOOL_MTREE} -def ${.OBJDIR}/211BSD.dist -N ${.CURDIR}/.. \
	    -p ${DESTDIR}/ -U ${TOOL_MTREE.unpriv}
.if ${MKUNPRIVED} != "no"						# {
	${TOOL_MTREE} -def ${.OBJDIR}/211BSD.dist -N ${.CURDIR}/.. \
	    -p ${DESTDIR}/ -C -k all | \
	    ${TOOL_AWK} '/ optional/ {next} // {print}' | ${METALOG.add}
.endif	# MKUNPRIVED							# }
.endif	# DISTRIBUTION_DONE						# }

#CLEANFILES+=	211BSD.dist

.include <bsd.prog.mk>
