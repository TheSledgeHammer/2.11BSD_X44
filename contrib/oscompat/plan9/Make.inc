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

ROOT:= $(CURDIR)
GOROOT:= $(ROOT)
