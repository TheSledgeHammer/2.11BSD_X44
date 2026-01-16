#	$211BSD_X44: Makefile,v 1.27 2025/05/26 17:33:59 martin Exp $
#	$NetBSD: Makefile,v 1.27 2019/04/20 17:16:41 christos Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIB=	ssh

# SRCS built from /usr/src/crypto/openssh/Makefile.in,
#	XMSS_OBJS
#	LIBOPENSSH_OBJS
#	LIBSHS_OBJS
#	+ ssh-sk-client.c
#
#	ssh-sk-client.c is used by sshkey.c from LIBOPENSSH_OBJS and had
#	to be included in libssh in order for initrd rescue to build.
#
XMSS_SRCS= \
        ssh-xmss.c \
        sshkey-xmss.c \
        xmss_commons.c \
        xmss_fast.c \
        xmss_hash.c \
        xmss_hash_address.c \
        xmss_wots.c \

LIBOPENSSH_SRCS= \
        ssh_api.c \
        ssherr.c \
        sshbuf.c \
        sshkey.c \
        sshbuf-getput-basic.c \
        sshbuf-misc.c \
        sshbuf-getput-crypto.c \
        krl.c \
        bitmap.c

LIBSSH_SRCS= \
        authfd.c authfile.c \
        canohost.c channels.c cipher.c cipher-aes.c cipher-aesctr.c \
	    cleanup.c \
    	compat.c fatal.c hostfile.c groupaccess.c \
	    log.c match.c moduli.c nchan.c packet.c \
    	readpass.c ttymodes.c xmalloc.c addr.c addrmatch.c \
	    atomicio.c dispatch.c mac.c misc.c utf8.c \
    	monitor_fdpass.c rijndael.c ssh-dss.c ssh-ecdsa.c ssh-ecdsa-sk.c \
	    ssh-ed25519-sk.c ssh-rsa.c dh.c \
	    msg.c progressmeter.c dns.c entropy.c gss-genr.c umac.c umac128.c \
	    ssh-pkcs11.c smult_curve25519_ref.c \
	    poly1305.c chacha.c cipher-chachapoly.c cipher-chachapoly-libcrypto.c \
	    ssh-ed25519.c digest-openssl.c digest-libc.c \
	    hmac.c ed25519.c hash.c \
	    kex.c kex-names.c kexdh.c kexgex.c kexecdh.c kexc25519.c \
	    kexgexc.c kexgexs.c \
	    kexsntrup761x25519.c kexmlkem768x25519.c sntrup761.c kexgen.c \
	    sftp-realpath.c platform.c platform-pledge.c platform-tracing.c platform-misc.c \
	    sshbuf-io.c uidswap.c

SK_SRCS= \
        ssh-sk-client.c

OPENBSD_COMPAT_SRCS= \
        base64.c \
        bcrypt_pbkdf.c \
        blowfish.c \
        bsd-closefrom.c \
        bsd-getpeereid.c \
        bsd-misc.c \
        bsd-poll.c \
        bsd-setres_id.c \
        fmt_scaled.c \
        getrrsetbyname.c \
        glob.c \
        memmem.c \
        mktemp.c \
        openssl-compat.c \
        port-net.c \
        readpassphrase.c \
        xcrypt.c

SRCS+= ${LIBOPENSSH_SRCS} ${XMSS_SRCS} ${LIBSSH_SRCS} ${SK_SRCS} \
       ${OPENBSD_COMPAT_SRCS}

COPTS.addr.c+= -Wno-sign-compare
COPTS.bcrypt_pbkdf.c+= -Wno-pointer-sign
COPTS.channels.c+= -Wno-pointer-sign -fno-strict-aliasing
COPTS.fmt_scaled.c+= -Wno-format-truncation
COPTS.kex.c+= -Wno-pointer-sign
COPTS.log.c+= -Wno-format-truncation
COPTS.misc.c+= -Wno-format-truncation
COPTS.packet.c+= -Wno-pointer-sign
COPTS.ssh-pkcs11.c+= -Wno-pointer-sign
COPTS.sshkey.c+= -Wno-pointer-sign

CPPFLAGS+= -I${SSHDIST}
CPPFLAGS+= -I${OPENBSD_COMPAT}
.PATH:		${SSHDIST}
.PATH:      ${OPENBSD_COMPAT}

LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/crypto/${EXTERNAL_LIBRESSL_SUBDIR}/lib/libcrypto \
			crypt	${NETBSDSRCDIR}/usr.lib/libcrypt \
            util    ${NETBSDSRCDIR}/usr.lib/libutil \
			z		${NETBSDSRCDIR}/usr.lib/libz

.include <bsd.lib.mk>
