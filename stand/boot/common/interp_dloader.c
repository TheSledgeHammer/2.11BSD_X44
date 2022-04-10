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

#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include "bootstrap.h"
#include "commands.h"

struct bootblk_command commands[] = {
		COMMON_COMMANDS,
		{	"local", "List local variables", command_local },
		{	"lunset", "Unset local variable", command_lunset },
		{	"lunsetif", "Unset local variable if kenv variable is true", command_lunsetif },
		{	"loadall", "Load kernel + modules", command_loadall },
		{	"menuclear", "Clear all menus", command_menuclear },
		{	"menuitem", "Add menu bullet", command_menuitem },
		{	"menuadd", "Add script line for bullet", command_menuadd },
		{	"menu", "Run menu system", command_menu },
		{ NULL,	NULL, NULL },
};

void
interp_init(void)
{
	setenv("script.lang", "dloader", 1);
//    setenv("base", DirBase, 1);

	/* Read our default configuration. */
	 if(interp_include("/boot/loader.rc") != CMD_OK) {
		 interp_include("/boot/loader.conf");
	 }
}
