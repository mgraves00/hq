#

SRCS=	hq.c print.c parse.y modify.c utils.c selector.y

PROG=	hq
CFLAGS+=	-Wall -I${.CURDIR}
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=	-Wmissing-declarations
CFLAGS+=	-Wshadow -Wpointer-arith -Wcast-qual
CFLAGS+=	-Wsign-compare
YFLAGS=		-v -t
DEBUG=		-g
CLEANFILES+=	y.output hq.md

#LDADD+=
#DPADD+=

hq.md: hq.1
	mandoc -T markdown hq.1 > hq.md

.include <bsd.prog.mk>
