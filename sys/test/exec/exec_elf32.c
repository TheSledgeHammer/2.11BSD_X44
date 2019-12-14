/*
 * exec_elf32.c
 *
 *  Created on: 14 Dec 2019
 *      Author: osiris
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/resourcevar.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_elf.h>
#include <sys/mman.h>
#include <sys/kernel.h>
#include <sys/sysent.h>

#include <vm/vm.h>
