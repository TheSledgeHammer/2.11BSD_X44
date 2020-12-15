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

typedef int *attach_t(struct devswtable *devsw, dev_t dev, void *data);
typedef int *detach_t(dev_t dev, void *data);
/* slave/client control */
struct slavesw {
	char					*s_name;
	int						(*s_attach)(struct devswtable *devsw, dev_t dev, void *data);
	int						(*s_detach)(dev_t dev, void *data);
	int						s_size;
};

/* master/server control */
struct mastersw {
	struct slavesw 			*m_slavesw;
	struct devswtable 		*m_devswtable;

	dev_t					m_major;
	int 					m_refcnt;

	struct bdevsw			*m_bdevsw;
	struct cdevsw			*m_cdevsw;
	struct linesw			*m_linesw;
};

dev_type_attach(slave_attach);
dev_type_detach(slave_detach);

dev_t
m_major(msw)
	struct mastersw *msw;
{
	return (msw->m_major);
}

struct slavesw *
m_slavsw(msw)
	struct mastersw *msw;
{
	return (msw->m_slavesw);
}

struct devswtable *
m_devswtable(msw)
	struct mastersw *msw;
{
	return (msw->m_devswtable);
}

struct bdevsw *
m_bdevsw(msw)
	struct mastersw *msw;
{
	return (msw->m_bdevsw);
}

struct cdevsw *
m_cdevsw(msw)
	struct mastersw *msw;
{
	return (msw->m_cdevsw);
}

struct linesw *
m_linesw(msw)
	struct mastersw *msw;
{
	return (msw->m_linesw);
}

int
devswtable_add_bdevsw(devsw, bdev, major)
	struct devswtable  *devsw;
	struct bdevsw 		*bdev;
	dev_t 				major;
{
	devswtable_add(devsw, bdev, major);

	return (bdevsw_attach(bdev, major));
}

int
devswtable_add_cdevsw(devsw, cdev, major)
	struct devswtable  *devsw;
	struct cdevsw 		*cdev;
	dev_t 				major;
{
	devswtable_add(devsw, cdev, major);

	return (cdevsw_attach(cdev, major));
}

int
devswtable_add_linesw(devsw, line, major)
	struct devswtable  *devsw;
	struct linesw 		*line;
	dev_t 				major;
{
	devswtable_add(devsw, line, major);

	return (linesw_attach(line, major));
}

int
devswtable_remove_bdevsw(devsw, bdev, major)
	struct devswtable  *devsw;
	struct bdevsw 		*bdev;
	dev_t 				major;
{
	devswtable_remove(devsw, bdev, major);

	return (bdevsw_detach(bdev));
}

int
devswtable_remove_cdevsw(devsw, cdev, major)
	struct devswtable  *devsw;
	struct cdevsw 		*cdev;
	dev_t 				major;
{
	devswtable_remove(cdev, major);

	return (cdevsw_detach(cdev));
}

int
devswtable_remove_linesw(devsw, line, major)
	struct devswtable  *devsw;
	struct linesw 		*line;
	dev_t 				major;
{
	devswtable_remove(line, major);

	return (linesw_detach(line));
}

int
add(major, type)
	dev_t		major;
	int 		type;
{
	struct mastersw *msw;
	int error = 0;

	msw->m_major = major;

	switch(type) {
	    case BDEVTYPE:
	    	if(bdevsw(msw)) {
	    		error = devswtable_add_bdevsw(devswtable(msw), bdevsw(msw), major(msw));
	    		if(error != 0) {
	    			return (error);
	    		}
	    	}
	    	break;
	    case CDEVTYPE:
	    	if(cdevsw(msw)) {
	    		error = devswtable_add_cdevsw(devswtable(msw), cdevsw(msw), major(msw));
	    		if(error != 0) {
	    			return (error);
	    		}
	    	}
	    	break;
	    case LINETYPE:
	    	if(linesw(msw)) {
	    		error = devswtable_add_cdevsw(devswtable(msw), linesw(msw), major(msw));
	    		if(error != 0) {
	    			return (error);
	    		}
	    	}
	    	break;
	}
	return (error);
}

struct slavesw 	*
slave_create(type)
	int 			type;
{
	int error;
	struct mastersw *msw;
	struct slavesw 	*ssw = msw->m_slavesw;

	if(ssw == NULL) {
		return (NULL);
	}

	if(devswtable(msw) == NULL && add(major(msw), type) != 0) {
		return (NULL);
	}

	error = add(major(msw), type);
	msw->m_refcnt++;
	printf("added to devswtable successfully");

	return (ssw);
}

int
slave_attach(devsw, dev, data)
	struct devswtable 	*devsw;
	dev_t 				dev;
	void 				*data;
{
	struct slavesw *s;
	int rv, error;

	if(s == slavesw(&sys_msw)) {

	}

	rv = s->s_attach(devsw, dev, data);

	return (rv);
}

int
slave_detach(dev, data)
	dev_t 		dev;
	void 		*data;
{
	struct slavesw *s;
	int rv;

	rv = s->s_detach(dev, data);

	return (rv);
}
