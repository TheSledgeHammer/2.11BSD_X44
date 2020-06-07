/*
 ---------------------------------------------------------------------------
 Copyright (c) 1999, Dr Brian Gladman, Worcester, UK.   All rights reserved.
 LICENSE TERMS
 The free distribution and use of this software is allowed (with or without
 changes) provided that:
  1. source code distributions include the above copyright notice, this
     list of conditions and the following disclaimer;
  2. binary distributions include the above copyright notice, this list
     of conditions and the following disclaimer in their documentation;
  3. the name of the copyright holder is not used to endorse products
     built using this software without specific written permission.
 DISCLAIMER
 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 My thanks to Doug Whiting and Niels Ferguson for comments that led
 to improvements in this implementation.
 Issue Date: 14th January 1999
*/

#ifndef __TWOFISH_H
#define __TWOFISH_H

typedef struct {
        u_int32_t l_key[40];
        u_int32_t s_key[4];
        u_int32_t mk_tab[4*256];
        u_int32_t k_len;
} twofish_ctx;

void twofish_set_key(twofish_ctx *ctx, const u_int8_t in_key[], int key_len);
void twofish_encrypt(twofish_ctx *ctx, const u_int8_t in_blk[], u_int8_t out_blk[]);
void twofish_decrypt(twofish_ctx *ctx, const u_int8_t in_blk[], u_int8_t out_blk[]);

#endif /* __TWOFISH_H */
