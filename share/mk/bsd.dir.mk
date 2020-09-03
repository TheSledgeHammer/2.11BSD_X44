# Generic Makefile Directory Definitions:

SRCTOP?=		/usr/src
SYSDIR?=		${SRCTOP}/sys

# kern directories
ARCHDIR?=		${SYSDIR}/arch
CONFDIR?=		${SYSDIR}/conf
DDBDIR/=		${SYSDIR}/ddb
DEVDIR?=		${SYSDIR}/dev
KERNDIR?=		${SYSDIR}/kern
LIBDIR?=		${SYSDIR}/lib
MISCFSDIR?=		${SYSDIR}/miscfs
NETDIR?=		${SYSDIR}/net
NETIMPDIR?=		${SYSDIR}/netimp
NETINETDIR?=	${SYSDIR}/netinet
NETNSDIR?=		${SYSDIR}/netns
STANDDIR?=		${SYSDIR}/stand
UFSDIR?=		${SYSDIR}/ufs
VFSDIR?=		${SYSDIR}/vfs
VMDIR?=			${SYSDIR}/vm

# lib & stand sub-directories
BOOTDIR?=		${STANDDIR}/boot
LIBKERN?= 		${LIBDIR}/libkern
LIBSA?=			${LIBDIR}/libsa