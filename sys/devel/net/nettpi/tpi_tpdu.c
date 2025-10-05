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

#include "tpi_tpdu.h"
#include "tpdu_var.h"

#define TPDU_COMMAND(tpdu_kind, cmd) ((tpdu_kind) + (cmd))

int
tpdu_statehandler(struct tpdu *tpdu, int tpdu_kind, int cmd)
{
    register int action = 0;

	switch (action) {
	case TPDU_CONNECT_INDICATION:
	case TPDU_CONNECT_CONFIRM:
	case TPDU_DISCONNECT_INDICATION:
	case TPDU_DATA_INDICATION:
	case TPDU_XPD_DATA_INDICATION:
	}
    return (action);
}


tpdu_state_error()
{

}

tpdu_state_open()
{

}

tpdu_state_close()
{

}


tpdu_state_confirm()
{

}


int
tpdu_class(int tpdu_kind, int cmd)
{
	int command = TPDU_COMMAND(tpdu_kind, cmd);
	switch (command) {
	case TPDU_CLASS0 + TPDU_CMD:
	case TPDU_CLASS1 + TPDU_CMD:
	case TPDU_CLASS2 + TPDU_CMD:
	case TPDU_CLASS3 + TPDU_CMD:
	case TPDU_CLASS4 + TPDU_CMD:
	}
	return (0);
}
