# Derived from Inferno libmach/mkfile
# http://code.google.com/p/inferno-os/source/browse/utils/libmach/mkfile
#
#	Copyright © 1994-1999 Lucent Technologies Inc.
#	Power PC support Copyright © 1995-2004 C H Forsyth (forsyth@terzarima.net).
#	Portions Copyright © 1997-1999 Vita Nuova Limited.
#	Portions Copyright © 2000-2007 Vita Nuova Holdings Limited (www.vitanuova.com).
#	Revisions Copyright © 2000-2004 Lucent Technologies Inc. and others.
#	Portions Copyright © 2009 The Go Authors.  All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

include ../Make.inc
include ../unix/Make.$(SYSNAME)

PREFIX=/usr/local

NUKEFILES=

TGZFILES=

CLEANFILES=

LIB=libmach.a
VERSION=2.0
PORTPLACE=devel/libmach
NAME=libmach

OFILES=\
	executable.$O\
	fakeobj.$O\
	map.$O\
	obj.$O\
	swap.$O\
	sym.$O\
	access.$O\
	machdata.$O\
	setmach.$O\
	5.$O\
	6.$O\
	8.$O\
	5db.$O\
	8db.$O\
	5obj.$O\
	6obj.$O\
	8obj.$O\

ifneq ($(GOHOSTOS),windows)
OFILES+=\
	$(shell uname | tr A-Z a-z).$O\

else
OFILES+=\
	windows.$O\

endif

HFILES=\
    mach.h\
    elf.h\
    macho.h\
    obj.h\

all: $(LIB)

install: $(LIB)
	mkdir -p $(PREFIX)/include
	install -m 0644 mach.h $(PREFIX)/include/mach.h
	mkdir -p $(PREFIX)/lib
	install -m 0644 $(LIB) $(PREFIX)/lib/$(LIB)

include ../Make.clib
include ../unix/Make.package
