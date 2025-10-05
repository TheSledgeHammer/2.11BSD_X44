/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
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

#ifndef _NETTPI_TPI_VAR_H_
#define _NETTPI_TPI_VAR_H_

#define TPDU_CMD 			0
#define TPDU_CLASS0			1	/* TP_CLASS_0 */
#define TPDU_CLASS1			2	/* TP_CLASS_1 */
#define TPDU_CLASS2			3	/* TP_CLASS_2 */
#define TPDU_CLASS3			4	/* TP_CLASS_3 */
#define TPDU_CLASS4			5	/* TP_CLASS_4 */
#define TPDU_MAXCMDCLASS		6
#define TPDU_CMDCLASS(val) 		((val) * TPDU_MAXCMDCLASS)

/* TPDU types */
#define TPDUT_CC			TPDU_CMDCLASS(0)	/* CC_TPDU */
#define TPDUT_CR			TPDU_CMDCLASS(1)	/* CR_TPDU */
#define TPDUT_ER			TPDU_CMDCLASS(2)	/* ER_TPDU */
#define TPDUT_DR			TPDU_CMDCLASS(3)	/* DR_TPDU */
#define TPDUT_DC			TPDU_CMDCLASS(4)	/* DC_TPDU */
#define TPDUT_AK			TPDU_CMDCLASS(5)	/* AK_TPDU */
#define TPDUT_DT			TPDU_CMDCLASS(6)	/* DT_TPDU */
#define TPDUT_XPD			TPDU_CMDCLASS(7)	/* XPD_TPDU */
#define TPDUT_XAK			TPDU_CMDCLASS(8)	/* XAK_TPDU */

/* TPDU timer events */
#define TPDU_INACT			TPDU_CMDCLASS(9)	/* TM_inact */
#define TPDU_RETRANS			TPDU_CMDCLASS(10)	/* TM_retrans */
#define TPDU_SENDACK			TPDU_CMDCLASS(11)	/* TM_sendack */
#define TPDU_NOTUSED			TPDU_CMDCLASS(12)	/* TM_notused */
#define TPDU_REFERENCE			TPDU_CMDCLASS(13)	/* TM_reference */
#define TPDU_DATA_RETRANS		TPDU_CMDCLASS(14)	/* TM_data_retrans */

/* TS-user requests */
#define TSU_CONNECT_REQUEST		TPDU_CMDCLASS(15)	/* T_CONN_req */
#define TSU_CONNECT_RESPONSE		TPDU_CMDCLASS(16)
#define TSU_DISCONNECT_REQUEST		TPDU_CMDCLASS(17)	/* T_DISC_req */
#define TSU_DATA_REQUEST		TPDU_CMDCLASS(18)	/* T_DATA_req */
#define TSU_XPD_DATA_REQUEST		TPDU_CMDCLASS(19)	/* T_XPD_req */
#define TSU_NETRESET			TPDU_CMDCLASS(20)	/* T_NETRESET */

/* state handlers */
#define TPDU_CONNECT_INDICATION		1
#define TPDU_CONNECT_CONFIRM		2
#define TPDU_DISCONNECT_INDICATION	3
#define TPDU_DATA_INDICATION		4
#define TPDU_XPD_DATA_INDICATION	5

#endif /* _NETTPI_TPI_VAR_H_ */
