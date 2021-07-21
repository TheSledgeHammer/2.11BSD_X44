
#	$NetBSD: dot.cshrc,v 1.29 2020/07/02 11:08:55 jdolecek Exp $

alias	h		history
alias	j		jobs -l
alias	hup		'( set pid=$< ; kill -HUP $pid ) < /var/run/\!$.pid'
alias	la		ls -a
alias	lf		ls -FA
alias	ll		ls -l
alias	mail 	/usr/ucb/Mail

alias	x		exit
alias	z		suspend

alias	back	'set back="$old"; set old="$cwd"; cd "$back"; unset back; dirs'
alias	cd		'set old="$cwd"; chdir \!*'
alias	pd		pushd
alias	pd2		pushd +2
alias	pd3		pushd +3
alias	pd4		pushd +4
alias	tset	'set noglob histchars=""; eval `\tset -s \!*`; unset noglob histchars'

setenv BLOCKSIZE 1k

# Uncomment the following line(s) to install binary packages

set history=1000
set path=(/sbin /bin /etc /usr/sbin /usr/bin /usr/ucb /usr/contrib/bin /usr/X11R7/bin /usr/local/sbin /usr/local/bin)

# directory stuff: cdpath/cd/back
set cdpath=(/usr/src/{sys,bin,sbin,usr.{bin,lib,sbin},lib,libexec,share,contrib,ucb,local,games})

if ($?prompt && -x /usr/bin/id ) then
	if (`/usr/bin/id -u` == 0) then
		set prompt="`hostname -s`# "
	else
		set prompt="`hostname -s`% "
	endif
endif

umask 022
