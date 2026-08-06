/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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

#ifndef _NETTPI_ISO_NSAP_H_
#define _NETTPI_ISO_NSAP_H_

/* NSAP: Network Service Access Point */
/* NSAP addr (ISO/OSI equivalent) */
struct nsap_iso {
	struct sap_tree 	*nsi_tree;
	struct sap_node 	*nsi_node;
#define nsi_snsap 		nsi_node->st_sasap 	/* sockaddr_sap */
#define nsi_nsapa 		nsi_node->st_sapa   /* sap_addr */
};

/* TSAP: Transport Service Access Point */
/* TSAP addr (ISO/OSI equivalent) */
struct tsap_iso {
	struct nsap_iso 	tsi_nsaps[SAPLEN];
	struct sap_select 	tsi_select;
#define tsi_selector	tsi_select.ss_selector
#define tsi_sid			tsi_select.ss_sid
#define tsi_af			tsi_select.ss_af
};

/* SSAP: Session Service Access Point */
/* SSAP addr (ISO/OSI equivalent) */
struct ssap_iso {
	struct tsap_iso 	ssi_tsaps[SAPLEN];
	struct sap_select 	ssi_select;
#define ssi_selector	ssi_select.ss_selector
#define ssi_sid			ssi_select.ss_sid
#define ssi_af			ssi_select.ss_af
};

/* PSAP: Presentation Service Access Point */
/* PSAP addr (ISO/OSI equivalent) */
struct psap_iso {
	struct ssap_iso 	psi_ssaps[SAPLEN];
	struct sap_select 	psi_select;
#define psi_selector	psi_select.ss_selector
#define psi_sid			psi_select.ss_sid
#define psi_af			psi_select.ss_af
};

/* NSAP's */
void nsap_attach(struct nsap_iso *, int, int);
void nsap_detach(struct nsap_iso *, int, int);
void nsap_iso_attach(struct nsap_iso *);
void nsap_iso_detach(struct nsap_iso *);
int nsap_iso_compare(struct nsap_iso *, struct nsap_iso *);

/* TSAP's */
void tsap_attach(struct tsap_iso *, struct nsap_iso *, int, int);
void tsap_detach(struct tsap_iso *, struct nsap_iso *, int, int);
struct nsap_iso *tsap_to_nsap(struct tsap_iso *);
void tsap_iso_attach(struct tsap_iso *, struct nsap_iso *);
void tsap_iso_detach(struct tsap_iso *, struct nsap_iso *);
int tsap_iso_compare(struct tsap_iso *, struct tsap_iso *);

/* SSAP's */
void ssap_attach(struct ssap_iso *, struct tsap_iso *, int, int);
void ssap_detach(struct ssap_iso *, struct tsap_iso *, int, int);
struct tsap_iso *ssap_to_tsap(struct ssap_iso *);
void ssap_iso_attach(struct ssap_iso *, struct tsap_iso *);
void ssap_iso_detach(struct ssap_iso *, struct tsap_iso *);
int ssap_iso_compare(struct ssap_iso *, struct ssap_iso *);

/* PSAP's */
void psap_attach(struct psap_iso *, struct ssap_iso *, int, int);
void psap_detach(struct psap_iso *, struct ssap_iso *, int, int);
struct ssap_iso *psap_to_ssap(struct psap_iso *);
void psap_iso_attach(struct psap_iso *, struct ssap_iso *);
void psap_iso_detach(struct psap_iso *, struct ssap_iso *);
int psap_iso_compare(struct psap_iso *, struct psap_iso *);

#endif /* _NETTPI_ISO_NSAP_H_ */
