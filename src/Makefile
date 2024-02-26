#

SRCS=	hq.c print.c parse.y modify.c utils.c selector.y

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
BINDIR?=		/usr/local/bin

hq.md: hq.1
	mandoc -T markdown hq.1 > hq.md

.include <bsd.prog.mk>
.include <bsd.own.mk>
