/*
 * mastersw.c
 *
 *  Created on: 15 Dec 2020
 *      Author: marti
 */


#include <sys/conf.h>
#include <sys/devsw.h>
#include <master.h>
#include <sys/user.h>
#include <sys/map.h>

struct devswio_config {
	const char 				*do_name;
	int						do_config;
};

struct devswio_config ksyms_conf = {
		.do_name = "ksyms",
		.do_config = ksyms_configure
};

int
ksyms_configure(conf)
	struct devswio_config *conf;
{
	conf->do_config = devswtable_configure(NKSYMS, NULL, &ksyms_cdevsw, NULL);

	return (conf->do_config);
}

struct devswio_driver {

};
