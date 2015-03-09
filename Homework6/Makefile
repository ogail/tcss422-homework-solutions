# Makefile for the encrypted ramdisk (encrd) driver.
PROG=	crd
SRCS=	crd.c

DPADD+=	${LIBCHARDRIVER} ${LIBSYS}
LDADD+=	-lblockdriver -lsys -lz

MAN=

BINDIR?= /usr/sbin

.include <minix.service.mk>
