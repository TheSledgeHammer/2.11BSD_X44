#include <fmt.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

#include <lib9.h>

#define	exits(x) exit(x && *x ? 1 : 0)
