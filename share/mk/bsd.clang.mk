
.if !empty(MACHINE_ARCH:Mearm*)
_LIBC_COMPILER_RT.${MACHINE_ARCH}=	yes
.endif

_LIBC_COMPILER_RT.aarch64=		yes
_LIBC_COMPILER_RT.aarch64eb=	yes
_LIBC_COMPILER_RT.i386=			yes
_LIBC_COMPILER_RT.powerpc=		yes
_LIBC_COMPILER_RT.powerpc64=	yes
_LIBC_COMPILER_RT.sparc=		yes
_LIBC_COMPILER_RT.sparc64=		yes
_LIBC_COMPILER_RT.x86_64=		yes

.if ${HAVE_LLVM:Uno} == "yes" && ${_LIBC_COMPILER_RT.${MACHINE_ARCH}:Uno} == "yes"
HAVE_LIBGCC?=	no
.else
HAVE_LIBGCC?=	yes
.endif


# Should libgcc have unwinding code?
.if ${HAVE_LLVM:Uno} == "yes" || !empty(MACHINE_ARCH:Mearm*)
HAVE_LIBGCC_EH?=	no
.else
HAVE_LIBGCC_EH?=	yes
.endif