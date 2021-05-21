#! /usr/bin/env sh
#	$NetBSD: build.sh,v 1.347 2021/04/25 22:29:22 christos Exp $
#
# Copyright (c) 2001-2011 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Todd Vierling and Luke Mewburn.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# Top level build wrapper, to build or cross-build NetBSD.
#

#
# {{{ Begin shell feature tests.
#
# We try to determine whether or not this script is being run under
# a shell that supports the features that we use.  If not, we try to
# re-exec the script under another shell.  If we can't find another
# suitable shell, then we print a message and exit.
#

errmsg=''		# error message, if not empty
shelltest=false		# if true, exit after testing the shell
re_exec_allowed=true	# if true, we may exec under another shell

# Parse special command line options in $1.  These special options are
# for internal use only, are not documented, and are not valid anywhere
# other than $1.
case "$1" in
"--shelltest")
    shelltest=true
    re_exec_allowed=false
    shift
    ;;
"--no-re-exec")
    re_exec_allowed=false
    shift
    ;;
esac

# Solaris /bin/sh, and other SVR4 shells, do not support "!".
# This is the first feature that we test, because subsequent
# tests use "!".
#
if test -z "$errmsg"; then
    if ( eval '! false' ) >/dev/null 2>&1 ; then
	:
    else
	errmsg='Shell does not support "!".'
    fi
fi

# Does the shell support functions?
#
if test -z "$errmsg"; then
    if ! (
	eval 'somefunction() { : ; }'
	) >/dev/null 2>&1
    then
	errmsg='Shell does not support functions.'
    fi
fi

# Does the shell support the "local" keyword for variables in functions?
#
# Local variables are not required by SUSv3, but some scripts run during
# the NetBSD build use them.
#
# ksh93 fails this test; it uses an incompatible syntax involving the
# keywords 'function' and 'typeset'.
#
if test -z "$errmsg"; then
    if ! (
	eval 'f() { local v=2; }; v=1; f && test x"$v" = x"1"'
	) >/dev/null 2>&1
    then
	errmsg='Shell does not support the "local" keyword in functions.'
    fi
fi

# Does the shell support ${var%suffix}, ${var#prefix}, and their variants?
#
# We don't bother testing for ${var+value}, ${var-value}, or their variants,
# since shells without those are sure to fail other tests too.
#
if test -z "$errmsg"; then
    if ! (
	eval 'var=a/b/c ;
	      test x"${var#*/};${var##*/};${var%/*};${var%%/*}" = \
		   x"b/c;c;a/b;a" ;'
	) >/dev/null 2>&1
    then
	errmsg='Shell does not support "${var%suffix}" or "${var#prefix}".'
    fi
fi

# Does the shell support IFS?
#
# zsh in normal mode (as opposed to "emulate sh" mode) fails this test.
#
if test -z "$errmsg"; then
    if ! (
	eval 'IFS=: ; v=":a b::c" ; set -- $v ; IFS=+ ;
		test x"$#;$1,$2,$3,$4;$*" = x"4;,a b,,c;+a b++c"'
	) >/dev/null 2>&1
    then
	errmsg='Shell does not support IFS word splitting.'
    fi
fi

# Does the shell support ${1+"$@"}?
#
# Some versions of zsh fail this test, even in "emulate sh" mode.
#
if test -z "$errmsg"; then
    if ! (
	eval 'set -- "a a a" "b b b"; set -- ${1+"$@"};
	      test x"$#;$1;$2" = x"2;a a a;b b b";'
	) >/dev/null 2>&1
    then
	errmsg='Shell does not support ${1+"$@"}.'
    fi
fi

# Does the shell support $(...) command substitution?
#
if test -z "$errmsg"; then
    if ! (
	eval 'var=$(echo abc); test x"$var" = x"abc"'
	) >/dev/null 2>&1
    then
	errmsg='Shell does not support "$(...)" command substitution.'
    fi
fi

# Does the shell support $(...) command substitution with
# unbalanced parentheses?
#
# Some shells known to fail this test are:  NetBSD /bin/ksh (as of 2009-12),
# bash-3.1, pdksh-5.2.14, zsh-4.2.7 in "emulate sh" mode.
#
if test -z "$errmsg"; then
    if ! (
	eval 'var=$(case x in x) echo abc;; esac); test x"$var" = x"abc"'
	) >/dev/null 2>&1
    then
	# XXX: This test is ignored because so many shells fail it; instead,
	#      the NetBSD build avoids using the problematic construct.
	: ignore 'Shell does not support "$(...)" with unbalanced ")".'
    fi
fi

# Does the shell support getopts or getopt?
#
if test -z "$errmsg"; then
    if ! (
	eval 'type getopts || type getopt'
	) >/dev/null 2>&1
    then
	errmsg='Shell does not support getopts or getopt.'
    fi
fi

#
# If shelltest is true, exit now, reporting whether or not the shell is good.
#
if $shelltest; then
    if test -n "$errmsg"; then
	echo >&2 "$0: $errmsg"
	exit 1
    else
	exit 0
    fi
fi

#
# If the shell was bad, try to exec a better shell, or report an error.
#
# Loops are broken by passing an extra "--no-re-exec" flag to the new
# instance of this script.
#
if test -n "$errmsg"; then
    if $re_exec_allowed; then
	for othershell in \
	    "${HOST_SH}" /usr/xpg4/bin/sh ksh ksh88 mksh pdksh dash bash
	    # NOTE: some shells known not to work are:
	    # any shell using csh syntax;
	    # Solaris /bin/sh (missing many modern features);
	    # ksh93 (incompatible syntax for local variables);
	    # zsh (many differences, unless run in compatibility mode).
	do
	    test -n "$othershell" || continue
	    if eval 'type "$othershell"' >/dev/null 2>&1 \
		&& "$othershell" "$0" --shelltest >/dev/null 2>&1
	    then
		cat <<EOF
$0: $errmsg
$0: Retrying under $othershell
EOF
		HOST_SH="$othershell"
		export HOST_SH
		exec $othershell "$0" --no-re-exec "$@" # avoid ${1+"$@"}
	    fi
	    # If HOST_SH was set, but failed the test above,
	    # then give up without trying any other shells.
	    test x"${othershell}" = x"${HOST_SH}" && break
	done
    fi

    #
    # If we get here, then the shell is bad, and we either could not
    # find a replacement, or were not allowed to try a replacement.
    #
    cat <<EOF
$0: $errmsg
The NetBSD build system requires a shell that supports modern POSIX
features, as well as the "local" keyword in functions (which is a
widely-implemented but non-standardised feature).
Please re-run this script under a suitable shell.  For example:
	/path/to/suitable/shell $0 ...
The above command will usually enable build.sh to automatically set
HOST_SH=/path/to/suitable/shell, but if that fails, then you may also
need to explicitly set the HOST_SH environment variable, as follows:
	HOST_SH=/path/to/suitable/shell
	export HOST_SH
	\${HOST_SH} $0 ...
EOF
    exit 1
fi

#
# }}} End shell feature tests.
#

progname=${0##*/}
toppid=$$
results=/dev/null
tab='	'
nl='
'
trap "exit 1" 1 2 3 15

bomb()
{
	cat >&2 <<ERRORMESSAGE
ERROR: $@
*** BUILD ABORTED ***
ERRORMESSAGE
	kill ${toppid}		# in case we were invoked from a subshell
	exit 1
}

# Quote args to make them safe in the shell.
# Usage: quotedlist="$(shell_quote args...)"
#
# After building up a quoted list, use it by evaling it inside
# double quotes, like this:
#    eval "set -- $quotedlist"
# or like this:
#    eval "\$command $quotedlist \$filename"
#
shell_quote()
{(
	local result=''
	local arg qarg
	LC_COLLATE=C ; export LC_COLLATE # so [a-zA-Z0-9] works in ASCII
	for arg in "$@" ; do
		case "${arg}" in
		'')
			qarg="''"
			;;
		*[!-./a-zA-Z0-9]*)
			# Convert each embedded ' to '\'',
			# then insert ' at the beginning of the first line,
			# and append ' at the end of the last line.
			# Finally, elide unnecessary '' pairs at the
			# beginning and end of the result and as part of
			# '\'''\'' sequences that result from multiple
			# adjacent quotes in he input.
			qarg="$(printf "%s\n" "$arg" | \
			    ${SED:-sed} -e "s/'/'\\\\''/g" \
				-e "1s/^/'/" -e "\$s/\$/'/" \
				-e "1s/^''//" -e "\$s/''\$//" \
				-e "s/'''/'/g"
				)"
			;;
		*)
			# Arg is not the empty string, and does not contain
			# any unsafe characters.  Leave it unchanged for
			# readability.
			qarg="${arg}"
			;;
		esac
		result="${result}${result:+ }${qarg}"
	done
	printf "%s\n" "$result"
)}

statusmsg()
{
	${runcmd} echo "===> $@" | tee -a "${results}"
}

statusmsg2()
{
	local msg

	msg="${1}"
	shift
	case "${msg}" in
	????????????????*)	;;
	??????????*)		msg="${msg}      ";;
	?????*)			msg="${msg}           ";;
	*)			msg="${msg}                ";;
	esac
	case "${msg}" in
	?????????????????????*)	;;
	????????????????????)	msg="${msg} ";;
	???????????????????)	msg="${msg}  ";;
	??????????????????)	msg="${msg}   ";;
	?????????????????)	msg="${msg}    ";;
	????????????????)	msg="${msg}     ";;
	esac
	statusmsg "${msg}$*"
}

warning()
{
	statusmsg "Warning: $@"
}

# Find a program in the PATH, and print the result.  If not found,
# print a default.  If $2 is defined (even if it is an empty string),
# then that is the default; otherwise, $1 is used as the default.
find_in_PATH()
{
	local prog="$1"
	local result="${2-"$1"}"
	local oldIFS="${IFS}"
	local dir
	IFS=":"
	for dir in ${PATH}; do
		if [ -x "${dir}/${prog}" ]; then
			result="${dir}/${prog}"
			break
		fi
	done
	IFS="${oldIFS}"
	echo "${result}"
}

# Try to find a working POSIX shell, and set HOST_SH to refer to it.
# Assumes that uname_s, uname_m, and PWD have been set.
set_HOST_SH()
{
	# Even if ${HOST_SH} is already defined, we still do the
	# sanity checks at the end.

	# Solaris has /usr/xpg4/bin/sh.
	#
	[ -z "${HOST_SH}" ] && [ x"${uname_s}" = x"SunOS" ] && \
		[ -x /usr/xpg4/bin/sh ] && HOST_SH="/usr/xpg4/bin/sh"

	# Try to get the name of the shell that's running this script,
	# by parsing the output from "ps".  We assume that, if the host
	# system's ps command supports -o comm at all, it will do so
	# in the usual way: a one-line header followed by a one-line
	# result, possibly including trailing white space.  And if the
	# host system's ps command doesn't support -o comm, we assume
	# that we'll get an error message on stderr and nothing on
	# stdout.  (We don't try to use ps -o 'comm=' to suppress the
	# header line, because that is less widely supported.)
	#
	# If we get the wrong result here, the user can override it by
	# specifying HOST_SH in the environment.
	#
	[ -z "${HOST_SH}" ] && HOST_SH="$(
		(ps -p $$ -o comm | sed -ne "2s/[ ${tab}]*\$//p") 2>/dev/null )"

	# If nothing above worked, use "sh".  We will later find the
	# first directory in the PATH that has a "sh" program.
	#
	[ -z "${HOST_SH}" ] && HOST_SH="sh"

	# If the result so far is not an absolute path, try to prepend
	# PWD or search the PATH.
	#
	case "${HOST_SH}" in
	/*)	:
		;;
	*/*)	HOST_SH="${PWD}/${HOST_SH}"
		;;
	*)	HOST_SH="$(find_in_PATH "${HOST_SH}")"
		;;
	esac

	# If we don't have an absolute path by now, bomb.
	#
	case "${HOST_SH}" in
	/*)	:
		;;
	*)	bomb "HOST_SH=\"${HOST_SH}\" is not an absolute path."
		;;
	esac

	# If HOST_SH is not executable, bomb.
	#
	[ -x "${HOST_SH}" ] ||
	    bomb "HOST_SH=\"${HOST_SH}\" is not executable."

	# If HOST_SH fails tests, bomb.
	# ("$0" may be a path that is no longer valid, because we have
	# performed "cd $(dirname $0)", so don't use $0 here.)
	#
	"${HOST_SH}" build.sh --shelltest ||
	    bomb "HOST_SH=\"${HOST_SH}\" failed functionality tests."
}


# initdefaults --
# Set defaults before parsing command line options.
#
initdefaults()
{
	makeenv=
	makewrapper=
	makewrappermachine=
	makewrapperformat=
	runcmd=
	operations=
	removedirs=

	[ -d usr.bin/make ] || cd "$(dirname $0)"
	[ -d usr.bin/make ] ||
	    bomb "usr.bin/make not found; build.sh must be run from the top \
level of source directory"
	[ -f share/mk/bsd.own.mk ] ||
	    bomb "src/share/mk is missing; please re-fetch the source tree"

	# Set various environment variables to known defaults,
	# to minimize (cross-)build problems observed "in the field".
	#
	# LC_ALL=C must be set before we try to parse the output from
	# any command.  Other variables are set (or unset) here, before
	# we parse command line arguments.
	#
	# These variables can be overridden via "-V var=value" if
	# you know what you are doing.
	#
	unsetmakeenv INFODIR
	unsetmakeenv LESSCHARSET
	unsetmakeenv MAKEFLAGS
	unsetmakeenv TERMINFO
	setmakeenv LC_ALL C

	# Find information about the build platform.  This should be
	# kept in sync with _HOST_OSNAME, _HOST_OSREL, and _HOST_ARCH
	# variables in share/mk/bsd.sys.mk.
	#
	# Note that "uname -p" is not part of POSIX, but we want uname_p
	# to be set to the host MACHINE_ARCH, if possible.  On systems
	# where "uname -p" fails, prints "unknown", or prints a string
	# that does not look like an identifier, fall back to using the
	# output from "uname -m" instead.
	#
	uname_s=$(uname -s 2>/dev/null)
	uname_r=$(uname -r 2>/dev/null)
	uname_m=$(uname -m 2>/dev/null)
	uname_p=$(uname -p 2>/dev/null || echo "unknown")
	case "${uname_p}" in
	''|unknown|*[!-_A-Za-z0-9]*) uname_p="${uname_m}" ;;
	esac

	id_u=$(id -u 2>/dev/null || /usr/xpg4/bin/id -u 2>/dev/null)

	# If $PWD is a valid name of the current directory, POSIX mandates
	# that pwd return it by default which causes problems in the
	# presence of symlinks.  Unsetting PWD is simpler than changing
	# every occurrence of pwd to use -P.
	#
	# XXX Except that doesn't work on Solaris. Or many Linuces.
	#
	unset PWD
	TOP=$( (exec pwd -P 2>/dev/null) || (exec pwd 2>/dev/null) )

	# The user can set HOST_SH in the environment, or we try to
	# guess an appropriate value.  Then we set several other
	# variables from HOST_SH.
	#
	set_HOST_SH
	setmakeenv HOST_SH "${HOST_SH}"
	setmakeenv BSHELL "${HOST_SH}"
	setmakeenv CONFIG_SHELL "${HOST_SH}"

	# Set defaults.
	#
	toolprefix=nb

	# Some systems have a small ARG_MAX.  -X prevents make(1) from
	# exporting variables in the environment redundantly.
	#
	case "${uname_s}" in
	Darwin | FreeBSD | CYGWIN*)
		MAKEFLAGS="-X ${MAKEFLAGS}"
		;;
	esac

	# do_{operation}=true if given operation is requested.
	#
	do_expertmode=false
	do_rebuildmake=false
	do_removedirs=false
	do_tools=false
	do_libs=false
	do_cleandir=false
	do_obj=false
	do_build=false
	do_distribution=false
	do_release=false
	do_kernel=false
	#do_releasekernel=false
	do_kernels=false
	#do_modules=false
	do_installmodules=false
	#do_install=false
	#do_sets=false
	#do_sourcesets=false
	#do_syspkgs=false
	do_iso_image=false
	do_iso_image_source=false
	do_live_image=false
	do_install_image=false
	#do_disk_image=false
	do_params=false

	# done_{operation}=true if given operation has been done.
	#
	done_rebuildmake=false

	# Create scratch directory
	#
	tmpdir="${TMPDIR-/tmp}/nbbuild$$"
	mkdir "${tmpdir}" || bomb "Cannot mkdir: ${tmpdir}"
	trap "cd /; rm -r -f \"${tmpdir}\"" 0
	results="${tmpdir}/build.sh.results"

	# Set source directories
	#
	setmakeenv NETBSDSRCDIR "${TOP}"

	# Make sure KERNOBJDIR is an absolute path if defined
	#
	case "${KERNOBJDIR}" in
	''|/*)	;;
	*)	KERNOBJDIR="${TOP}/${KERNOBJDIR}"
		setmakeenv KERNOBJDIR "${KERNOBJDIR}"
		;;
	esac

	# Find the version of NetBSD
	#
	DISTRIBVER="$(${HOST_SH} ${TOP}/sys/conf/osrelease.sh)"

	# Set the BUILDSEED to 211BSD-"N"
	#
	setmakeenv BUILDSEED "211BSD-$(${HOST_SH} ${TOP}/sys/conf/osrelease.sh -m)"

	# Set MKARZERO to "yes"
	#
	setmakeenv MKARZERO "yes"
}

# valid_MACHINE_ARCH -- A multi-line string, listing all valid
# MACHINE/MACHINE_ARCH pairs.
#
# Each line contains a MACHINE and MACHINE_ARCH value, an optional ALIAS
# which may be used to refer to the MACHINE/MACHINE_ARCH pair, and an
# optional DEFAULT or NO_DEFAULT keyword.
#
# When a MACHINE corresponds to multiple possible values of
# MACHINE_ARCH, then this table should list all allowed combinations.
# If the MACHINE is associated with a default MACHINE_ARCH (to be
# used when the user specifies the MACHINE but fails to specify the
# MACHINE_ARCH), then one of the lines should have the "DEFAULT"
# keyword.  If there is no default MACHINE_ARCH for a particular
# MACHINE, then there should be a line with the "NO_DEFAULT" keyword,
# and with a blank MACHINE_ARCH.
#
valid_MACHINE_ARCH='
MACHINE=amd64		MACHINE_ARCH=x86_64
MACHINE=i386		MACHINE_ARCH=i386
'

# getarch -- find the default MACHINE_ARCH for a MACHINE,
# or convert an alias to a MACHINE/MACHINE_ARCH pair.
#
# Saves the original value of MACHINE in makewrappermachine before
# alias processing.
#
# Sets MACHINE and MACHINE_ARCH if the input MACHINE value is
# recognised as an alias, or recognised as a machine that has a default
# MACHINE_ARCH (or that has only one possible MACHINE_ARCH).
#
# Leaves MACHINE and MACHINE_ARCH unchanged if MACHINE is recognised
# as being associated with multiple MACHINE_ARCH values with no default.
#
# Bombs if MACHINE is not recognised.
#
getarch()
{
	local IFS
	local found=""
	local line

	IFS="${nl}"
	makewrappermachine="${MACHINE}"
	for line in ${valid_MACHINE_ARCH}; do
		line="${line%%#*}" # ignore comments
		line="$( IFS=" ${tab}" ; echo $line )" # normalise white space
		case "${line} " in
		" ")
			# skip blank lines or comment lines
			continue
			;;
		*" ALIAS=${MACHINE} "*)
			# Found a line with a matching ALIAS=<alias>.
			found="$line"
			break
			;;
		"MACHINE=${MACHINE} "*" NO_DEFAULT"*)
			# Found an explicit "NO_DEFAULT" for this MACHINE.
			found="$line"
			break
			;;
		"MACHINE=${MACHINE} "*" DEFAULT"*)
			# Found an explicit "DEFAULT" for this MACHINE.
			found="$line"
			break
			;;
		"MACHINE=${MACHINE} "*)
			# Found a line for this MACHINE.  If it's the
			# first such line, then tentatively accept it.
			# If it's not the first matching line, then
			# remember that there was more than one match.
			case "$found" in
			'')	found="$line" ;;
			*)	found="MULTIPLE_MATCHES" ;;
			esac
			;;
		esac
	done

	case "$found" in
	*NO_DEFAULT*|*MULTIPLE_MATCHES*)
		# MACHINE is OK, but MACHINE_ARCH is still unknown
		return
		;;
	"MACHINE="*" MACHINE_ARCH="*)
		# Obey the MACHINE= and MACHINE_ARCH= parts of the line.
		IFS=" "
		for frag in ${found}; do
			case "$frag" in
			MACHINE=*|MACHINE_ARCH=*)
				eval "$frag"
				;;
			esac
		done
		;;
	*)
		bomb "Unknown target MACHINE: ${MACHINE}"
		;;
	esac
}

# validatearch -- check that the MACHINE/MACHINE_ARCH pair is supported.
#
# Bombs if the pair is not supported.
#
validatearch()
{
	local IFS
	local line
	local foundpair=false foundmachine=false foundarch=false

	case "${MACHINE_ARCH}" in
	"")
		bomb "No MACHINE_ARCH provided. Use 'build.sh -m ${MACHINE} list-arch' to show options."
		;;
	esac

	IFS="${nl}"
	for line in ${valid_MACHINE_ARCH}; do
		line="${line%%#*}" # ignore comments
		line="$( IFS=" ${tab}" ; echo $line )" # normalise white space
		case "${line} " in
		" ")
			# skip blank lines or comment lines
			continue
			;;
		"MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH} "*)
			foundpair=true
			;;
		"MACHINE=${MACHINE} "*)
			foundmachine=true
			;;
		*"MACHINE_ARCH=${MACHINE_ARCH} "*)
			foundarch=true
			;;
		esac
	done

	case "${foundpair}:${foundmachine}:${foundarch}" in
	true:*)
		: OK
		;;
	*:false:*)
		bomb "Unknown target MACHINE: ${MACHINE}"
		;;
	*:*:false)
		bomb "Unknown target MACHINE_ARCH: ${MACHINE_ARCH}"
		;;
	*)
		bomb "MACHINE_ARCH '${MACHINE_ARCH}' does not support MACHINE '${MACHINE}'"
		;;
	esac
}

# listarch -- list valid MACHINE/MACHINE_ARCH/ALIAS values,
# optionally restricted to those where the MACHINE and/or MACHINE_ARCH
# match specifed glob patterns.
#
listarch()
{
	local machglob="$1" archglob="$2"
	local IFS
	local wildcard="*"
	local line xline frag
	local line_matches_machine line_matches_arch
	local found=false

	# Empty machglob or archglob should match anything
	: "${machglob:=${wildcard}}"
	: "${archglob:=${wildcard}}"

	IFS="${nl}"
	for line in ${valid_MACHINE_ARCH}; do
		line="${line%%#*}" # ignore comments
		xline="$( IFS=" ${tab}" ; echo $line )" # normalise white space
		[ -z "${xline}" ] && continue # skip blank or comment lines

		line_matches_machine=false
		line_matches_arch=false

		IFS=" "
		for frag in ${xline}; do
			case "${frag}" in
			MACHINE=${machglob})
				line_matches_machine=true ;;
			ALIAS=${machglob})
				line_matches_machine=true ;;
			MACHINE_ARCH=${archglob})
				line_matches_arch=true ;;
			esac
		done

		if $line_matches_machine && $line_matches_arch; then
			found=true
			echo "$line"
		fi
	done
	if ! $found; then
		echo >&2 "No match for" \
		    "MACHINE=${machglob} MACHINE_ARCH=${archglob}"
		return 1
	fi
	return 0
}

# valid_OBJECT_FMT -- A multi-line string, listing all valid
# FORMAT/OBJECT_FMT pairs.
#
valid_OBJECT_FMT='
FORMAT=a.out        OBJECT_FMT=a.out
FORMAT=ELF          OBJECT_FMT=ELF         DEFAULT
,

# getformat -- find the default OBJECT_FMT for a FORMAT.
#
# Sets FORMAT and OBJECT_FMT if the input MACHINE value is
# recognised as a machine that has a default
# OBJECT_FMT (or that has only one possible OBJECT_FMT).
#
# Leaves FORMAT and OBJECT_FMT unchanged if FORMAT is recognised
# as being associated with multiple OBJECT_FMT values with no default.
#
# Bombs if FORMAT is not recognised.
#
getformat()
{
    local IFS
	local found=""
	local line

    IFS="${nl}"
    makewrapperformat="${FORMAT}"
    for line in ${valid_OBJECT_FMT}; do
		line="${line%%#*}" # ignore comments
		line="$( IFS=" ${tab}" ; echo $line )" # normalise white space
		case "${line} " in
		" ")
			# skip blank lines or comment lines
			continue
			;;
		*" ALIAS=${FORMAT} "*)
			# Found a line with a matching ALIAS=<alias>.
			found="$line"
			break
			;;
		"FORMAT=${FORMAT} "*" NO_DEFAULT"*)
			# Found an explicit "NO_DEFAULT" for this FORMAT.
			found="$line"
			break
			;;
		"FORMAT=${FORMAT} "*" DEFAULT"*)
			# Found an explicit "DEFAULT" for this FORMAT.
			found="$line"
			break
			;;
		"FORMAT=${FORMAT} "*)
			# Found a line for this FORMAT.  If it's the
			# first such line, then tentatively accept it.
			# If it's not the first matching line, then
			# remember that there was more than one match.
			case "$found" in
			'')	found="$line" ;;
			*)	found="MULTIPLE_MATCHES" ;;
			esac
			;;
		esac
	done

	case "$found" in
	*NO_DEFAULT*|*MULTIPLE_MATCHES*)
		# FORMAT is OK, but OBJECT_FMT is still unknown
		return
		;;
	"FORMAT="*" OBJECT_FMT="*)
		# Obey the FORMAT= and OBJECT_FMT= parts of the line.
		IFS=" "
		for frag in ${found}; do
			case "$frag" in
			FORMAT=*|OBJECT_FMT=*)
				eval "$frag"
				;;
			esac
		done
		;;
	*)
		bomb "Unknown target FORMAT: ${FORMAT}"
		;;
	esac
}

# validateformat -- check that the FORMAT/OBJECT_FMT pair is supported.
#
# Bombs if the pair is not supported.
#
validateformat()
{
    local IFS
	local line
	local foundpair=false foundformat=false foundobj=false

	case "${FORMAT}" in
	"")
		bomb "No OBJECT_FMT provided. Use 'build.sh -f ${FORMAT} list-format' to show options."
		;;
	esac

	IFS="${nl}"
	for line in ${valid_OBJECT_FMT}; do
		line="${line%%#*}" # ignore comments
		line="$( IFS=" ${tab}" ; echo $line )" # normalise white space
		case "${line} " in
		" ")
			# skip blank lines or comment lines
			continue
			;;
		"FORMAT=${FORMAT} OBJECT_FMT=${OBJECT_FMT} "*)
			foundpair=true
			;;
		"FORMAT=${FORMAT} "*)
			foundformat=true
			;;
		*"FORMAT=${OBJECT_FMT} "*)
			foundobj=true
			;;
		esac
	done

	case "${foundpair}:${foundformat}:${foundobj}" in
	true:*)
		: OK
		;;
	*:false:*)
		bomb "Unknown target FORMAT: ${FORMAT}"
		;;
	*:*:false)
		bomb "Unknown target OBJECT_FMT: ${OBJECT_FMT}"
		;;
	*)
		bomb "OBJECT_FMT '${OBJECT_FMT}' does not support FORMAT '${FORMAT}'"
		;;
	esac
}

# nobomb_getmakevar --
# Given the name of a make variable in $1, print make's idea of the
# value of that variable, or return 1 if there's an error.
#
nobomb_getmakevar()
{
	[ -x "${make}" ] || return 1
	"${make}" -m ${TOP}/share/mk -s -B -f- _x_ <<EOF || return 1
_x_:
	echo \${$1}
.include <bsd.prog.mk>
.include <bsd.kernobj.mk>
EOF
}

# bomb_getmakevar --
# Given the name of a make variable in $1, print make's idea of the
# value of that variable, or bomb if there's an error.
#
bomb_getmakevar()
{
	[ -x "${make}" ] || bomb "bomb_getmakevar $1: ${make} is not executable"
	nobomb_getmakevar "$1" || bomb "bomb_getmakevar $1: ${make} failed"
}

# getmakevar --
# Given the name of a make variable in $1, print make's idea of the
# value of that variable, or print a literal '$' followed by the
# variable name if ${make} is not executable.  This is intended for use in
# messages that need to be readable even if $make hasn't been built,
# such as when build.sh is run with the "-n" option.
#
getmakevar()
{
	if [ -x "${make}" ]; then
		bomb_getmakevar "$1"
	else
		echo "\$$1"
	fi
}

setmakeenv()
{
	eval "$1='$2'; export $1"
	makeenv="${makeenv} $1"
}
safe_setmakeenv()
{
	case "$1" in

	#	Look for any vars we want to prohibit here, like:
	# Bad | Dangerous)	usage "Cannot override $1 with -V";;

	# That first char is OK has already been verified.
	*[!A-Za-z0-9_]*)	usage "Bad variable name (-V): '$1'";;
	esac
	setmakeenv "$@"
}

unsetmakeenv()
{
	eval "unset $1"
	makeenv="${makeenv} $1"
}
safe_unsetmakeenv()
{
	case "$1" in

	#	Look for any vars user should not be able to unset
	# Needed | Must_Have)	usage "Variable $1 cannot be unset";;

	[!A-Za-z_]* | *[!A-Za-z0-9_]*)	usage "Bad variable name (-Z): '$1'";;
	esac
	unsetmakeenv "$1"
}

# Given a variable name in $1, modify the variable in place as follows:
# For each space-separated word in the variable, call resolvepath.
resolvepaths()
{
	local var="$1"
	local val
	eval val=\"\${${var}}\"
	local newval=''
	local word
	for word in ${val}; do
		resolvepath word
		newval="${newval}${newval:+ }${word}"
	done
	eval ${var}=\"\${newval}\"
}

# Given a variable name in $1, modify the variable in place as follows:
# Convert possibly-relative path to absolute path by prepending
# ${TOP} if necessary.  Also delete trailing "/", if any.
resolvepath()
{
	local var="$1"
	local val
	eval val=\"\${${var}}\"
	case "${val}" in
	/)
		;;
	/*)
		val="${val%/}"
		;;
	*)
		val="${TOP}/${val%/}"
		;;
	esac
	eval ${var}=\"\${val}\"
}

usage()
{
	if [ -n "$*" ]; then
		echo ""
		echo "${progname}: $*"
	fi
	cat <<_usage_
Usage: ${progname} [-EhnoPRrUuxy] [-a arch] [-B buildid] [-C cdextras]
                [-c compiler] [-D dest] -f [format] [-j njob] [-M obj] [-m mach]
                [-N noisy] [-O obj] [-R release] [-S seed] [-T tools]
                [-V var=[value]] [-w wrapper] [-X x11src] [-Y extsrcsrc]
                [-Z var]
                operation [...]
 Build operations (all imply "obj" and "tools"):
    build               Run "make build".
    distribution        Run "make distribution" (includes DESTDIR/etc/ files).
    release             Run "make release" (includes kernels & distrib media).
 Other operations:
    help                Show this message and exit.
    makewrapper         Create ${toolprefix}make-\${MACHINE} wrapper and ${toolprefix}make.
                        Always performed.
    cleandir            Run "make cleandir".  [Default unless -u is used]
    dtb			Build devicetree blobs.
    obj                 Run "make obj".  [Default unless -o is used]
    tools               Build and install tools.
    install=idir        Run "make installworld" to \`idir' to install all sets
                        except \`etc'.  Useful after "distribution" or "release"
    kernel=conf         Build kernel with config file \`conf'
    kernel.gdb=conf     Build kernel (including netbsd.gdb) with config
                        file \`conf'
    releasekernel=conf  Install kernel built by kernel=conf to RELEASEDIR.
    kernels             Build all kernels
    installmodules=idir Run "make installmodules" to \`idir' to install all
                        kernel modules.
    modules             Build kernel modules.
    rumptest            Do a linktest for rump (for developers).
    sets                Create binary sets in
                        RELEASEDIR/RELEASEMACHINEDIR/binary/sets.
                        DESTDIR should be populated beforehand.
    distsets            Same as "distribution sets".
    sourcesets          Create source sets in RELEASEDIR/source/sets.
    syspkgs             Create syspkgs in
                        RELEASEDIR/RELEASEMACHINEDIR/binary/syspkgs.
    iso-image           Create CD-ROM image in RELEASEDIR/images.
    iso-image-source    Create CD-ROM image with source in RELEASEDIR/images.
    live-image          Create bootable live image in
                        RELEASEDIR/RELEASEMACHINEDIR/installation/liveimage.
    install-image       Create bootable installation image in
                        RELEASEDIR/RELEASEMACHINEDIR/installation/installimage.
    disk-image=target   Create bootable disk image in
                        RELEASEDIR/RELEASEMACHINEDIR/binary/gzimg/target.img.gz.
    params              Display various make(1) parameters.
    list-arch           Display a list of valid MACHINE/MACHINE_ARCH values,
                        and exit.  The list may be narrowed by passing glob
                        patterns or exact values in MACHINE or MACHINE_ARCH.
 Options:
    -a arch        Set MACHINE_ARCH to arch.  [Default: deduced from MACHINE]
    -B buildid     Set BUILDID to buildid.
    -C cdextras    Append cdextras to CDEXTRA variable for inclusion on CD-ROM.
    -c compiler    Select compiler:
                       clang
                       gcc
                   [Default: gcc]
    -D dest        Set DESTDIR to dest.  [Default: destdir.MACHINE]
    -E             Set "expert" mode; disables various safety checks.
                   Should not be used without expert knowledge of the build
                   system.
    -f			   Set FORMAT to mach.  Some mach values are actually
                   aliases that set FORMAT/OBJECT_FMT pairs.
                   [Default: deduced from the host system if the host
                   OS is NetBSD]
    -h             Print this help message.
    -j njob        Run up to njob jobs in parallel; see make(1) -j.
    -M obj         Set obj root directory to obj; sets MAKEOBJDIRPREFIX.
                   Unsets MAKEOBJDIR.
    -m mach        Set MACHINE to mach.  Some mach values are actually
                   aliases that set MACHINE/MACHINE_ARCH pairs.
                   [Default: deduced from the host system if the host
                   OS is NetBSD]
    -N noisy       Set the noisyness (MAKEVERBOSE) level of the build:
                       0   Minimal output ("quiet")
                       1   Describe what is occurring
                       2   Describe what is occurring and echo the actual
                           command
                       3   Ignore the effect of the "@" prefix in make commands
                       4   Trace shell commands using the shell's -x flag
                   [Default: 2]
    -n             Show commands that would be executed, but do not execute them.
    -O obj         Set obj root directory to obj; sets a MAKEOBJDIR pattern.
                   Unsets MAKEOBJDIRPREFIX.
    -o             Set MKOBJDIRS=no; do not create objdirs at start of build.
    -P             Set MKREPRO and MKREPRO_TIMESTAMP to the latest source
                   CVS timestamp for reproducible builds.
    -R release     Set RELEASEDIR to release.  [Default: releasedir]
    -r             Remove contents of TOOLDIR and DESTDIR before building.
    -S seed        Set BUILDSEED to seed.  [Default: NetBSD-majorversion]
    -T tools       Set TOOLDIR to tools.  If unset, and TOOLDIR is not set in
                   the environment, ${toolprefix}make will be (re)built
                   unconditionally.
    -U             Set MKUNPRIVED=yes; build without requiring root privileges,
                   install from an UNPRIVED build with proper file permissions.
    -u             Set MKUPDATE=yes; do not run "make cleandir" first.
                   Without this, everything is rebuilt, including the tools.
    -V var=[value] Set variable \`var' to \`value'.
    -w wrapper     Create ${toolprefix}make script as wrapper.
                   [Default: \${TOOLDIR}/bin/${toolprefix}make-\${MACHINE}]
    -X x11src      Set X11SRCDIR to x11src.  [Default: /usr/xsrc]
    -x             Set MKX11=yes; build X11 from X11SRCDIR
    -Y extsrcsrc   Set EXTSRCSRCDIR to extsrcsrc.  [Default: /usr/extsrc]
    -y             Set MKEXTSRC=yes; build extsrc from EXTSRCSRCDIR
    -Z var         Unset ("zap") variable \`var'.
_usage_
	exit 1
}

parseoptions()
{
	opts='a:B:C:c:D:Ehj:M:m:N:nO:oPR:rS:T:UuV:w:X:xY:yZ:'
	opt_a=false
	opt_m=false

	if type getopts >/dev/null 2>&1; then
		# Use POSIX getopts.
		#
		getoptcmd='getopts ${opts} opt && opt=-${opt}'
		optargcmd=':'
		optremcmd='shift $((${OPTIND} -1))'
	else
		type getopt >/dev/null 2>&1 ||
		    bomb "Shell does not support getopts or getopt"

		# Use old-style getopt(1) (doesn't handle whitespace in args).
		#
		args="$(getopt ${opts} $*)"
		[ $? = 0 ] || usage
		set -- ${args}

		getoptcmd='[ $# -gt 0 ] && opt="$1" && shift'
		optargcmd='OPTARG="$1"; shift'
		optremcmd=':'
	fi

	# Parse command line options.
	#
	while eval ${getoptcmd}; do
		case ${opt} in

		-a)
			eval ${optargcmd}
			MACHINE_ARCH=${OPTARG}
			opt_a=true
			;;

		-B)
			eval ${optargcmd}
			BUILDID=${OPTARG}
			;;

		-C)
			eval ${optargcmd}; resolvepaths OPTARG
			CDEXTRA="${CDEXTRA}${CDEXTRA:+ }${OPTARG}"
			;;

		-c)
			eval ${optargcmd}
			case "${OPTARG}" in
			gcc)	# default, no variables needed
				;;
			clang)	setmakeenv HAVE_LLVM yes
				setmakeenv MKLLVM yes
				setmakeenv MKGCC no
				;;
			#pcc)	...
			#	;;
			*)	bomb "Unknown compiler: ${OPTARG}"
			esac
			;;

		-D)
			eval ${optargcmd}; resolvepath OPTARG
			setmakeenv DESTDIR "${OPTARG}"
			;;

		-E)
			do_expertmode=true
			;;
			
			# -f overrides OBJECT_FMT unless "-a" is specified
		-f)
			eval ${optargcmd}
			FORMAT="${OPTARG}"
			opt_f=true
			;;
			

		-j)
			eval ${optargcmd}
			parallel="-j ${OPTARG}"
			;;

		-M)
			eval ${optargcmd}; resolvepath OPTARG
			case "${OPTARG}" in
			\$*)	usage "-M argument must not begin with '\$'"
				;;
			*\$*)	# can use resolvepath, but can't set TOP_objdir
				resolvepath OPTARG
				;;
			*)	resolvepath OPTARG
				TOP_objdir="${OPTARG}${TOP}"
				;;
			esac
			unsetmakeenv MAKEOBJDIR
			setmakeenv MAKEOBJDIRPREFIX "${OPTARG}"
			;;

			# -m overrides MACHINE_ARCH unless "-a" is specified
		-m)
			eval ${optargcmd}
			MACHINE="${OPTARG}"
			opt_m=true
			;;

		-N)
			eval ${optargcmd}
			case "${OPTARG}" in
			0|1|2|3|4)
				setmakeenv MAKEVERBOSE "${OPTARG}"
				;;
			*)
				usage "'${OPTARG}' is not a valid value for -N"
				;;
			esac
			;;

		-n)
			runcmd=echo
			;;

		-O)
			eval ${optargcmd}
			case "${OPTARG}" in
			*\$*)	usage "-O argument must not contain '\$'"
				;;
			*)	resolvepath OPTARG
				TOP_objdir="${OPTARG}"
				;;
			esac
			unsetmakeenv MAKEOBJDIRPREFIX
			setmakeenv MAKEOBJDIR "\${.CURDIR:C,^$TOP,$OPTARG,}"
			;;

		-o)
			MKOBJDIRS=no
			;;

		-P)
			MKREPRO=yes
			;;

		-R)
			eval ${optargcmd}; resolvepath OPTARG
			setmakeenv RELEASEDIR "${OPTARG}"
			;;

		-r)
			do_removedirs=true
			do_rebuildmake=true
			;;

		-S)
			eval ${optargcmd}
			setmakeenv BUILDSEED "${OPTARG}"
			;;

		-T)
			eval ${optargcmd}; resolvepath OPTARG
			TOOLDIR="${OPTARG}"
			export TOOLDIR
			;;

		-U)
			setmakeenv MKUNPRIVED yes
			;;

		-u)
			setmakeenv MKUPDATE yes
			;;

		-V)
			eval ${optargcmd}
			case "${OPTARG}" in
		    # XXX: consider restricting which variables can be changed?
			[a-zA-Z_]*=*)
				safe_setmakeenv "${OPTARG%%=*}" "${OPTARG#*=}"
				;;
			[a-zA-Z_]*)
				safe_setmakeenv "${OPTARG}" ""
				;;
			*)
				usage "-V argument must be of the form 'var[=value]'"
				;;
			esac
			;;

		-w)
			eval ${optargcmd}; resolvepath OPTARG
			makewrapper="${OPTARG}"
			;;

		-X)
			eval ${optargcmd}; resolvepath OPTARG
			setmakeenv X11SRCDIR "${OPTARG}"
			;;

		-x)
			setmakeenv MKX11 yes
			;;

		-Y)
			eval ${optargcmd}; resolvepath OPTARG
			setmakeenv EXTSRCSRCDIR "${OPTARG}"
			;;

		-y)
			setmakeenv MKEXTSRC yes
			;;

		-Z)
			eval ${optargcmd}
		    # XXX: consider restricting which variables can be unset?
			safe_unsetmakeenv "${OPTARG}"
			;;

		--)
			break
			;;

		-'?'|-h)
			usage
			;;

		esac
	done

	# Validate operations.
	#
	eval ${optremcmd}
	while [ $# -gt 0 ]; do
		op=$1; shift
		operations="${operations} ${op}"

		case "${op}" in

		help)
			usage
			;;

		list-arch)
			listarch "${MACHINE}" "${MACHINE_ARCH}"
			exit $?
			;;
		 	
		kernel=*|releasekernel=*|kernel.gdb=*)
			arg=${op#*=}
			op=${op%%=*}
			[ -n "${arg}" ] ||
			    bomb "Must supply a kernel name with \`${op}=...'"
			;;

		disk-image=*)
			arg=${op#*=}
			op=disk_image
			[ -n "${arg}" ] ||
			    bomb "Must supply a target name with \`${op}=...'"

			;;

		install=*|installmodules=*)
			arg=${op#*=}
			op=${op%%=*}
			[ -n "${arg}" ] ||
			    bomb "Must supply a directory with \`install=...'"
			;;

		distsets)
			operations="$(echo "$operations" | sed 's/distsets/distribution sets/')"
			do_sets=true
			op=distribution
			;;

		build|\
		cleandir|\
		distribution|\
		dtb|\
		install-image|\
		iso-image-source|\
		iso-image|\
		kernels|\
		libs|\
		live-image|\
		makewrapper|\
		modules|\
		obj|\
		params|\
		release|\
		rump|\
		rumptest|\
		sets|\
		sourcesets|\
		syspkgs|\
		tools)
			;;

		*)
			usage "Unknown operation \`${op}'"
			;;

		esac
		# ${op} may contain chars that are not allowed in variable
		# names.  Replace them with '_' before setting do_${op}.
		op="$( echo "$op" | tr -s '.-' '__')"
		eval do_${op}=true
	done
	[ -n "${operations}" ] || usage "Missing operation to perform."

	# Set up MACHINE*.  On a 211BSD host, these are allowed to be unset.
	#
	if [ -z "${MACHINE}" ]; then
		[ "${uname_s}" = "211BSD" ] ||
		    bomb "MACHINE must be set, or -m must be used, for cross builds."
		MACHINE=${uname_m}
		MACHINE_ARCH=${uname_p}
	fi
	if $opt_m && ! $opt_a; then
		# Settings implied by the command line -m option
		# override MACHINE_ARCH from the environment (if any).
		getarch
	fi
	[ -n "${MACHINE_ARCH}" ] || getarch
	validatearch
	fi
	if  $opt_f && ! $opt_a; then
		# Settings implied by the command line -m option
		# override OBJECT_FMT from the environment (if any).
		getformat
	fi
	[ -f "${OBJECT_FMT}" ] || getformat
	validateformat

	# Set up default make(1) environment.
	#
	makeenv="${makeenv} TOOLDIR MACHINE MACHINE_ARCH MAKEFLAGS"
	[ -z "${BUILDID}" ] || makeenv="${makeenv} BUILDID"
	[ -z "${BUILDINFO}" ] || makeenv="${makeenv} BUILDINFO"
	MAKEFLAGS="-de -m ${TOP}/share/mk ${MAKEFLAGS}"
	MAKEFLAGS="${MAKEFLAGS} MKOBJDIRS=${MKOBJDIRS-yes}"
	export MAKEFLAGS MACHINE MACHINE_ARCH
	setmakeenv USETOOLS "yes"
	setmakeenv MAKEWRAPPERMACHINE "${makewrappermachine:-${MACHINE}}"
	setmakeenv MAKEWRAPPERFORMAT "${makewrapperformat:-${FORMAT}}"
	setmakeenv MAKE_OBJDIR_CHECK_WRITABLE no
}