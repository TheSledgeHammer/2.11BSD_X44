/*
 * common.h
 *
 *  Created on: 13 Aug 2020
 *      Author: marti
 */

#define	ELF_NOTE_NETBSD_NAMESZ		ELF_NOTE_NETBSD_NAMESZ
#define	ELF_NOTE_NETBSD_DESCSZ		ELF_NOTE_NETBSD_DESCSZ
#define	ELF_NOTE_TYPE_NETBSD_TAG	ELF_NOTE_TYPE_NETBSD_TAG
#define	ELF_NOTE_NETBSD_NAME		ELF_NOTE_NETBSD_NAME
#define	__211BSD_Version__			__211BSD_Version__

static void __ctors_begin(void);
static void __dtors_begin(void);
static void __ctors_end(void);
static void __dtors_end(void);

extern void _Jv_RegisterClasses(void *) __attribute__((weak));
static void register_classes(void);
