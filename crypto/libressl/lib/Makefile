#	$211BSD: Makefile,v 1.0 2025/03/31 19:15:27 Exp $

.include "bsd.own.mk"

# LibreSSL libraries.
SUBDIR= libcrypto .WAIT
SUBDIR+= libssl .WAIT	# depends on libcrypto
SUBDIR+= libtls	        # depends on libcrypto

.include <bsd.subdir.mk>
