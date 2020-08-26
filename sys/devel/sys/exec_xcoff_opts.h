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

/* NOTE: The Following Contents of exec_xcoff_opts.h is derived from IBM Knowledge Center:
 * https://www.ibm.com/support/knowledgecenter/ssw_aix_72/filesreference/XCOFF.html
 */

#ifndef _SYS_EXEC_XCOFF_OPTS_H_
#define _SYS_EXEC_XCOFF_OPTS_H_

/* XCOFF loader header */
struct xcoff32_ldrhdr {
	int32_t 	l_version;		/* loader section version number */
	int32_t		l_nsyms;		/* # of symbol table entries */
	int32_t		l_nreloc;		/* # of relocation table entries */
	uint32_t	l_istlen;		/* length of import file ID string table */
	int32_t		l_nimpid;		/* # of import file IDs */
	uint32_t	l_impoff;		/* offset to start of import file IDs */
	uint32_t	l_stlen;		/* length of string table */
	uint32_t 	l_stoff;		/* offset to start of string table */
	uint32_t	l_symoff;		/* offset to start of symbol table */
	uint16_t 	l_rldoff;		/* offset to start of relocation entries */
};

struct xcoff64_ldrhdr {
	int32_t 	l_version;		/* loader section version number */
	int32_t		l_nsyms;		/* # of symbol table entries */
	int32_t		l_nreloc;		/* # of relocation table entries */
	uint32_t	l_istlen;		/* length of import file ID string table */
	int32_t		l_nimpid;		/* # of import file IDs */
	uint32_t	l_impoff;		/* offset to start of import file IDs */
	uint32_t	l_stlen;		/* length of string table */
	uint64_t 	l_stoff;		/* offset to start of string table */
	uint64_t	l_symoff;		/* offset to start of symbol table */
	uint16_t 	l_rldoff;		/* offset to start of relocation entries */
};

/* XCOFF loader symbol */
struct xcoff32_ldrsyms {
	char		l_name[8];		/* symbol name or byte offset into string table */
	int32_t		l_zeroes;		/* zero indicates symbol name is referenced from l_offset */
	int32_t		l_offset;		/* byte offset into string table of symbol name */
	int32_t 	l_value;		/* address field */
	int16_t 	l_scnum;		/* section number containing symbol */
	int8_t		l_smtype;		/* symbol type, export, import flags */
	int8_t 		l_smclas;		/* symbol storage class */
	int32_t 	l_ifile;		/* import file ID; ordinal of import file IDs */
	int32_t 	l_parm;			/* parameter type-check field */
};

struct xcoff64_ldrsyms {
	char		l_name[8];		/* symbol name or byte offset into string table */
	int32_t		l_zeroes;		/* zero indicates symbol name is referenced from l_offset */
	int32_t		l_offset;		/* byte offset into string table of symbol name */
	int64_t 	l_value;		/* address field */
	int16_t 	l_scnum;		/* section number containing symbol */
	int8_t		l_smtype;		/* symbol type, export, import flags */
	int8_t 		l_smclas;		/* symbol storage class */
	int32_t 	l_ifile;		/* import file ID; ordinal of import file IDs */
	int32_t 	l_parm;			/* parameter type-check field */
};

#endif /* _SYS_EXEC_XCOFF_OPTS_H_ */
