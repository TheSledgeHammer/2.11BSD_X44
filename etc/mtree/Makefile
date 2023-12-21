#	$NetBSD: Makefile,v 1.41 2019/03/12 10:07:49 mrg Exp $

.include <bsd.own.mk>


# The build specific files.


TwoBSD.dist:	TwoBSD.dist.tmp
	cmp -s TwoBSD.dist.tmp TwoBSD.dist || { \
		echo "Updating TwoBSD.dist"; \
		mv TwoBSD.dist.tmp TwoBSD.dist; \
	}
# Rebuild every time
.PHONY: TwoBSD.dist.tmp
TwoBSD.dist.tmp: TwoBSD.dist.base 
	${TOOL_CAT} ${.ALLSRC} > ${.TARGET}

CONFIGFILES=    TwoBSD.dist special
FILESDIR=	    /etc/mtree

# distrib-dirs --
#	Populate $DESTDIR with directories needed by NetBSD
#
.if ${MKUNPRIVED} == "no"
TOOL_MTREE.unpriv=
.else
TOOL_MTREE.unpriv=	-W
.endif

# postinstall(8) invokes this target to produce the right
# /etc/mtree/NetBSD.dist content without duplicating logic from
# the Makefile.
#
emit_dist_file:	TwoBSD.dist.base
	@cat ${.ALLSRC}

distrib-dirs: .PHONY check_DESTDIR TwoBSD.dist
.if !defined(DISTRIBUTION_DONE)						# {
# Create DESTDIR using HOST_INSTALL_DIR, not INSTALL_DIR, because
# INSTALL_DIR would want to write to the metalog, and it can't do that
# if the metalog is inside DESTDIR but DESTDIR doesn't yet exist.
	${HOST_INSTALL_DIR} -m 755 ${DESTDIR}
# Invoke mtree to create the directories listed in NetBSD.dist;
# then invoke mtree again to register those directories in the metalog.
	${TOOL_MTREE} -def ${.OBJDIR}/TwoBSD.dist -N ${.CURDIR}/.. \
	    -p ${DESTDIR}/ -U ${TOOL_MTREE.unpriv}
.if ${MKUNPRIVED} != "no"						# {
	${TOOL_MTREE} -def ${.OBJDIR}/TwoBSD.dist -N ${.CURDIR}/.. \
	    -p ${DESTDIR}/ -C -k all | \
	    ${TOOL_AWK} '/ optional/ {next} // {print}' | ${METALOG.add}
.endif	# MKUNPRIVED							# }
.endif	# DISTRIBUTION_DONE						# }

CLEANFILES+=	TwoBSD.dist TwoBSD.dist.tmp

.include <bsd.prog.mk>
