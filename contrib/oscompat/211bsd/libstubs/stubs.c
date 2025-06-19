#include <sys/cdefs.h>

#include <sys/types.h>

#include <time.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <utmp.h>
#include <varargs.h>

#define	SEND_FD	W[1]
#define	RECV_FD	R[0]

#define	CTIME			1
#define	ASCTIME			2
#define	TZSET			3
#define	LOCALTIME 		4
#define	GMTIME			5
#define	OFFTIME			6

#define GETPWENT        7
#define GETPWNAM        8
#define GETPWUID        9
#define SETPASSENT      10
#define ENDPWENT        11

static int R[2], W[2], inited;
static char	result[256 + 4];
static struct tm tmtmp;
static struct passwd _pw;

/* ctimed stubs command table */

struct commands {
    char  arg;
    int   val;
} command[] = {
    { .arg = 'c', .val = CTIME },
    { .arg = 'a', .val = ASCTIME },
    { .arg = 't', .val = TZSET },
    { .arg = 'l', .val = LOCALTIME },
    { .arg = 'g', .val = GMTIME },
    { .arg = 'o', .val = OFFTIME },
    { .arg = 'e', .val = GETPWENT },
    { .arg = 'n', .val = GETPWNAM },
    { .arg = 'u', .val = GETPWUID },
    { .arg = 'p', .val = SETPASSENT },
    { .arg = 'f', .val = ENDPWENT },
};

struct passwd *getandfixpw(void);
void getb(int, void *, size_t);
void sewer(char , int);
void XXctime(void);
void ctimed_commands(char , int);
void ctimed_usage(void);

char *
ctime(const time_t *t)
{
	u_char	fnc = CTIME;

	sewer('c', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, t, sizeof (*t));
	getb(RECV_FD, result, 26);
	return (result);
}

char *
asctime(const struct tm *tp)
{
	u_char	fnc = ASCTIME;

	sewer('a', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, tp, sizeof (*tp));
	getb(RECV_FD, result, 26);
	return (result);
}

void
tzset(void)
{
	u_char	fnc = TZSET;

	sewer('t', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
}

struct tm *
localtime(const time_t *tp)
{
	u_char	fnc = LOCALTIME;

	sewer('l', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, tp, sizeof (*tp));
	getb(RECV_FD, &tmtmp, sizeof tmtmp);
	getb(RECV_FD, result, 24);
	tmtmp.tm_zone = result;
	return (&tmtmp);
}

struct tm *
gmtime(const time_t *tp)
{
	u_char	fnc = GMTIME;

	sewer('g', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, tp, sizeof (*tp));
	getb(RECV_FD, &tmtmp, sizeof tmtmp);
	getb(RECV_FD, result, 24);
	tmtmp.tm_zone = result;
	return(&tmtmp);
}

struct tm *
offtime(const time_t *clock, long offset)
{
	u_char	fnc = OFFTIME;

	sewer('o', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, clock, sizeof (*clock));
	write(SEND_FD, &offset, sizeof offset);
	getb(RECV_FD, &tmtmp, sizeof tmtmp);
	tmtmp.tm_zone = NULL;
	return (&tmtmp);
}

struct passwd *
getpwent(void)
{
	u_char	fnc = GETPWENT;

	sewer('e', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	return (getandfixpw());
}

struct passwd *
getpwnam(const char *nam)
{
	u_char	fnc = GETPWNAM;
	char lnam[UT_NAMESIZE + 1];
	int	len;

	len = strlen(nam);
	if	(len > UT_NAMESIZE) {
		len = UT_NAMESIZE;
    }
	bcopy(nam, lnam, len);
	lnam[len] = '\0';

	sewer('n', fnc);
	write(SEND_FD, &fnc, 1);
	write(SEND_FD, &len, sizeof (int));
	write(SEND_FD, lnam, len);
	return (getandfixpw());
}

struct passwd *
getpwuid(uid_t uid)
{
	u_char	fnc = GETPWUID;

	sewer('u', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, &uid, sizeof (uid_t));
	return (getandfixpw());
}

void
setpwent(void)
{
	(void)setpassent(0);
}

int
setpassent(int stayopen)
{
	u_char	fnc = SETPASSENT;
	int	sts;

	sewer('p', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, &stayopen, sizeof (int));
	getb(RECV_FD, &sts, sizeof (int));
	return (sts);
}

void
endpwent(void)
{
	u_char	fnc = ENDPWENT;

	sewer('f', fnc);
	write(SEND_FD, &fnc, sizeof (fnc));
	return;
}

/* setpwfile() is deprecated */
void
setpwfile(const char *file)
{
	return;
}

struct passwd *
getandfixpw(void)
{
	short	sz;

	getb(RECV_FD, &sz, sizeof(int));
	if (sz == 0) {
		return (NULL);
	}
	getb(RECV_FD, &_pw, sizeof(_pw));
	getb(RECV_FD, result, sz);
	_pw.pw_name += (int) result;
	_pw.pw_passwd += (int) result;
	_pw.pw_class += (int) result;
	_pw.pw_gecos += (int) result;
	_pw.pw_dir += (int) result;
	_pw.pw_shell += (int) result;
	return (&_pw);
}

void
getb(int f, void *p, size_t n)
{
	int i;
    char *b;

    b = (char *)p;
	while (n) {
		i = read(f, b, n);
		if (i <= 0) {
			return;
        }
		b += i;
		n -= (size_t)i;
	}
}

void
sewer(char arg, int func)
{
	register int pid, ourpid = getpid();

	if (inited == ourpid)
		return;
	if (inited) {
		close(SEND_FD);
		close(RECV_FD);
	}
	pipe(W);
	pipe(R);
	pid = vfork();
	if (pid == 0) { 	/* child */
		alarm(0); 		/* cancel alarms */
		dup2(W[0], 0); 	/* parent write side to our stdin */
		dup2(R[1], 1); 	/* parent read side to our stdout */
		close(SEND_FD); /* copies made, close the... */
		close(RECV_FD); /* originals now */
		ctimed_commands(arg, func);
		_exit(EX_OSFILE);
	}
	if (pid == -1) {
		abort(); 		/* nothing else really to do */
	}
	close(W[0]); 		/* close read side of SEND channel */
	close(R[1]); 		/* close write side of RECV channel */
	inited = ourpid; 	/* don't do this again in this proc */
}

void
XXctime(void)
{
	if (SEND_FD) {
		close(SEND_FD);
	}
	if (RECV_FD) {
		close(RECV_FD);
	}
	SEND_FD = RECV_FD = 0;
	inited = 0;
}

/* ctimed fix: allows stubs to run as expected */
void
ctimed_commands(char arg, int val)
{
	struct commands *cmd;

    cmd = &command[val];
    if (cmd != NULL) {
		if ((cmd->arg == arg) && (cmd->val == val)) {
			(void)execl(_PATH_CTIMED, "ctimed", cmd->arg, (char *)NULL);
		}
	}
}

void
ctimed_usage(void)
{
	(void)fprintf(stderr, "usage: ctimed stubs [-c ctime] [-a asctime] [-t tzet] [-l localtime] [-g gmtime] [-o offtime]\n");
	(void)fprintf(stderr, "usage: ctimed stubs [-e getpwent] [-n getpwnam] [-u getpwuid] [-p setpassent ] [-f endpwent]\n");
}
