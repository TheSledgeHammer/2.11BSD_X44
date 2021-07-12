
# this works in gnu make
SYSNAME:=${shell uname}

# this works in bsd make
SYSNAME!=uname

# the gnu rules will mess up bsd but not vice versa,
# hence the gnu rules come first.

RANLIB=true

include Make.$(SYSNAME)

PREFIX=/usr/local

NUKEFILES=

TGZFILES=

CLEANFILES=

TARG=mk
VERSION=2.0
PORTPLACE=devel/mk
NAME=mk
CLEANFILES=mk mk.1a

OFILES=\
	arc.$O\
	archive.$O\
	bufblock.$O\
	env.$O\
	file.$O\
	graph.$O\
	job.$O\
	lex.$O\
	main.$O\
	match.$O\
	mk.$O\
	parse.$O\
	recipe.$O\
	rc.$O\
	rule.$O\
	run.$O\
	sh.$O\
	shell.$O\
	shprint.$O\
	symtab.$O\
	var.$O\
	varsub.$O\
	word.$O\
	unix.$O\

HFILES=\
	mk.h\
	fns.h\

all: $(TARG)

TGZFILES+=mk.pdf

install: $(TARG)
	mkdir -p $(PREFIX)/bin
	install -m 0755 mk $(PREFIX)/bin/mk
	cat mk.1 | sed 's;DOCPREFIX;$(PREFIX);g' >mk.1a
	mkdir -p $(PREFIX)/man/man1
	install -m 0644 mk.1a $(PREFIX)/man/man1/mk.1
	mkdir -p $(PREFIX)/doc
	install -m 0644 mk.pdf $(PREFIX)/doc/mk.pdf

$(LIB): $(OFILES)
	$(AR) $(ARFLAGS) $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

NUKEFILES+=$(LIB)
.c.$O:
	$(CC) $(CFLAGS) -I../libutf -I../libfmt -I../libbio -I../libregexp -I$(PREFIX)/include $*.c

%.$O: %.c
	$(CC) $(CFLAGS) -I../libutf -I../libfmt -I../libbio -I../libregexp -I$(PREFIX)/include $*.c


$(OFILES): $(HFILES)

tgz:
	rm -rf $(NAME)-$(VERSION)
	mkdir $(NAME)-$(VERSION)
	cp Makefile Make.* README LICENSE NOTICE *.[ch137] rpm.spec bundle.ports $(TGZFILES) $(NAME)-$(VERSION)
	tar cf - $(NAME)-$(VERSION) | gzip >$(NAME)-$(VERSION).tgz
	rm -rf $(NAME)-$(VERSION)

clean:
	rm -f $(OFILES) $(LIB) $(CLEANFILES)

nuke:
	rm -f $(OFILES) *.tgz *.rpm $(NUKEFILES)

rpm:
	make tgz
	cp $(NAME)-$(VERSION).tgz /usr/src/RPM/SOURCES
	rpm -ba rpm.spec
	cp /usr/src/RPM/SRPMS/$(NAME)-$(VERSION)-1.src.rpm .
	cp /usr/src/RPM/RPMS/i586/$(NAME)-$(VERSION)-1.i586.rpm .
	scp *.rpm rsc@amsterdam.lcs.mit.edu:public_html/software

PORTDIR=/usr/ports/$(PORTPLACE)

ports:
	make tgz
	rm -rf $(PORTDIR)
	mkdir $(PORTDIR)
	cp $(NAME)-$(VERSION).tgz /usr/ports/distfiles
	cat bundle.ports | (cd $(PORTDIR) && awk '$$1=="---" && $$3=="---" { ofile=$$2; next} {if(ofile) print >ofile}')
	(cd $(PORTDIR); make makesum)
	(cd $(PORTDIR); make)
	(cd $(PORTDIR); /usr/local/bin/portlint)
	rm -rf $(PORTDIR)/work
	shar `find $(PORTDIR)` > ports.shar
	(cd $(PORTDIR); tar cf - *) | gzip >$(NAME)-$(VERSION)-ports.tgz
	scp *.tgz rsc@amsterdam.lcs.mit.edu:public_html/software

.phony: all clean nuke install tgz rpm ports

$(TARG): $(OFILES)
	$(CC) -o $(TARG) $(OFILES) -L$(PREFIX)/lib -L../libutf -L../libfmt -L../libbio -L../libregexp -lregexp9 -lbio -lfmt -lutf

