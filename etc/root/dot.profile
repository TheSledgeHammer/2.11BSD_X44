#	$NetBSD: dot.profile,v 1.32 2020/08/24 12:46:57 nia Exp $

export PATH=/sbin:/bin:/etc:/usr/sbin:/usr/bin:/usr/ucb
export PATH=${PATH}:/usr/X11R7/bin:/usr/local/sbin:/usr/local/bin

# Uncomment the following line(s) to install binary packages


export BLOCKSIZE=1k

export HOST="$(hostname)"

umask 022
#ulimit -c 0

export ENV=/root/.shrc

# Do not display in 'su -' case
if [ -z "$SU_FROM" ]; then
        echo "We recommend that you create a non-root account and use su(1) for root access."
fi