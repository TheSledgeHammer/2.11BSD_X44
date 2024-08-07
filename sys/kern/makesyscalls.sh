#! /bin/sh -
#
#	@(#)makesyscalls.sh	8.2 (Berkeley) 2/14/95

set -e

case $# in
    2)	;;
    *)	echo "Usage: $0 config-file input-file" 1>&2
		exit 1
		;;
esac

# the config file sets the following variables:
#	sysnames	the syscall names file
#	sysnumhdr	the syscall numbers file
#	sysproto	the syscall declarations file
#	syssw		the syscall switch file
#	compatopts	those syscall types that are for 'compat' syscalls
#	switchname	the name for the 'struct sysent' we define
#	namesname	the name for the 'char *[]' we define
#	constprefix	the prefix for the system call constants
#
# NOTE THAT THIS makesyscalls.sh DOES NOT SUPPORT 'LIBCOMPAT'.

# source the config file.
case $1 in
/*)	. $1;;
*)	. ./$1;;
esac

# tmp files:
sysdcl="sysent.dcl"
syscompat_pref="sysent."
sysent="sysent.switch"

syscompat_files=""
for file in $compatopts; do
	syscompat_files="$syscompat_files $syscompat_pref$file"
done

trap "rm $sysdcl $syscompat_files $sysent" 0

# Awk program (must support nawk extensions)
# Use "awk" at Berkeley, "nawk" or "gawk" elsewhere.
awk=${AWK:-awk}

# Does this awk have a "toupper" function?
isgawk="$($awk 'BEGIN { print toupper("true"); exit; }' 2>/dev/null)"

# If this awk does not define "toupper" then define our own.
if [ "$isgawk" = TRUE ] ; then
	# Used awk (GNU awk or nawk) provides it
	toupper=
else
	# Provide our own toupper()
	toupper='
function toupper(str) {
	_toupper_cmd = "echo "str" |tr a-z A-Z"
	_toupper_cmd | getline _toupper_str;
	close(_toupper_cmd);
	return _toupper_str;
}'
fi

# before handing it off to awk, make a few adjustments:
#	(1) insert spaces around {, }, (, ), *, and commas.
#	(2) get rid of any and all dollar signs (so that rcs id use safe)
#
# The awk script will deal with blank lines and lines that
# start with the comment character (';').

sed -e '
s/\$//g
:join
	/\\$/{a\
	N
	s/\\\n//
	b join
	}
2,${
	/^#/!s/\([{}()*,]\)/ \1 /g
}
' < $2 | $awk "
$toupper
BEGIN {
	sysnames = \"$sysnames\"
	sysnumhdr = \"$sysnumhdr\"
	sysproto = \"$sysproto\"
	switchname = \"$switchname\"
	namesname = \"$namesname\"
	constprefix = \"$constprefix\"
	
	sysdcl = \"$sysdcl\"
	syscompat_pref = \"$syscompat_pref\"
	sysent = \"$sysent\"
	infile = \"$2\"

	compatopts = \"$compatopts\"
	"'
	
	printf "/* \$211BSD\$ */\n\n" > sysdcl
	printf "/*\n * System call switch table.\n *\n" > sysdcl
	printf " * DO NOT EDIT-- this file is automatically generated.\n" > sysdcl
	
	ncompat = split(compatopts,compat)
	for (i = 1; i <= ncompat; i++) {
		compat_upper[i] = toupper(compat[i])
		compat_file[i] = sprintf("%s%s", syscompat_pref, compat[i])
		
		printf "\n#ifdef %s\n", compat_upper[i] > compat_file[i]
		printf "#define %s(func) __CONCAT(%s_,func)\n\n", \
		    compat[i], compat[i] > compat_file[i]
	}
	
	printf "/*\t\$211BSD\$\t*/\n\n" > sysnames
	printf "/*\n * System call names.\n *\n" > sysnames
	printf " * DO NOT EDIT-- this file is automatically generated.\n" > sysnames
	
	printf "/* \$211BSD\$ */\n\n" > sysnumhdr
	printf "/*\n * System call numbers.\n *\n" > sysnumhdr
	printf " * DO NOT EDIT-- this file is automatically generated.\n" > sysnumhdr

	printf "/* \$211BSD\$\t*/\n\n" > sysproto
	printf "/*\n * System call declarations.\n *\n" > sysproto
	printf " * DO NOT EDIT-- this file is automatically generated.\n" > sysproto
}
NR == 1 {
	printf " * created from%s\n */\n\n", $0 > sysdcl
	
	printf "#define\ts(type)\tsizeof(type)\n\n" > sysent
	printf "struct sysent %s[] = {\n",switchname > sysent
	
	printf " * created from%s\n */\n\n", $0 > sysnames
	printf "char *%s[] = {\n",namesname > sysnames
	
	printf " * created from%s\n */\n\n", $0 > sysnumhdr
	
	printf " * created from%s\n */\n\n", $0 > sysproto

	next
}
NF == 0 || $1 ~ /^;/ {
	next
}
$1 ~ /^#[ 	]*include/ {
	print > sysdcl
	next
}
$1 ~ /^#[ 	]*if/ {
	print > sysent
	print > sysdcl
	for (i = 1; i <= ncompat; i++)
		print > compat_file[i]
	print > sysnames
	savesyscall = syscall
	next
}
$1 ~ /^#[ 	]*else/ {
	print > sysent
	print > sysdcl
	for (i = 1; i <= ncompat; i++)
		print > compat_file[i]
	print > sysnames
	syscall = savesyscall
	next
}
$1 ~ /^#/ {
	print > sysent
	print > sysdcl
	for (i = 1; i <= ncompat; i++)
		print > compat_file[i]
	print > sysnames
	next
}
syscall != $1 {
	printf "%s: line %d: syscall number out of sync at %d\n", \
	   infile, NR, syscall
	printf "line is:\n"
	print
	exit 1
}
function parserr(was, wanted) {
	printf "%s: line %d: unexpected %s (expected %s)\n", \
	    infile, NR, was, wanted
	exit 1
}
function parseline() {
	f=3						# toss number and type
	if ($NF != "}") {
		funcalias=$NF
		end=NF-1
	} else {
		funcalias=""
		end=NF
	}
	if ($f != "{")
		parserr($f, "{")
	f++
	if ($end != "}")
		parserr($end, "}")
	end--
	if ($end != ";")
		parserr($end, ";")
	end--
	if ($end != ")")
		parserr($end, ")")
	end--
	
	f++						# toss return type
	
	funcname=$f
	if (funcalias == "")
		funcalias=funcname
	f++
	
	if ($f != "(")
		parserr($f, ")")
	f++
	
	argc= 0;
	if (f == end) {
		if ($f != "void")
			parserr($f, "argument definition")
		return
	}
	
	while (f <= end) {
		argc++
		argtype[argc]=""
		oldf=""
		while (f < end && $(f+1) != ",") {
			if (argtype[argc] != "" && oldf != "*")
				argtype[argc] = argtype[argc]" ";
			argtype[argc] = argtype[argc]$f;
			oldf = $f;
			f++
		}
		if (argtype[argc] == "")
			parserr($f, "argument definition")
		argname[argc]=$f;
		f += 2;			# skip name, and any comma
	}
}
function putent(nodefs, declfile, compatwrap) {
	# output syscall declaration for switch table
	if (nodefs != "INDIR") {
		prototype = "()"
		if (compatwrap == "")
			printf("int\t%s();\n", funcname) > declfile
		else
			printf("int\t%s(%s)();\n", compatwrap, funcname) > declfile
	}
	# output syscall switch entry
	if (nodefs == "INDIR") {
		printf("\t{ 0, 0, %s,\n\t    nosys },\t\t\t/* %d = %s */\n", \
		    sycall_flags, syscall, funcalias) > sysent
	} else {
		printf("\t{ %d, ", argc) > sysent
		if (argc == 0)
			printf("0") > sysent
		else if (compatwrap == "")
			printf("s(struct %s_args)", funcname) > sysent
		else
			printf("s(struct %s_%s_args)", compatwrap, funcname) > sysent
		if (compatwrap == "")
			wfn = sprintf("%s", funcname);
		else
			wfn = sprintf("%s(%s)", compatwrap, funcname);
		printf(",\n\t    %s },", wfn) > sysent
		for (i = 0; i < (33 - length(wfn)) / 8; i++)
			printf("\t") > sysent
		if (compatwrap == "")
			printf("/* %d = %s */\n", syscall, funcalias) > sysent
		else
			printf("/* %d = %s %s */\n", syscall, compatwrap, funcalias) > sysent
	}

	# output syscall name for names table
	if (compatwrap == "")
		printf("\t\"%s\",\t\t\t/* %d = %s */\n", funcalias, syscall,
		    funcalias) > sysnames
	else
		printf("\t\"%s_%s\",\t/* %d = %s %s */\n", compatwrap,
		    funcalias, syscall, compatwrap, funcalias) > sysnames
		    
	# output syscall number of header, if appropriate
	if (nodefs == "" || nodefs == "NOARGS" || nodefs == "INDIR")
		printf("#define\t%s%s\t%d\n", constprefix, funcalias,
		    syscall) > sysnumhdr
	else if (nodefs != "NODEF")
		printf("\t\t\t\t/* %d is %s %s */\n", syscall,
		    compatwrap, funcalias) > sysnumhdr

	# output syscall prototype declarations
	if (compatwrap == "")
		printf("extern int\t%s();\n", funcname) > sysproto
	else
		printf("extern int\t%s_%s();\n", compatwrap, funcname) > sysproto
}
$2 == "STD" {
	parseline()
	putent("", sysdcl, "")
	syscall++
	next
}
$2 == "NODEF" || $2 == "NOARGS" || $2 == "INDIR" {
	parseline()
	putent($2, sysdcl, "")
	syscall++
	next
}
$2 == "OBSOL" || $2 == "UNIMPL" {
	if ($2 == "OBSOL")
		comment="obsolete"
	else
		comment="unused"
	for (i = 3; i <= NF; i++)
		comment=comment " " $i
		
	printf("\t{ 0, 0,\n\t    nosys },\t\t\t\t/* %d = %s */\n", \
	    syscall, comment) > sysent
	printf("\t\"#%d\",\t\t/* %d = %s */\n", \
	    syscall, syscall, comment) > sysnames
	if ($2 != "UNIMPL")
		printf("\t\t\t\t/* %d is %s */\n", syscall, comment) > sysnumhdr
	syscall++
	next
}
{
	for (i = 1; i <= ncompat; i++) {
		if ($2 == compat_upper[i]) {
			parseline();
			putent("COMMENT", compat_file[i], compat[i])
			syscall++
			next
		}
	}
	printf "%s: line %d: unrecognized keyword %s\n", infile, NR, $2
	exit 1
}
END {
	for (i = 1; i <= ncompat; i++) {
		printf("\n#else /* %s */\n", compat_upper[i]) > compat_file[i]
		printf("#define %s(func) nosys\n", compat[i]) > \
		    compat_file[i]
		printf("#endif /* %s */\n\n", compat_upper[i]) > compat_file[i]
    }
    
	printf("};\n\n") > sysent
	printf("int\tn%s= sizeof(%s) / sizeof(%s[0]);\n", switchname, switchname, switchname) > sysent
	printf("};\n") > sysnames
	printf("#define\t%sMAXSYSCALL\t%d\n", constprefix, syscall) > sysnumhdr
} '

cat $sysdcl $sysent > $syssw

# $syscompat_files
#chmod 444 $sysnames $syshdr $syssw
