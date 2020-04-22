/*
 * Copyright (c) 2010 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <boot/bootstand.h>
#include "dloader.h"

#define BEASTIE_LINES 	20

static char *beastie_color[BEASTIE_LINES] =  {

};

static char *beastie_indigo[BEASTIE_LINES] =  {

};

static char *beastie_mono[BEASTIE_LINES] =  {

};

void
display_beastie(logo_left, separated)
	int logo_left, separated;
{
	dvar_t dvar;
	int i;
	char **logo = beastie_indigo;
	char *console_val = getenv("console");

	if (dvar_istrue(dvar_get("logo_is_red")))
		logo = beastie_color;

	if (dvar_istrue(dvar_get("loader_plain")))
		logo = beastie_mono;

	if (strcmp(console_val, "comconsole") == 0)
		logo = beastie_mono;

	if (dvar_istrue(dvar_get("logo_disable")))
		logo = NULL;

	if (dvar_istrue(dvar_get("logo_on_left")))
		logo_left = 1;

	if (dvar_istrue(dvar_get("logo_separated")))
		separated = 1;

	dvar = dvar_first();
	i = 0;

	if (logo != NULL) {
		if (logo_left)
			printf(separated ? "%35s|%43s\n" : "%35s %43s\n", " ", " ");
		else
			printf(separated ? "%43s|%35s\n" : "%43s %35s\n", " ", " ");
	}

	while (dvar || i < BEASTIE_LINES) {
		if (logo_left)
			logo_display(logo, i, BEASTIE_LINES, LOGO_LEFT, separated);

		while (dvar) {
			if (strncmp(dvar->name, "menu_", 5) == 0) {
				printf(" %c. %-38.38s", dvar->name[5], dvar->data[0]);
				dvar = dvar_next(dvar);
				break;
			}
			dvar = dvar_next(dvar);
		}
		/*
		 * Pad when the number of menu entries is less than
		 * LOGO_LINES.
		 */
		if (dvar == NULL)
			printf("    %38.38s", " ");

		if (!logo_left)
			logo_display(logo, i, BEASTIE_LINES, LOGO_RIGHT, separated);
		printf("\n");
		i++;
	}
}
