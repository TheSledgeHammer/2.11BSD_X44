/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "../../ufs/ufml/ufml.h"
#include "../../ufs/ufml/ufml_extern.h"
#include "../../ufs/ufml/ufml_meta.h"
#include "../../ufs/ufml/ufml_ops.h"

int
ufml_encrypt(ap)
	struct uop_encrypt_args *ap;
{
	struct ufml_node *ip = ap->a_up;
	struct ufml_metadata *meta = ip->ufml_meta;
	int fs, type;

	if (ufml_check_filesystem(meta, meta->ufml_filesystem) > 0) {
		fs = meta->ufml_filesystem;
		if (ufml_check_encryption(meta, meta->ufml_encrypt) > 0) {
			type = meta->ufml_encrypt;
			return (UOP_ENCRYPT(ip, ap->a_vp, ap->a_mp, fs, type));
		}
		return (UOP_ENCRYPT(ip, ap->a_vp, ap->a_mp, fs, 0));
	}

	return (ENIVAL);
}

int
ufml_decrypt(ap)
	struct uop_decrypt_args *ap;
{
	struct ufml_node *ip = ap->a_up;
	struct ufml_metadata *meta = ip->ufml_meta;
	int fs, type;

	if (ufml_check_filesystem(meta, meta->ufml_filesystem) > 0) {
		fs = meta->ufml_filesystem;
		if (ufml_check_encryption(meta, meta->ufml_encrypt) > 0) {
			type = meta->ufml_encrypt;
			return (UOP_DECRYPT(ip, ap->a_vp, ap->a_mp, fs, type));
		}
		return (UOP_DECRYPT(ip, ap->a_vp, ap->a_mp, fs, 0));
	}

	return (ENIVAL);
}
