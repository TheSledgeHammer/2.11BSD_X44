#!/bin/sh -
#
#	$NetBSD: newvers.sh,v 1.37 2004/01/05 03:33:06 lukem Exp $
#
# Copyright (c) 1984, 1986, 1990, 1993
#	The Regents of the University of California.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the University of
#	California, Berkeley and its contributors.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
#	@(#)newvers.sh	8.1 (Berkeley) 4/20/94

if [ ! -e version ]; then
	echo 0 > version
fi

v=$(cat version)
t=$(date)
u=${USER-root}
h=$(hostname)
d=$(pwd)
cwd=$(dirname $0)
copyright=$(awk '{ print "\""$0"\\n\""}' ${cwd}/copyright)

if [ -f ident ]; then
	id="$(cat ident)"
else
	id=$(basename ${d})
fi

osrelcmd=${cwd}/osrelease.sh

ost="2.11BSD_X44"
osr=$(sh $osrelcmd)

fullversion="${ost} ${osr} (${id}) #${v}: ${t}\n\t${u}@${h}:${d}\n"

cat << _EOF > vers.c
/*
 * Automatically generated file from $0
 * Do not edit.
 */
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
const char ostype[] = "${ost}";
const char osrelease[] = "${osr}";
const char sccs[] = "@(#)${fullversion}";
const char version[] = "${fullversion}";
const char copyright[] =
${copyright}
"\n";

_EOF
echo $(expr ${v} + 1) > version
