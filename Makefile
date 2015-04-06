PREFIX = /usr
BIN = /bin
BINDIR = $(PREFIX)$(BIN)
DATA = /share
DATADIR = $(PREFIX)$(DATA)
DOCDIR = $(DATADIR)/doc
INFODIR = $(DATADIR)/info
LOCALEDIR = $(DATADIR)/locale
LICENSEDIR = $(DATADIR)/licenses

PKGNAME = epkill
VERSION = 1.1


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

OPTIMISE = -Og -g

EXPORTS = -DVERSION='"$(VERSION)"' -DPACKAGE='"$(PKGNAME)"' -DLOCALEDIR='"$(LOCALEDIR)"'

FLAGS = $$(pkg-config --cflags libprocps) -std=gnu99 $(OPTIMISE) $(EXPORTS) $(WARN)

L = $$(pkg-config --libs libprocps) -largparser



.PHONY: all
default: cmd info

.PHONY: all
all: cmd doc

.PHONY: base
base: cmd


.PHONY: cmd
cmd: epkill epgrep epidof

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


.PHONY: doc
doc: info pdf dvi ps

.PHONY: info
info: epkill.info
%.info: info/%.texinfo info/fdl.texinfo
	makeinfo $<

.PHONY: pdf
pdf: epkill.pdf
%.pdf: info/%.texinfo info/fdl.texinfo
	mkdir -p obj/pdf
	cd obj/pdf ; yes X | texi2pdf ../../$<
	mv obj/pdf/$@ $@

.PHONY: dvi
dvi: epkill.dvi
%.dvi: info/%.texinfo info/fdl.texinfo
	mkdir -p obj/dvi
	cd obj/dvi ; yes X | $(TEXI2DVI) ../../$<
	mv obj/dvi/$@ $@

.PHONY: ps
ps: epkill.ps
%.ps: info/%.texinfo info/fdl.texinfo
	mkdir -p obj/ps
	cd obj/ps ; yes X | texi2pdf --ps ../../$<
	mv obj/ps/$@ $@



.PHONY: install
install: install-base install-info

.PHONY: install-base
install-base: install-cmd install-copyright

.PHONY: install-base
install-all: install-base install-doc

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
	install -m644 $< -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/$<"

.PHONY: install-license
install-license: LICENSE
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	install -m644 $< -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/$<"


.PHONY: install-doc
install-doc: install-info install-pdf install-ps install-dvi

.PHONY: install-info
install-info: epkill.info
	install -dm755 -- "$(DESTDIR)$(INFODIR)"
	install -m644 $< -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"

.PHONY: install-pdf
install-pdf: epkill.pdf
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"

.PHONY: install-ps
install-ps: epkill.ps
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"

.PHONY: install-dvi
install-dvi: epkill.dvi
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"



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
	-rm -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"



.PHONY: clean
clean:
	-rm -r obj bin

