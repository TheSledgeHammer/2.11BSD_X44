/*	$OpenBSD: chacha_private.h,v 1.4 2020/07/22 13:54:30 tobhe Exp $	*/
/*
chacha-merged.c version 20080118
D. J. Bernstein
Public domain.
*/

typedef uint8_t  u8;
typedef uint32_t u32;

typedef struct {
	u32 input[16]; /* could be compressed */
} chacha_ctx;

#define ROTL32(v,n) \
	((v) << (n)) | ((v) >> (32 - (n)))

#define U8TO32_LITTLE(v,p) 	(v = le32dec(p))
#define U32TO8_LITTLE(p,v)	(le32enc(p, v))

#define ROTATE(v,c) (ROTL32(v,c))
#define XOR(v,w) 	((v)^(w))
#define PLUS(v,w) 	((v) + (w))
#define PLUSONE(v) 	(PLUS((v),1))

#define XOR_U8TO32_LITTLE(v, p) (v = XOR(v, le32dec(p)))

#define QUARTERROUND(a, b, c, d) 			\
	a = PLUS(a,b); d = ROTATE(XOR(d,a),16); \
	c = PLUS(c,d); b = ROTATE(XOR(b,c),12); \
	a = PLUS(a,b); d = ROTATE(XOR(d,a), 8); \
	c = PLUS(c,d); b = ROTATE(XOR(b,c), 7);

static const char sigma[16] = "expand 32-byte k";
static const char tau[16] = "expand 16-byte k";

void 	hchacha20(u32 derived_key[8], const u8 nonce[16], const u8 key[32]);
void	chacha_keysetup(chacha_ctx *x, const u8 *k, u32 kbits);
void	chacha_ivsetup(chacha_ctx *x, const u8 *iv, const u8 *counter);
void	chacha_encrypt_bytes(chacha_ctx *x,const u8 *m,u8 *c, u32 bytes);
