/*
 * exec_boot.c
 *
 *  Created on: 10 Apr 2020
 *      Author: marti
 */

struct bootinfo bootinfo;

boot(howto, dev, off)
{

	struct bootinfo *btinfo;
	struct bootargs *btargs;

	bi_setboothowto(howto);

	btargs->bootdev;
	btargs->howto;
	btargs->bootinfo;

	run_loadfile(, howto);
}


copyunix(io, howto)
{

}

void
enter_kernel(filename, start, bi)
	const char* filename;
	u_int64_t start;
	struct bootinfo *bi;
{
	printf("Entering %s at 0x%lx...\n", filename, start);
}

static int
run_loadfile(fp)
	struct preloaded_file *fp;
{
	struct bootinfo		*bi;

	if (strcmp(fp->f_type, AOUT_KERNELTYPE)) {
		return(EFTYPE);
	}
	if (strcmp(fp->f_type, ELF32_KERNELTYPE)) {
		return(EFTYPE);
	}

	bi = &bootinfo;
	memset(bi, 0, sizeof(struct bootinfo));
	bi_load(bi, fp, "");

	enter_kernel(fp->f_name, fp->marks[MARK_ENTRY], bi);
}


