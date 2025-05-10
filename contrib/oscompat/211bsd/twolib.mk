#	$211BSD: Makefile,v 1.0 2025/03/08 21:15:27 Exp $

#
# 2.11BSD Original Libraries
#
MKLIBNDBM?= 	no
MKLIBFORTRAN?= 	no
MKLIBMP?= 		no
MKLIBOM?= 		no
MKLIBSTUBS?= 	no
MKLIBTERMCAP?= 	no # here for legacy reasons (cannot be enabled: conflicts with libterminfo)
MKLIBVMF?= 		no

#
# Notes:
# libtermcap: 
# - Cannot be enabled, until below sub-points are fixed.
    - conflicts with libterminfo, which has toolchain dependents (i.e. tic), resulting in not compiling
# - Legacy code
