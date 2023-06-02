#	$NetBSD: Makefile,v 1.37.2.1 2004/06/22 07:56:26 tron Exp $
# @(#)Makefile	1.6 Makefile 1997/12/28

.include <bsd.own.mk>

# If you want something other than Western United States time used on your
# system, change the line below (after finding the zone you want in the
# time zone files, or adding it to a time zone file).
# Alternately, if you discover you've got the wrong time zone, you can just
#	install -c -m 444 /usr/share/zoneinfo/right_zone /etc/localtime

LOCALTIME=	US/Pacific
POSIXRULES=	${LOCALTIME}

# Use an absolute path name for TZDIR unless you're just testing the software.
# Note: ${DESTDIR} is prepended to this for the actual copy.
TZDIR=	/usr/share/zoneinfo

# If you want code inspired by certain emerging standards, add
#	-DSTD_INSPIRED
# to the end of the "CFLAGS=" line.
#
# If you want to handle solar-time-based time zones, remove the
# "#define NOSOLAR" from the include file usr/include/tzfile.h.
# (and add solar87 to the DATA= line below).


# If you always want time values interpreted as "seconds since the epoch
# (not counting leap seconds)", use
# 	REDO=		posix_only
# below.  If you always want right time values interpreted as "seconds since
# the epoch" (counting leap seconds)", use
#	REDO=		right_only
# below.  If you want both sets of data available, with leap seconds not
# counted normally, use
#	REDO=		posix_right
# below.  If you want both sets of data available, with leap seconds counted
# normally, use
#	REDO=		right_posix
# below.

REDO=		posix_only

# Since "." may not be in PATH...
YEARISTYPE=	"${HOST_SH} ${.CURDIR}/yearistype.sh"

YDATA=		africa antarctica asia australasia \
			europe etcetera northamerica southamerica \
			pacificnew factory backward
NDATA=		systemv
SDATA=		solar87 solar88 solar89
TDATA=		$(YDATA) $(NDATA) $(SDATA)
DATA=		$(YDATA) $(NDATA) $(SDATA) leapseconds # yearistype.sh
USNO=		usno1988 usno1989

TZBUILDDIR=	${.OBJDIR}/builddir

.PHONY:	posix_only
posix_only: ${TDATA}
	${_MKTARGET_CREATE}
	mkdir -p ${TZBUILDDIR}
	cd ${.CURDIR} && \
	    ${TOOL_ZIC} -y ${YEARISTYPE} -d ${TZBUILDDIR} -L /dev/null ${TDATA}

.PHONY:	right_only
right_only: leapseconds ${TDATA}
	${_MKTARGET_CREATE}
	mkdir -p ${TZBUILDDIR}
	cd ${.CURDIR} && \
	    ${TOOL_ZIC} -y ${YEARISTYPE} -d ${TZBUILDDIR} -L leapseconds ${TDATA}

.PHONY:	other_two
other_two: leapseconds ${TDATA}
	${_MKTARGET_CREATE}
	mkdir -p ${TZBUILDDIR}
	cd ${.CURDIR} && \
	    ${TOOL_ZIC} -y ${YEARISTYPE} -d ${TZBUILDDIR}/posix -L /dev/null ${TDATA}
	cd ${.CURDIR} && \
	    ${TOOL_ZIC} -y ${YEARISTYPE} -d ${TZBUILDDIR}/right -L leapseconds ${TDATA}

.PHONY:	posix_right
posix_right: posix_only other_two

.PHONY:	right_posix
right_posix: right_only other_two

.if ${MKUNPRIVED} == "no"
TOOL_PAX.unpriv=-pe
.else
TOOL_PAX.unpriv=
.endif

.if ${MKSHARE} != "no"
afterinstall: ${DATA} ${REDO}
	${_MKMSG_CREATE} ${POSIXRULES}
	mkdir -p ${TZBUILDDIR}
	cd ${.CURDIR} && \
	    ${TOOL_ZIC} -y ${YEARISTYPE} -d ${TZBUILDDIR} -p ${POSIXRULES}
	cd ${TZBUILDDIR} && find . -type f -print | sort \
	| while read tzfile; do \
		destfile=${DESTDIR}${TZDIR}/$${tzfile#./}; \
		cmp -s $$tzfile $$destfile > /dev/null 2>&1 && continue; \
		${_MKSHMSG_INSTALL} $$destfile; \
		${_MKSHECHO} ${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} \
		    -m ${BINMODE} $$tzfile $$destfile ; \
		${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} \
		    -m ${BINMODE} $$tzfile $$destfile ; \
	done
.else	# ${MKSHARE} == "no"
afterinstall:
.endif	# ${MKSHARE} == "no"

clean:
	-rm -rf ${TZBUILDDIR}

.include <bsd.prog.mk>