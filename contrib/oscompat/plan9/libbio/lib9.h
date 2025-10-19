#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE

#include <utf.h>
#include <fmt.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <u.h>
#include <libc.h>

#define seek(fd, offset, whence) lseek(fd, offset, whence)
#define create(name, mode, perm) creat(name, perm)
