#	$NetBSD: bsd.obj.mk,v 1.46 2003/12/04 12:15:20 lukem Exp $

.if !defined(_BSD_OBJ_MK_)
_BSD_OBJ_MK_=1

.include <bsd.own.mk>

__curdir:=	${.CURDIR}

.if ${MKOBJ} == "no"
obj:
.else
.if defined(MAKEOBJDIRPREFIX) || defined(MAKEOBJDIR)
.if defined(MAKEOBJDIRPREFIX)
__objdir:= ${MAKEOBJDIRPREFIX}${__curdir}
.else
__objdir:= ${MAKEOBJDIR}
.endif

# MAKEOBJDIR and MAKEOBJDIRPREFIX are env variables supported
# by make(1).  We simply mkdir -p the specified path.
# If that fails - we do a mkdir to get the appropriate error message
# before bailing out.
obj:
.if defined(MAKEOBJDIRPREFIX)
	@if [ ! -d ${MAKEOBJDIRPREFIX} ]; then \
		echo "MAKEOBJDIRPREFIX ${MAKEOBJDIRPREFIX} does not exist, bailing..."; \
		exit 1; \
	fi;
.endif
.if ${.CURDIR} == ${.OBJDIR}
	@if [ ! -d ${__objdir} ]; then \
		mkdir -p ${__objdir}; \
		if [ ! -d ${__objdir} ]; then \
			mkdir ${__objdir}; exit 1; \
		fi; \
		${_MKSHMSG} " objdir  ${__objdir}"; \
	fi
.endif
.else
PAWD?=			/bin/pwd

__objdirsuffix=	${OBJMACHINE:D.${MACHINE}${OBJMACHINE_ARCH:D-${MACHINE_ARCH}}}
__objdir=		obj${__objdirsuffix}

__usrobjdir=	${BSDOBJDIR}${USR_OBJMACHINE:D.${MACHINE}}
__usrobjdirpf=	${USR_OBJMACHINE:D:U${__objdirsuffix}}

.if defined(BUILDID)
__objdir:=		${__objdir}.${BUILDID}
__usrobjdirpf:=	${__usrobjdirpf}.${BUILDID}
__need_objdir_target=yes
.endif

.if defined(OBJHOSTMACHINE) && (${MKHOSTOBJ:Uno} != "no")
# In case .CURDIR has been twiddled by a .mk file and is now relative,
# make it absolute again.


print-objdir:
	@echo ${.OBJDIR}

.include <bsd.sys.mk>

.endif	# !defined(_BSD_OBJ_MK_)