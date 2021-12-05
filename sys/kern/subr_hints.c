/*
 * Copyright (c) 1997,1998 Doug Rabson
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
 *
 * $FreeBSD: src/sys/kern/subr_bus.c,v 1.54.2.9 2002/10/10 15:13:32 jhb Exp $
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/kenv.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/bus.h>

/*
 * Access functions for device resources.
 */

/* Supplied by config(8) in ioconf.c */
extern struct cfhint allhints[];

/* Runtime version */
struct cfhint *hint = allhints;

void
resource_init()
{
	resource_cfgload();
}

static int
resource_new_name(const char *name, int unit)
{
	struct cfhint *new;

	new = &allhints;
	new = malloc((cfhint_count + 1) * sizeof(*new), M_TEMP, M_WAITOK | M_ZERO);
	if(hint && cfhint_count > 0) {
		bcopy(hint, new, cfhint_count * sizeof(*new));
	}
	new[cfhint_count].ch_name = malloc(strlen(name) + 1, M_TEMP, M_WAITOK);
	if(new[cfhint_count].ch_name == NULL) {
		free(hint, M_TEMP);
		return (-1);
	}
	strcpy(new[cfhint_count].ch_name, name);
	new[cfhint_count].ch_unit = unit;
	new[cfhint_count].ch_rescount = 0;
	new[cfhint_count].ch_resources = NULL;
	if (hint && hint != allhints) {
		free(new, M_TEMP);
	}
	hint = new;
	return (cfhint_count++);
}

static int
resource_new_resname(int j, const char *resname, resource_type type)
{
	struct cfresource *new;
	int i;

	i = hint[j].ch_rescount;
	new = malloc((i + 1) * sizeof(*new), M_TEMP, M_WAITOK | M_ZERO);
	if (hint[j].ch_resources && i > 0) {
		bcopy(hint[j].ch_resources, new, i * sizeof(*new));
	}
	new[i].cr_name = malloc(strlen(resname) + 1, M_TEMP, M_WAITOK);
	if (new[i].cr_name == NULL) {
		free(new, M_TEMP);
		return (-1);
	}
	strcpy(new[i].cr_name, resname);
	new[i].cr_type = type;
	if (hint[j].ch_resources) {
		free(hint[j].ch_resources, M_TEMP);
	}
	hint[j].ch_resources = new;
	hint[j].ch_rescount = i + 1;

	return (i);
}

static int
resource_match_string(int i, const char *resname, const char *value)
{
	int j;
	struct cfresource *res;
	for (j = 0, res = hint[i].ch_resources; j < hint[i].ch_rescount; j++, res++) {
		if (!strcmp(res->cr_name, resname) && res->cr_type == RES_STRING && !strcmp(res->cr_u.stringval, value)) {
			return (j);
		}
	}
	return (-1);
}

static int
resource_find(const char *name, int unit, const char *resname, struct cfresource **result)
{
	int i, j;
	struct cfresource *res;

	/*
	 * First check specific instances, then generic.
	 */
	for (i = 0; i < cfhint_count; i++) {
		if (allhints[i].ch_unit < 0)
			continue;
		if (!strcmp(hint[i].ch_name, name) && hint[i].ch_unit == unit) {
			res = hint[i].ch_resources;
			for (j = 0; j < hint[i].ch_rescount; j++, res++) {
				if (!strcmp(res->cr_name, resname)) {
					*result = res;
					return(0);
				}
			}
		}
	}

	for (i = 0; i < cfhint_count; i++) {
		if (hint[i].ch_unit >= 0)
			continue;
		/* XXX should this `&& devtab[i].unit == unit' be here? */
		/* XXX if so, then the generic match does nothing */
		if (!strcmp(hint[i].ch_name, name) && hint[i].ch_unit == unit) {
			res = hint[i].ch_resources;
			for (j = 0; j < hint[i].ch_rescount; j++, res++) {
				if (!strcmp(res->cr_name, resname)) {
					*result = res;
					return(0);
				}
			}
		}
	}
	return (ENOENT);
}

/*
 * DragonFly style loader.conf hinting
 */
static int
resource_kenv(const char *name, int unit, const char *resname, long *result)
{
	const char *env;
	char buf[64];

	snprintf(buf, sizeof(buf), "%s%d.%s", name, unit, resname);
	if ((env = kern_getenv(buf)) != NULL) {
		*result = strtol(env, NULL, 0);
		return(0);
	}

	/*
	 * Also support FreeBSD style loader.conf hinting
	 */
	snprintf(buf, sizeof(buf), "hint.%s.%d.%s", name, unit, resname);
	if ((env = kern_getenv(buf)) != NULL) {
		*result = strtol(env, NULL, 0);
		return (0);
	}

	return (ENOENT);
}

int
resource_int_value(const char *name, int unit, const char *resname, int *result)
{
	struct cfresource *res;
	long value = 0;
	int error;

	if(resource_kenv(name, unit, resname, &value) == 0) {
		*result = (int)value;
		return (0);
	}
	if ((error = resource_find(name, unit, resname, &res)) != 0) {
		return (error);
	}
	if (res->cr_type != RES_INT) {
		return (EFTYPE);
	}
	*result = res->cr_u.intval;
	return (0);
}

int
resource_long_value(const char *name, int unit, const char *resname, long *result)
{
	struct cfresource *res;
	long value = 0;
	int error;

	if(resource_kenv(name, unit, resname, &value) == 0) {
		*result = (int)value;
		return (0);
	}
	if ((error = resource_find(name, unit, resname, &res)) != 0) {
		return (error);
	}
	if (res->cr_type != RES_LONG) {
		return (EFTYPE);
	}
	*result = res->cr_u.longval;
	return (0);
}

int
resource_string_value(const char *name, int unit, const char *resname, const char **result)
{
	struct cfresource *res;
	int error;
	char buf[64];
	const char *env;

	/*
	 * DragonFly style loader.conf hinting
	 */
	snprintf(buf, sizeof(buf), "%s%d.%s", name, unit, resname);
	if ((env = kern_getenv(buf)) != NULL) {
		*result = env;
		return (0);
	}

	/*
	 * Also support FreeBSD style loader.conf hinting
	 */
	snprintf(buf, sizeof(buf), "hint.%s.%d.%s", name, unit, resname);
	if ((env = kern_getenv(buf)) != NULL) {
		*result = env;
		return (0);
	}

	if ((error = resource_find(name, unit, resname, &res)) != 0) {
		return (error);
	}
	if (res->cr_type != RES_STRING) {
		return (EFTYPE);
	}
	*result = res->cr_u.stringval;
	return (0);
}

int
resource_query_string(int i, const char *resname, const char *value)
{
	if (i < 0) {
		i = 0;
	} else {
		i = i + 1;
	}
	for (; i < cfhint_count; i++) {
		if (resource_match_string(i, resname, value) >= 0) {
			return (i);
		}
	}
	return (-1);
}

int
resource_locate(int i, const char *resname)
{
	if (i < 0) {
		i = 0;
	} else {
		i = i + 1;
	}
	for (; i < cfhint_count; i++) {
		if (!strcmp(hint[i].ch_name, resname)) {
			return (i);
		}
	}
	return (-1);
}

int
resource_count(void)
{
	return (cfhint_count);
}

char *
resource_query_name(int i)
{
	return (hint[i].ch_name);
}

int
resource_query_unit(int i)
{
	return (hint[i].ch_unit);
}

static int
resource_create(const char *name, int unit, const char *resname, resource_type type, struct cfresource **result)
{
	int i, j;
	struct cfresource *res = NULL;

	for (i = 0; i < cfhint_count; i++) {
		if (!strcmp(hint[i].ch_name, name) && hint[i].ch_unit == unit) {
			res = hint[i].ch_resources;
			break;
		}
	}
	if (res == NULL) {
		i = resource_new_name(name, unit);
		if (i < 0) {
			return (ENOMEM);
		}
		res = hint[i].ch_resources;
	}
	for (j = 0; j < hint[i].ch_rescount; j++, res++) {
		if (!strcmp(res->cr_name, resname)) {
			*result = res;
			return (0);
		}
	}
	j = resource_new_resname(i, resname, type);
	if (j < 0) {
		return (ENOMEM);
	}
	res = &hint[i].ch_resources[j];
	*result = res;
	return (0);
}

int
resource_set_int(const char *name, int unit, const char *resname, int value)
{
	int error;
	struct cfresource *res;

	error = resource_create(name, unit, resname, RES_INT, &res);
	if (error)
		return (error);
	if (res->cr_type != RES_INT)
		return (EFTYPE);
	res->cr_u.intval = value;
	return (0);
}

int
resource_set_long(const char *name, int unit, const char *resname, long value)
{
	int error;
	struct cfresource *res;

	error = resource_create(name, unit, resname, RES_LONG, &res);
	if (error)
		return (error);
	if (res->cr_type != RES_LONG)
		return (EFTYPE);
	res->cr_u.longval = value;
	return (0);
}

int
resource_set_string(const char *name, int unit, const char *resname, const char *value)
{
	int error;
	struct cfresource *res;

	error = resource_create(name, unit, resname, RES_STRING, &res);
	if (error)
		return (error);
	if (res->cr_type != RES_STRING)
		return (EFTYPE);
	if (res->cr_u.stringval)
		free(res->cr_u.stringval, M_TEMP);
	res->cr_u.stringval = malloc(strlen(value) + 1, M_TEMP, M_WAITOK);
	if (res->cr_u.stringval == NULL)
		return (ENOMEM);
	strcpy(res->cr_u.stringval, value);
	return (0);
}

static void
resource_cfgload(void *dummy /*__unused*/)
{
	struct cfresource *res, *cfgres;
	int i, j;
	int error;
	char *name, *resname;
	int unit;
	resource_type type;
	char *stringval;
	int config_cfhint_count;

	config_cfhint_count = cfhint_count;
	hint = NULL;
	cfhint_count = 0;

	for (i = 0; i < config_cfhint_count; i++) {
		name = allhints[i].ch_name;
		unit = allhints[i].ch_unit;

		for (j = 0; j < allhints[i].ch_rescount; j++) {
			cfgres = allhints[i].ch_resources;
			resname = cfgres[j].cr_name;
			type = cfgres[j].cr_type;
			error = resource_create(name, unit, resname, type, &res);
			if (error) {
				printf("create resource %s%d: error %d\n", name, unit, error);
				continue;
			}
			if (res->cr_type != type) {
				printf("type mismatch %s%d: %d != %d\n", name, unit, res->cr_type, type);
				continue;
			}
			switch (type) {
			case RES_INT:
				res->cr_u.intval = cfgres[j].cr_u.intval;
				break;
			case RES_LONG:
				res->cr_u.longval = cfgres[j].cr_u.longval;
				break;
			case RES_STRING:
				if (res->cr_u.stringval) {
					free(res->cr_u.stringval, M_TEMP);
				}
				stringval = cfgres[j].cr_u.stringval;
				res->cr_u.stringval = malloc(strlen(stringval) + 1, M_TEMP, M_WAITOK);
				if (res->cr_u.stringval == NULL) {
					break;
				}
				strcpy(res->cr_u.stringval, stringval);
				break;
			default:
				panic("unknown resource type %d", type);
			}
		}
	}
}

/*
 * Check to see if a device is enabled via a disabled hint.
 */
int
resource_enabled(const char *name, int unit)
{
	return (!resource_disabled(name, unit));
}

/*
 * Check to see if a device is disabled via a disabled hint.
 */
int
resource_disabled(const char *name, int unit)
{
	int error, value;

	error = resource_int_value(name, unit, "disabled", &value);
	if (error) {
		return (0);
	}
	return (value);
}
