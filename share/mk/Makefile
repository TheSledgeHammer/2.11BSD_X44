#	$NetBSD: Makefile,v 1.52 2020/05/18 21:19:35 jmcneill Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	bsd.README bsd.buildinstall.mk bsd.clang-analyze.mk bsd.clean.mk bsd.dep.mk 	\
		bsd.doc.mk bsd.endian.mk bsd.files.mk bsd.gcc.mk bsd.host.mk bsd.hostinit.mk	\
		bsd.hostlib.mk bsd.hostprog.mk bsd.inc.mk bsd.info.mk bsd.init.mk 				\
		bsd.kernobj.mk bsd.kinc.mk bsd.lib.mk bsd.links.mk bsd.loader.mk bsd.man.mk 	\
		bsd.nls.mk bsd.obj.mk bsd.oscompat.mk bsd.own.mk bsd.prog.mk bsd.rpc.mk 		\
		bsd.sanitizer.mk bsd.shlib.mk bsd.subdir.mk bsd.sys.mk sys.mk
			
FILESDIR=	/usr/share/mk
.endif

FILESMODE=	444

.include <bsd.prog.mk>
