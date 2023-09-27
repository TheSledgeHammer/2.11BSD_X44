# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# this works in gnu make
SYSNAME:=${shell uname}

# this works in bsd make
SYSNAME!=uname

# the gnu rules will mess up bsd but not vice versa,
# hence the gnu rules come first.

RANLIB=true

# Makefile included by all other Go makefiles.

# Clear variables that must come from Makefiles,
# not the environment.
LIB:=
TARG:=
GOFILES:=
HFILES:=
OFILES:=
YFILES:=

GOROOT:= $(CURDIR)

# GOROOT must be set.
ifeq ($(GOROOT),)
$(error $$GOROOT is not set; use gomake or set $$GOROOT in your environment)
endif

# Set up GOROOT_FINAL, GOARCH, GOOS if needed.
GOROOT_FINAL?=$(GOROOT)

ifeq ($(GOHOSTOS),)
GOHOSTOS:=$(shell uname | tr A-Z a-z | sed 's/mingw/windows/; s/.*windows.*/windows/')
endif

ifeq ($(GOOS),)
GOOS:=$(GOHOSTOS)
endif

GOOS_LIST=\
	darwin\
	freebsd\
	linux\
	plan9\
	windows\

GOARCH_LIST=\
	386\
	amd64\
	arm\

ifeq ($(filter $(GOOS),$(GOOS_LIST)),)
$(error Invalid $$GOOS '$(GOOS)'; must be one of: $(GOOS_LIST))
endif

ifeq ($(GOHOSTARCH),)
ifeq ($(GOHOSTOS),darwin)
# Even on 64-bit platform, darwin uname -m prints i386.
# Check for amd64 with sysctl instead.
GOHOSTARCH:=${shell if sysctl machdep.cpu.extfeatures | grep EM64T >/dev/null; then echo amd64; else uname -m | sed 's/i386/386/'; fi}
else
# Ask uname -m for the processor.
GOHOSTARCH:=${shell uname -m | sed 's/^..86$$/386/; s/^.86$$/386/; s/x86_64/amd64/; s/arm.*/arm/'}
endif
endif

ifeq ($(GOARCH),)
GOARCH:=$(GOHOSTARCH)
endif

# darwin requires GOHOSTARCH match GOARCH
ifeq ($(GOOS),darwin)
GOHOSTARCH:=$(GOARCH)
endif

ifeq ($(filter $(GOARCH),$(GOARCH_LIST)),)
$(error Invalid $$GOARCH '$(GOARCH)'; must be one of: $(GOARCH_LIST))
endif


ifeq ($(GOARCH),386)
O:=8
else ifeq ($(GOARCH),amd64)
O:=6
else ifeq ($(GOARCH),arm)
O:=5
ifneq ($(GOOS),linux)
$(error Invalid $$GOOS '$(GOOS)' for GOARCH=arm; must be linux)
endif
else
$(error Missing $$O for '$(GOARCH)')
endif

# Save for recursive make to avoid recomputing.
export GOARCH GOOS GOHOSTARCH GOHOSTOS GOARCH_LIST GOOS_LIST

AS=${O}a
CC=${O}c
GC=${O}g
LD=${O}l
OS=568vq
CFLAGS=-FVw

# Make environment more standard.
LANG:=
LC_ALL:=C
LC_CTYPE:=C
GREP_OPTIONS:=
GREP_COLORS:=
export LANG LC_ALL LC_CTYPE GREP_OPTIONS GREP_COLORS
