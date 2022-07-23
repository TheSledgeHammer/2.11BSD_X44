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

#include "dloader.h"

char *
logo_get_blank_lines(lineLen)
    int lineLen;
{
    int i;
    char *blank = logo_blank_line;

    if (lineLen == 0) {
        lineLen = DEFAULT_LINES;
    } else if (lineLen >= MAX_BLANK_LINES) {
        lineLen = MAX_BLANK_LINES;
    } else {
        for(i = 0; i < lineLen; i++) {
            blank[i] = logo_blank_line[i];
        }
    }
    return (blank);
}

void
logo_display(logo, line, lineNum, orientation, barrier)
    char **logo;
    int line, lineNum, orientation, barrier;
{
    const char *fmt;
    char *blank_line = logo_get_blank_lines(lineNum);

    if (orientation == 0) {
        fmt = barrier ? "%s  | " : "  %s  ";
    } else {
        fmt = barrier ? " |  %s" : "  %s  ";
    }

    if (logo != NULL) {
        if (line < lineNum) {
            printf(fmt, logo[line]);
        } else {
            printf(fmt, blank_line);
        }
    }
}
