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

#include "ctimed.h"

#define	CTIME			CTIME_VAL
#define	ASCTIME			ASCTIME_VAL
#define	TZSET			TZSET_VAL
#define	LOCALTIME 		LOCALTIME_VAL
#define	GMTIME			GMTIME_VAL
#define	OFFTIME			OFFTIME_VAL
#define GETPWENT        GETPWENT_VAL
#define GETPWNAM        GETPWNAM_VAL
#define GETPWUID        GETPWUID_VAL
#define SETPASSENT      SETPASSENT_VAL
#define ENDPWENT        ENDPWENT_VAL

static int R[2], W[2];
static char	result[256 + 4];
static struct tm tmtmp;
static int inited;
struct passwd _pw;

char *ctime_stub(const time_t *);
char *asctime_stub(const struct tm *);
void tzset_stub(void);
struct tm *localtime_stub(const time_t *);
struct tm *gmtime_stub(const time_t *);
struct tm *offtime_stub(const time_t *, long);
struct passwd *getpwent_stub(void);
struct passwd *getpwnam_stub(const char *);
struct passwd *getpwuid_stub(uid_t uid);
void setpwent_stub(void);
int setpassent_stub(int);
void endpwent_stub(void);
void setpwfile_stub(const char *);

void getb(int, void *, size_t);
struct passwd *getandfixpw(void);
void sewer(char , int);
void XXctime(void);

struct cmd_ops stub_ops = {
		.cmd_ctime = ctime_stub,
		.cmd_asctime = asctime_stub,
		.cmd_tzset = tzset_stub,
		.cmd_localtime = localtime_stub,
		.cmd_gmtime = gmtime_stub,
		.cmd_offtime = offtime_stub,
		.cmd_getpwent = getpwent_stub,
		.cmd_getpwnam = getpwnam_stub,
		.cmd_getpwuid = getpwuid_stub,
		.cmd_setpwent = setpwent_stub,
		.cmd_setpassent = setpassent_stub,
		.cmd_endpwent = endpwent_stub,
};

char *
ctime_stub(const time_t *t)
{
	u_char	fnc = CTIME;

	sewer('c', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, t, sizeof (*t));
	getb(RECV_FD, result, 26);
	return (result);
}

char *
asctime_stub(const struct tm *tp)
{
	u_char	fnc = ASCTIME;

	sewer('a', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, tp, sizeof (*tp));
	getb(RECV_FD, result, 26);
	return (result);
}

void
tzset_stub(void)
{
	u_char	fnc = TZSET;

	sewer('t', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
}

struct tm *
localtime_stub(const time_t *tp)
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
gmtime_stub(const time_t *tp)
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
offtime_stub(const time_t *clock, long offset)
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
getpwent_stub(void)
{
	u_char	fnc = GETPWENT;

	sewer('e', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	return (getandfixpw());
}

struct passwd *
getpwnam_stub(const char *nam)
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
getpwuid_stub(uid_t uid)
{
	u_char	fnc = GETPWUID;

	sewer('u', fnc);
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, &uid, sizeof (uid_t));
	return (getandfixpw());
}

void
setpwent_stub(void)
{
	(void)setpassent(0);
}

int
setpassent_stub(int stayopen)
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
endpwent_stub(void)
{
	u_char	fnc = ENDPWENT;

	sewer('f', fnc);
	write(SEND_FD, &fnc, sizeof (fnc));
	return;
}

/* setpwfile() is deprecated */
void
setpwfile_stub(const char *file)
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
	register int pid, ourpid;
    struct cmds *cmds;
    
	ourpid = getpid();

    cmds = ctimed_cmds_get(arg, func);
    if (cmds == NULL) {
        return;
    }
	if (inited == ourpid) {
		return;
	}
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
		(void)execl(_PATH_CTIMED, "ctimed", (char *)NULL);
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
