#
include Makefile.configure

SRCS=	hq.c print.c parse.y modify.c utils.c selector.y compats.c
OBJS=	hq.o print.o parse.o modify.o utils.o selector.o compats.o

PROG=		hq
MAN=		hq.1
CFLAGS+=	-Wall -I${.CURDIR}
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=	-Wmissing-declarations
CFLAGS+=	-Wshadow -Wpointer-arith -Wcast-qual
CFLAGS+=	-Wsign-compare
YFLAGS=		-v -t
CLEANFILES+=	y.output hq.md
DEBUG=		-g

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDADD)

install:
	$(INSTALL_PROGRAM) $(PROG) $(DESTDIR)$(BINDIR)
	$(INSTALL_MAN) $(MAN) $(DESTDIR)$(MANDIR)

regress:
	# do nothing

distcheck:
	# do nothing

#$(OBJS): hq.h config.h

clean:
	cd ${.CURDIR}/obj && rm -f $(OBJS) $(CLEANFILES) *.core parse.c selector.c $(PROG)

distclean:
	echo ${.CURDIR}
	rm -f config.log config.h Makefile.configure

hq.md: hq.1
	mandoc -T markdown ${.CURDIR}/hq.1 > ${.CURDIR}/../hq.md

