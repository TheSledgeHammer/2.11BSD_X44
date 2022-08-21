#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# locale sources
.PATH: ${ARCHDIR}/locale ${.CURDIR}/locale

SRCS+=	ansi.c citrus_ctype.c ctype.c euc.c frune.c isctype.c lconv.c 	\
		localeconv.c mbrune.c __mb_cur_max.c none.c rune.c setlocale.c 	\
		srune.c table.c ues.c utf1632.c utf2.c utf8.c wcscoll.c wcstod.c \
		wcstol.c wcstoll.c wcstoimax.c wcstoul.c wcstoull.c wcstoumax.c \
		wcsxfrm.c aliasname.c
SRCS+=	multibyte.c _wctrans.c iswctype.c
SRCS+=	multibyte_sb.c iswctype_sb.c
SRC+=	setlocale32.c
SRCS+=	collate.c ldpart.c lmessages.c lmonetary.c lnumeric.c ltime.c

MAN+=	ctype.3 isalnum.3 isalpha.3 isascii.3 isblank.3 iscntrl.3 		\
		isdigit.3 isgraph.3 islower.3 isprint.3 ispunct.3 isspace.3 	\
		isupper.3 isxdigit.3 mbrune.3 multibyte.3 rune.3 setlocale.3 	\
		toascii.3 tolower.3 toupper.3
MAN+=	btowc.3 iswalnum.3 iswctype.3 towlower.3 towctrans.3 mblen.3 	\
		mbrlen.3 mbrtowc.3 mbsinit.3 mbsrtowcs.3 mbstowcs.3 mbtowc.3 	\
		wcrtomb.3 wcsrtombs.3 wcstombs.3 wctob.3 wctomb.3 wctrans.3		\
		wctype.3
MAN+=	euc.4 utf2.4
MAN+=	utf8.5

MLINKS+=iswalnum.3 iswalpha.3 iswalnum.3 iswblank.3 \
		iswalnum.3 iswcntrl.3 iswalnum.3 iswdigit.3 \
		iswalnum.3 iswgraph.3 iswalnum.3 iswlower.3 \
		iswalnum.3 iswprint.3 iswalnum.3 iswpunct.3 \
		iswalnum.3 iswspace.3 iswalnum.3 iswupper.3 \
		iswalnum.3 iswxdigit.3
MLINKS+=towlower.3 towupper.3
