VERSION = 0.1.0

PREFIX  ?= /usr/local
DATADIR ?= $(PREFIX)/share
MANDIR  ?= $(DATADIR)/man
DISTDIR ?= tsup-$(VERSION)

INCS = -I./ $(shell pkg-config --cflags libevent)
LIBS = $(shell pkg-config --libs libevent)

CC       ?= gcc

CPPFLAGS += -D_DEFAULT_SOURCE -DVERSION=\"$(VERSION)\"
CPPFLAGS += ${INCS}

CFLAGS   ?= -Os
CFLAGS   += -std=c99 -Wpedantic -Wall

LDFLAGS  += ${LIBS}

