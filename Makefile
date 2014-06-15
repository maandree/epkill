PREFIX = /usr
BIN = /bin
BINDIR = $(PREFIX)$(BIN)
DATA = /share
DATADIR = $(PREFIX)$(DATA)
INFODIR = $(DATADIR)/info
LOCALEDIR = $(DATADIR)/locale
LICENSEDIR = $(DATADIR)/licenses

PKGNAME = epkill
VERSION = 1.0


WARN = -Wall -Wextra -pedantic -Wdouble-promotion -Wformat=2 -Winit-self       \
       -Wmissing-include-dirs -Wtrampolines -Wfloat-equal -Wshadow             \
       -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls           \
       -Wnested-externs -Winline -Wno-variadic-macros -Wsign-conversion        \
       -Wswitch-default -Wconversion -Wsync-nand -Wunsafe-loop-optimizations   \
       -Wcast-align -Wstrict-overflow -Wdeclaration-after-statement -Wundef    \
       -Wbad-function-cast -Wcast-qual -Wlogical-op                            \
       -Wstrict-prototypes -Wold-style-definition -Wpacked                     \
       -Wvector-operation-performance -Wunsuffixed-float-constants             \
       -Wsuggest-attribute=const -Wsuggest-attribute=noreturn                  \
       -Wsuggest-attribute=pure -Wsuggest-attribute=format -Wnormalized=nfkc
# -Waggregate-return -Wwrite-strings

EXPORTS = -DVERSION='"$(VERSION)"' -DPACKAGE='"$(PKGNAME)"' -DLOCALEDIR='"$(LOCALEDIR)"'

FLAGS = $$(pkg-config --cflags libprocps) -std=gnu99 $(EXPORTS) $(WARN)

L = $$(pkg-config --libs libprocps) -largparser


.PHONY: all
all: epkill epgrep epidof

.PHONY: epkill epgrep epidof
epkill: bin/epkill
epgrep: bin/epgrep
epidof: bin/epidof


bin/epkill: bin/epgrep
	mkdir -p bin
	ln -sf epgrep $@

bin/%: obj/%.o obj/environment.o
	mkdir -p bin
	$(CC) $(FLAGS) $(L) -o $@ $^

obj/%.o: src/%.c src/*.h
	mkdir -p obj
	$(CC) $(FLAGS) -c -o $@ $<


.PHONY: install
install: install-base

.PHONY: install-base
install-base: install-cmd install-copyright

.PHONY: install-cmd
install-cmd: install-dpkill install-dpgrep install-dpidof

.PHONY: install-epkill
ifeq ($(EPKILL_AS_SYMLINK),y)
install-epkill: bin/epgrep install-epgrep
	ln -sf epgrep "$(DESTDIR)$(BINDIR)/epkill"
else
install-epkill: bin/epgrep
	install -dm755 -- "$(DESTDIR)$(BINDIR)"
	install -m755 $< -- "$(DESTDIR)$(BINDIR)/epkill"
endif

.PHONY: install-epgrep
install-epgrep: bin/epgrep
	install -dm755 -- "$(DESTDIR)$(BINDIR)"
	install -m755 $< -- "$(DESTDIR)$(BINDIR)/epgrep"

.PHONY: install-epidof
install-epidof: bin/epidof
	install -dm755 -- "$(DESTDIR)$(BINDIR)"
	install -m755 $< -- "$(DESTDIR)$(BINDIR)/epidof"

.PHONY: install-dpkill
install-dpkill: src/dpkill install-epkill
	install -m755 $< -- "$(DESTDIR)$(BINDIR)/dpkill"

.PHONY: install-dpgrep
install-dpgrep: src/dpgrep install-epgrep
	install -m755 $< -- "$(DESTDIR)$(BINDIR)/dpgrep"

.PHONY: install-dpidof
install-dpidof: src/dpidof install-epidof
	install -m755 $< -- "$(DESTDIR)$(BINDIR)/dpidof"

.PHONY: install-copyright
install-copyright: install-copying install-license

.PHONY: install-copying
install-copying: COPYING
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	install -dm755 $< -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/$<"

.PHONY: install-license
install-license: LICENSE
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	install -dm755 $< -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/$<"


.PHONY: uninstall
uninstall:
	-rm -- "$(DESTDIR)$(BINDIR)/dpidof"
	-rm -- "$(DESTDIR)$(BINDIR)/dpgrep"
	-rm -- "$(DESTDIR)$(BINDIR)/dpkill"
	-rm -- "$(DESTDIR)$(BINDIR)/epidof"
	-rm -- "$(DESTDIR)$(BINDIR)/epgrep"
	-rm -- "$(DESTDIR)$(BINDIR)/epkill"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/LICENSE"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/COPYING"
	-rmdir -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"


.PHONY: clean
clean:
	-rm -r obj bin

