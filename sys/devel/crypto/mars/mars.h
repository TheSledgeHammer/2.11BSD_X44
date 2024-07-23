#ifndef _MARS_H
#define _MARS_H

typedef struct {
	u_int32_t l_key[40];
} mars_ctx;

void mars_set_key(mars_ctx *ctx, const u_int8_t in_key[], const u_int8_t key_len);
void mars_encrypt(mars_ctx *ctx, const u_int8_t in_blk[], u_int8_t out_blk[]);
void mars_decrypt(mars_ctx *ctx, const u_int8_t in_blk[], u_int8_t out_blk[]);

#endif /* _MARS_H */
