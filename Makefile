include config.mk

HEADERS    = config.h $(wildcard src/*.h)
SRC_COMMON = $(wildcard src/*.c)
OBJ_COMMON = ${SRC_COMMON:.c=.o}

BIN_DAEMON = tsupd
SRC_DAEMON = tsupd.c
OBJ_DAEMON = tsupd.o

BIN_CLIENT = tsupq
SRC_CLIENT = tsupq.c
OBJ_CLIENT = tsupq.o

BINS = $(BIN_DAEMON) $(BIN_CLIENT)

.PHONY : all options dist clean install uninstall install-man
all: options $(BINS)

options:
	@echo tsup build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CPPFLAGS = ${CPPFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"

config.h: config.def.h
	@echo Creating $@ from $<
	@cp $< $@

%.o: %.c $(HEADERS)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(BINS): % : %.o $(OBJ_COMMON)
	$(CC) -o $@ $^ $(LDFLAGS)

install-man:
	@echo Installing man pages to ${DESTDIR}$(MANDIR)/
	mkdir -p ${DESTDIR}$(MANDIR)/man1
	sed "s/VERSION/$(VERSION)/g" man/$(BIN_DAEMON).1 \
		> ${DESTDIR}$(MANDIR)/man1/$(BIN_DAEMON).1
	sed "s/VERSION/$(VERSION)/g" man/$(BIN_CLIENT).1 \
		> ${DESTDIR}$(MANDIR)/man1/$(BIN_CLIENT).1
	chmod 0644 ${DESTDIR}$(MANDIR)/man1/$(BIN_DAEMON).1
	chmod 0644 ${DESTDIR}$(MANDIR)/man1/$(BIN_CLIENT).1

install: all install-man
	@echo Installing executables to ${DESTDIR}$(PREFIX)/bin
	mkdir -p ${DESTDIR}$(PREFIX)/bin
	cp $(BINS) ${DESTDIR}$(PREFIX)/bin
	chmod 0755 ${DESTDIR}$(PREFIX)/bin/$(BIN_DAEMON)
	chmod 0755 ${DESTDIR}$(PREFIX)/bin/$(BIN_CLIENT)

uninstall:
	rm -f ${DESTDIR}$(PREFIX)/bin/$(BIN_DAEMON) \
		${DESTDIR}$(PREFIX)/bin/$(BIN_CLIENT)
	rm -f ${DESTDIR}$(MANDIR)/man1/$(BIN_DAEMON).1 \
		${DESTDIR}$(MANDIR)/man1/$(BIN_CLIENT).1

dist:
	@echo Creating dist release $(DISTDIR)
	mkdir -p $(DISTDIR)
	cp config.mk Makefile config.def.h LICENSE README.rst $(DISTDIR)/
	cp $(SRC_DAEMON) $(SRC_CLIENT) $(DISTDIR)/
	mkdir -p $(DISTDIR)/src
	cp src/*.c src/*.h $(DISTDIR)/src/
	mkdir -p $(DISTDIR)/man
	cp -r man/. $(DISTDIR)/man/
	tar -c -f $(DISTDIR).tar --remove-files $(DISTDIR)/
	gzip $(DISTDIR).tar

clean:
	@echo Cleaning
	rm -f $(BINS)
	rm -f $(OBJ_DAEMON) $(OBJ_CLIENT) $(OBJ_COMMON)
	rm -f config.h
	rm -f $(DISTDIR).tar.gz

