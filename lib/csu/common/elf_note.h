#ifndef _ELF_NOTE_H_
#define _ELF_NOTE_H_

#include <sys/param.h>

#define	CSU_ELF_NOTE_211BSD_NAMESZ		  7
#define	CSU_ELF_NOTE_211BSD_DESCSZ		  4
#define	CSU_ELF_NOTE_TYPE_211BSD_TAG	  1
#define	CSU_ELF_NOTE_211BSD_NAME		    "NetBSD\0\0"
#define	CSU_211BSD_Version__		        __211BSD_Version__

#define	CSU_ELF_NOTE_PAX_NAMESZ		      4
#define	CSU_ELF_NOTE_PAX_DESCSZ		      4
#define	CSU_ELF_NOTE_TYPE_PAX_TAG	      3
#define	CSU_ELF_NOTE_PAX_NAME		        "PaX\0"

#endif
