ARCH=	i386 pdp
all:
	@echo "make tags, make links or init_sysent.c only"

sysent: init_sysent.c syscalls.c ../h/syscall.h ../h/syscall.mk

init_sysent.c syscalls.c ../sys/syscall.h \
../sys/syscall.mk: makesyscalls.sh syscalls.master
	sh makesyscalls.sh syscalls.master