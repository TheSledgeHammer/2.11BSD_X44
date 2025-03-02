/*
 * Copyright (c) 2001 Alexey Zelkin <phantom@FreeBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__FBSDID("$FreeBSD$");
#endif

#include <stddef.h>

#include "lmessages.h"
#include "ldpart.h"

#define LCMESSAGES_SIZE_FULL (sizeof(messages_locale_t) / sizeof(char *))
#define LCMESSAGES_SIZE_MIN  (offsetof(messages_locale_t, yesstr) / sizeof(char *))

static char empty[] = "";

static const messages_locale_t _C_messages_locale = {
		"^[yY]" ,	/* yesexpr */
		"^[nN]" ,	/* noexpr */
		"yes" , 	/* yesstr */
		"no"		/* nostr */
};

static messages_locale_t _messages_locale;
static int	_messages_using_locale;
static char	*_messages_locale_buf;

int
__messages_load_locale(const char *name)
{
	int ret;

	ret = __part_load_locale(name, &_messages_using_locale,
			&_messages_locale_buf, "LC_MESSAGES",
			LCMESSAGES_SIZE_FULL, LCMESSAGES_SIZE_MIN,
			(const char **) &_messages_locale);
	if (ret == _LDP_LOADED) {
		if (_messages_locale.yesstr == NULL)
			_messages_locale.yesstr = empty;
		if (_messages_locale.nostr == NULL)
			_messages_locale.nostr = empty;
	}
	return (ret);
}

messages_locale_t *
__get_current_messages_locale(void)
{
	return (__UNCONST(_messages_using_locale ?
			&_messages_locale : &_C_messages_locale));
}

#ifdef LOCALE_DEBUG
void
msgdebug() {
printf(	"yesexpr = %s\n"
	"noexpr = %s\n"
	"yesstr = %s\n"
	"nostr = %s\n",
	_messages_locale.yesexpr,
	_messages_locale.noexpr,
	_messages_locale.yesstr,
	_messages_locale.nostr
);
}
#endif /* LOCALE_DEBUG */
