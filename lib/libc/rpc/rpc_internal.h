/*	$NetBSD: rpc_internal.h,v 1.3 2003/09/09 00:22:17 itojun Exp $	*/

/*
 * Private include file for XDR functions only used internally in libc.
 * These are not exported interfaces.
 */

bool_t __xdrrec_getrec(XDR *, enum xprt_stat *, bool_t);
bool_t __xdrrec_setnonblock(XDR *, int);
void __xprt_unregister_unlocked(SVCXPRT *);
bool_t __svc_clean_idle(fd_set *, int, bool_t);

bool_t __xdrrec_getrec(XDR *, enum xprt_stat *, bool_t);
bool_t __xdrrec_setnonblock(XDR *, int);

u_int __rpc_get_a_size(int);
int __rpc_dtbsize(void);
struct netconfig * __rpcgettp(int);
int  __rpc_get_default_domain(char **);

char *__rpc_taddr2uaddr_af(int, const struct netbuf *);
struct netbuf *__rpc_uaddr2taddr_af(int, const char *);
int __rpc_fixup_addr(struct netbuf *, const struct netbuf *);
int __rpc_sockinfo2netid(struct __rpc_sockinfo *, const char **);
int __rpc_seman2socktype(int);
int __rpc_socktype2seman(int);
void *rpc_nullproc(CLIENT *);
int __rpc_sockisbound(int);

struct netbuf *__rpcb_findaddr(rpcprog_t, rpcvers_t,
				    const struct netconfig *,
				    const char *, CLIENT **);
bool_t __rpc_control(int, void *);

char *_get_next_token(char *, int);

u_int32_t __rpc_getxid(void);
#define __RPC_GETXID()	(__rpc_getxid())

extern SVCXPRT **__svc_xports;
extern int __svc_maxrec;
