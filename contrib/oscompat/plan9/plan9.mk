#	$211BSD: Makefile,v 1.0 2025/05/19 19:27:27 Exp $
#
# Note:
# Makefile automatically includes dependencies, whether 
# those dependencies have been disabled or enabled
#

MKLIBFMT?= no
MKLIBUTF?= no
MKLIBBIO?= no
MKLIBREGEXP?= no
MKMK?= no # See Notes/Info below for issues

# 
# Notes/Info:
# 
# libutf: 
#	- Plan 9 UTF7 library
# libfmt:
# 	- Plan 9 format library
# libbio:
# 	- Plan 9 buffered io library
# libregexp:
# 	- Plan 9 regular expression library
# mk:
# 	- Plan 9 make program
#	- Issues: Unresovled compile issues, 
# 	which prevent it compiling.
#
