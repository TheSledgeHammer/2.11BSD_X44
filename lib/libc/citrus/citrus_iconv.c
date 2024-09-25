/*
 * citrus_iconv.c
 *
 *  Created on: 25 Sept 2024
 *      Author: marti
 */

struct _citrus_iconv_encoding {
	struct frune_encoding 	*se_handle;
	void 					*se_ps;
	void 					*se_pssaved;
};

/*
 * convenience routines for stdenc.
 */
static __inline void
save_encoding_state(struct _citrus_iconv_std_encoding *se)
{
	if (se->se_ps) {
		memcpy(se->se_pssaved, se->se_ps, _citrus_stdenc_get_state_size(se->se_handle));
	}
}
