/*
 * ufs211_vnops.c
 *
 *  Created on: 18 Apr 2020
 *      Author: marti
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/dirent.h>

#include <vfs/specfs/specdev.h>

#include <ufs211/ufs211_quota.h>
#include <ufs211/ufs211_inode.h>
#include <ufs211/ufs211_dir.h>

#include <vm/include/vm.h>
