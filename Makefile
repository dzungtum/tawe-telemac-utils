CFLAGS=--std=gnu99 --pedantic -Wall -fstack-protector-all -Wstack-protector -Wmissing-prototypes -Wno-unused-result -D_GNU_SOURCE
LDLIBS=-lm
SHELL=/bin/bash
EXES=telemac-parse telemac-info telemac-vtu

.PHONY: clean check all release debug doc

all: release

debug: CFLAGS+=-g -O0
debug: clean ${EXES}

release: CFLAGS+=-D_FORTIFY_SOURCE=2 -O2
release: ${EXES}

${EXES}: telemac-loader.o

telemac-vtu: CFLAGS+=`xml2-config --cflags`
telemac-vtu: LDLIBS+=`xml2-config --libs`

%.o: %.c %.h

telemac-info: telemac-loader.o

clean:
	-@rm -f ${EXES:=${EXEEXT}} *.o

check:
	scan-build -enable-checker security.insecureAPI.strcpy -enable-checker alpha.core.CastSize \
		-enable-checker alpha.security.ArrayBoundV2 -enable-checker alpha.security.taint.TaintPropagation \
		-enable-checker alpha.unix.cstring.BufferOverlap -enable-checker alpha.unix.cstring.NotNullTerminated \
		make -s -B all

doc:
	doxygen doc/Doxyfile
