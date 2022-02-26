/*	$NetBSD: fileload.c,v 1.5 2014/03/25 18:35:32 christos Exp $	*/

/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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

/*
 * file/module function dispatcher, support, etc.
 */
#include <sys/user.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>

#include "bootstrap.h"
/*
static int	file_load(char *filename, vaddr_t dest, struct preloaded_file **result);
static int	file_havepath(const char *name);
static void file_insert_tail(struct preloaded_file *mp);
static void	file_remove(struct preloaded_file *fp);
static char *file_lookup(const char *path, const char *name, int namelen, char **extlist);
*/

/* load address should be tweaked by first module loaded (kernel) */
static vaddr_t	loadaddr = 0;

struct preloaded_file *preloaded_files = NULL;

/*
 * load a kernel from disk.
 *
 * kernels are loaded as:
 *
 * load <path> <options>
 */

int
command_load(int argc, char *argv[])
{
	char *typestr;
	int dofile, dokld, ch;

	dokld = dofile = 0;
	optind = 1;
	optreset = 1;
	typestr = NULL;
	if (argc == 1) {
		command_seterr("no filename specified");
		return (CMD_ERROR);
	}
	while ((ch = getopt(argc, argv, "k:")) != -1) {
		switch (ch) {
		case 'k':
			dokld = 1;
			break;
		case '?':
		default:
			/* getopt has already reported an error */
			return (CMD_OK);
		}
	}
	argv += (optind - 1);
	argc -= (optind - 1);

	/*
	 * Do we have explicit KLD load ?
	 */
	if (dokld || file_havepath(argv[1])) {
		int error = file_loadkernel(argv[1], argc - 2, argv + 2);
		if (error == EEXIST)
			command_seterr("warning: KLD '%s' already loaded", argv[1]);
		return error == 0 ? CMD_OK : CMD_ERROR;
	}
	return CMD_OK;
}


int
command_unload(int argc, char *argv[])
{
    struct preloaded_file	*fp;

    while (preloaded_files != NULL) {
    	fp = preloaded_files;
    	preloaded_files = preloaded_files->f_next;
    	file_discard(fp);
    }
    loadaddr = 0;
    unsetenv("kernelname");
    return(CMD_OK);
}


int
command_lskern(int argc, char *argv[])
{
    struct preloaded_file	*fp;
    char					lbuf[80];
    int						ch, verbose;

    verbose = 0;
    optind = 1;
    optreset = 1;

    pager_open();
    for (fp = preloaded_files; fp; fp = fp->f_next) {
    	snprintf(lbuf, sizeof(lbuf), " %p: %s (%s, 0x%lx)\n",
    			(void *) fp->f_addr, fp->f_name, fp->f_type, (long) fp->f_size);
    	pager_output(lbuf);
    	if (fp->f_args != NULL) {
    		pager_output("    args: ");
    		pager_output(fp->f_args);
    		pager_output("\n");
    	}
    }
    pager_close();
    return(CMD_OK);
}

/*
 * File level interface, functions file_*
 */
int
file_load(char *filename, vaddr_t dest, struct preloaded_file **result)
{
    struct preloaded_file *fp;
    int error;
    int i;

    error = EFTYPE;
    for (i = 0, fp = NULL; file_formats[i] && fp == NULL; i++) {
    	error = (file_formats[i]->l_load)(filename, dest, &fp);
    	if (error == 0) {
    		fp->f_loader = i;		/* remember the loader */
    		*result = fp;
    		break;
    	}
    	if (error == EFTYPE)
    		continue;		/* Unknown to this handler? */
    	if (error) {
    		command_seterr("can't load file '%s': %s",
    				filename, strerror(error));
    		break;
    	}
    }
    return (error);
}

/*
 * Load specified KLD. If path is omitted, then try to locate it via
 * search path.
 */
int
file_loadkernel(char *filename, int argc, char *argv[])
{
    struct preloaded_file	*fp, *last_file;
    int						err;

    /*
     * Check if KLD already loaded
     */
    fp = file_findfile(filename, NULL);
    if (fp) {
    	command_seterr("warning: KLD '%s' already loaded", filename);
    	free(filename);
    	return (0);
    }
    for (last_file = preloaded_files;
    		last_file != NULL && last_file->f_next != NULL;
    		last_file = last_file->f_next)
    	;
    do {
    	err = file_load(filename, loadaddr, &fp);
    	if (err)
    		break;
    	fp->f_args = unargv(argc, argv);
    	loadaddr = fp->f_addr + fp->f_size;
    	file_insert_tail(fp);		/* Add to the list of loaded files */
    } while(0);
    if (err == EFTYPE)
    	command_seterr("don't know how to load module '%s'", filename);
    if (err && fp)
    	file_discard(fp);
    	free(filename);
    return (err);
}

#define VECTX_HANDLE(fd) fd

/*
 * We've been asked to load (fname) as (type), so just suck it in,
 * no arguments or anything.
 */
struct preloaded_file *
file_loadraw(const char *fname, char *type, int insert)
{
	struct preloaded_file	*fp;
	char					*name;
	int						fd, got;
	vm_offset_t				laddr;

	/* We can't load first */
	if ((file_findfile(NULL, NULL)) == NULL) {
		command_errmsg = "can't load file before kernel";
		return (NULL);
	}

	/* locate the file on the load path */
	name = file_search(fname, NULL);
	if (name == NULL) {
		snprintf(command_errbuf, sizeof(command_errbuf), "can't find '%s'",
				fname);
		return (NULL);
	}

	if ((fd = open(name, O_RDONLY)) < 0) {
		snprintf(command_errbuf, sizeof(command_errbuf), "can't open '%s': %s",
				name, strerror(errno));
		free(name);
		return (NULL);
	}
	if (archsw.arch_loadaddr != NULL)
		loadaddr = archsw.arch_loadaddr(LOAD_RAW, name, loadaddr);

	printf("%s ", name);

	laddr = loadaddr;
	for (;;) {
		/* read in 4k chunks; size is not really important */
		got = archsw.arch_readin(VECTX_HANDLE(fd), laddr, 4096);
		if (got == 0) /* end of file */
			break;
		if (got < 0) { /* error */
			snprintf(command_errbuf, sizeof(command_errbuf),
					"error reading '%s': %s", name, strerror(errno));
			free(name);
			close(fd);
			return (NULL);
		}
		laddr += got;
	}

	printf("size=%#jx\n", (uintmax_t) (laddr - loadaddr));

	/* Looks OK so far; create & populate control structure */
	fp = file_alloc();
	if (fp == NULL) {
		snprintf(command_errbuf, sizeof(command_errbuf), "no memory to load %s",
				name);
		free(name);
		close(fd);
		return (NULL);
	}
	fp->f_name = name;
	fp->f_type = strdup(type);
	fp->f_args = NULL;
	fp->f_metadata = NULL;
	fp->f_loader = -1;
	fp->f_addr = loadaddr;
	fp->f_size = laddr - loadaddr;

	if (fp->f_type == NULL) {
		snprintf(command_errbuf, sizeof(command_errbuf), "no memory to load %s",
				name);
		free(name);
		close(fd);
		return (NULL);
	}
	/* recognise space consumption */
	loadaddr = laddr;

	/* Add to the list of loaded files */
	if (insert != 0)
		file_insert_tail(fp);
	close(fd);
	return (fp);
}

/*
 * Find a file matching (name) and (type).
 * NULL may be passed as a wildcard to either.
 */
struct preloaded_file *
file_findfile(char *name, char *type)
{
    struct preloaded_file *fp;

    for (fp = preloaded_files; fp != NULL; fp = fp->f_next) {
    	if (((name == NULL) || !strcmp(name, fp->f_name)) &&
    			((type == NULL) || !strcmp(type, fp->f_type)))
    		break;
    }
    return (fp);
}

/*
 * Check if file name have any qualifiers
 */
static int
file_havepath(const char *name)
{
    const char		*cp;

    archsw.arch_getdev(NULL, name, &cp);
    return (cp != name || strchr(name, '/') != NULL);
}

/*
 * Throw a file away
 */
void
file_discard(struct preloaded_file *fp)
{
    if (fp == NULL)
    	return;
    if (fp->f_name != NULL)
    	free(fp->f_name);
    if (fp->f_type != NULL)
        free(fp->f_type);
    if (fp->f_args != NULL)
        free(fp->f_args);
    if (fp->f_marks != NULL)
    	free(fp->f_marks);
    free(fp);
}

/*
 * Allocate a new file; must be used instead of malloc()
 * to ensure safe initialisation.
 */
struct preloaded_file *
file_alloc(void)
{
    struct preloaded_file	*fp;

    if ((fp = alloc(sizeof(struct preloaded_file))) != NULL) {
    	memset(fp, 0, sizeof(struct preloaded_file));
    }
    return (fp);
}

/*
 * Add a module to the chain
 */
static void
file_insert_tail(struct preloaded_file *fp)
{
    struct preloaded_file	*cm;

    /* Append to list of loaded file */
    fp->f_next = NULL;
    if (preloaded_files == NULL) {
    	preloaded_files = fp;
    } else {
    	for (cm = preloaded_files; cm->f_next != NULL; cm = cm->f_next)
    		;
    	cm->f_next = fp;
    }
}

/*
 * Make a copy of (size) bytes of data from (p), and associate them as
 * metadata of (type) to the module (mp).
 */
void
file_addmetadata(struct preloaded_file *fp, int type, size_t size, void *p)
{
	struct file_metadata	*md;

	md = malloc(sizeof(struct file_metadata) - sizeof(md->md_data) + size);
	if (md != NULL) {
		md->md_size = size;
		md->md_type = type;
		bcopy(p, md->md_data, size);
		md->md_next = fp->f_metadata;
	}
	fp->f_metadata = md;
}

/*
 * Find a metadata object of (type) associated with the file (fp)
 */
struct file_metadata*
file_findmetadata(struct preloaded_file *fp, int type) {
	struct file_metadata *md;

	for (md = fp->f_metadata; md != NULL; md = md->md_next) {
		if (md->md_type == type) {
			break;
		}
	}
	return (md);
}

/*
 * Remove all metadata from the file.
 */
void file_removemetadata(struct preloaded_file *fp) {
	struct file_metadata *md, *next;

	for (md = fp->f_metadata; md != NULL; md = next) {
		next = md->md_next;
		free(md);
	}
	fp->f_metadata = NULL;
}

struct file_metadata*
metadata_next(struct file_metadata *md, int type) {

	if (md == NULL) {
		return (NULL);
	}
	while ((md = md->md_next) != NULL) {
		if (md->md_type == type) {
			break;
		}
	}
	return (md);
}
