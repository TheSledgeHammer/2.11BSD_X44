#	$211BSD: Makefile,v 1.0 2025/03/08 21:15:27 Exp $

#
# 2.11BSD Original Libraries
#
MKLIBNDBM?= 	no
MKLIBFORTRAN?= 	no
MKLIBMP?= 		no
MKLIBOM?= 		no
MKLIBSTUBS?= 	no
MKLIBTERMCAP?= 	no
MKLIBVMF?= 		no

#
# Notes/Info:
# libndbm:
#    - includes dbm and ndbm
# libfortran:
#    - includes libF77, libI77 and libU77
# libmp:
#     - Math Library Part1
# libom:
#     - Math Library Part2
# libstubs:
#     - Time library stubs. Can be used with ctimed (ctime daemon)
#     - Needs fixes applied before it can be enabled
# libtermcap: 
#     - Cannot be enabled, until below sub-points are fixed.
#         - conflicts with libterminfo, which has toolchain dependents (i.e. tic), 
#           results in failing to compile
# libvmf:
#     - Disk Based Virtual Memory Routines
#         - Was used in 2.11BSD's linker program
