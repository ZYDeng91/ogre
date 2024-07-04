PROG =		ogre
SRC =		cfgparse.c main.c

PREFIX ?=	/usr#/local
BINDIR ?=	${PREFIX}/bin

CC ?=		gcc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-Wall -ffast-math -funsigned-char #-Wno-pointer-sign

PTHREADFLAGS ?=	-pthread
INCLUDEFLAGS ?=	-I${PREFIX}/include -I${PREFIX}/include/libxml2
LINKFLAGS ?=	-L${PREFIX}/lib
LINKFLAGS +=    -lshout -lxml2

all: ${PROG}

${PROG}: ${SRC}
	${CC} ${CFLAGS} ${PTHREADFLAGS} ${INCLUDEFLAGS} -o ${PROG} ${SRC} ${LINKFLAGS}

clean:
	-@rm -f ${PROG} *~ core *.core
