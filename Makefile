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
VERSION = 1.2


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
default: cmd info shell

.PHONY: all
all: cmd doc shell

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
info: bin/epkill.info
bin/%.info: doc/info/%.texinfo doc/info/fdl.texinfo
	mkdir -p bin
	makeinfo $<
	mv $*.info $@

.PHONY: pdf
pdf: bin/epkill.pdf
bin/%.pdf: doc/info/%.texinfo doc/info/fdl.texinfo
	mkdir -p obj/pdf bin
	cd obj/pdf && texi2pdf ../../$< < /dev/null
	mv obj/pdf/$*.pdf $@

.PHONY: dvi
dvi: bin/epkill.dvi
bin/%.dvi: doc/info/%.texinfo doc/info/fdl.texinfo
	mkdir -p obj/dvi bin
	cd obj/dvi && $(TEXI2DVI) ../../$< < /dev/null
	mv obj/dvi/$*.dvi $@

.PHONY: ps
ps: bin/epkill.ps
bin/%.ps: doc/info/%.texinfo doc/info/fdl.texinfo
	mkdir -p obj/ps bin
	cd obj/ps && texi2pdf --ps ../../$< < /dev/null
	mv obj/ps/$*.ps $@


.PHONY: shell
shell: bash fish zsh

.PHONY: bash
bash: bash-epkill bash-dpkill bash-epgrep bash-dpgrep bash-epidof bash-dpidof

.PHONY: fish
fish: fish-epkill fish-dpkill fish-epgrep fish-dpgrep fish-epidof fish-dpidof

.PHONY: zsh
zsh: zsh-epkill zsh-dpkill zsh-epgrep zsh-dpgrep zsh-epidof zsh-dpidof

.PHONY: bash-epkill
bash-epkill: bin/epkill.bash-completion

.PHONY: fish-epkill
fish-epkill: bin/epkill.fish-completion

.PHONY: zsh-epkill
zsh-epkill: bin/epkill.zsh-completion

.PHONY: bash-dpkill
bash-dpkill: bin/dpkill.bash-completion

.PHONY: fish-dpkill
fish-dpkill: bin/dpkill.fish-completion

.PHONY: zsh-dpkill
zsh-dpkill: bin/dpkill.zsh-completion

.PHONY: bash-epgrep
bash-epgrep: bin/epgrep.bash-completion

.PHONY: fish-epgrep
fish-epgrep: bin/epgrep.fish-completion

.PHONY: zsh-epgrep
zsh-epgrep: bin/epgrep.zsh-completion

.PHONY: bash-dpgrep
bash-dpgrep: bin/dpgrep.bash-completion

.PHONY: fish-dpgrep
fish-dpgrep: bin/dpgrep.fish-completion

.PHONY: zsh-dpgrep
zsh-dpgrep: bin/dpgrep.zsh-completion

.PHONY: bash-epidof
bash-epidof: bin/epidof.bash-completion

.PHONY: fish-epidof
fish-epidof: bin/epidof.fish-completion

.PHONY: zsh-epidof
zsh-epidof: bin/epidof.zsh-completion

.PHONY: bash-dpidof
bash-dpidof: bin/dpidof.bash-completion

.PHONY: fish-dpidof
fish-dpidof: bin/dpidof.fish-completion

.PHONY: zsh-dpidof
zsh-dpidof: bin/dpidof.zsh-completion

bin/%.bash-completion: src/%.auto-completion
	@mkdir -p bin
	auto-auto-complete bash --output $@ --source $<

bin/%.fish-completion: src/%.auto-completion
	@mkdir -p bin
	auto-auto-complete fish --output $@ --source $<

bin/%.zsh-completion: src/%.auto-completion
	@mkdir -p bin
	auto-auto-complete zsh --output $@ --source $<



.PHONY: install
install: install-base install-info install-shell

.PHONY: install-base
install-all: install-base install-doc install-shell

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
	install -m644 $< -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/$<"

.PHONY: install-license
install-license: LICENSE
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	install -m644 $< -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/$<"


.PHONY: install-doc
install-doc: install-info install-pdf install-ps install-dvi

.PHONY: install-info
install-info: bin/epkill.info
	install -dm755 -- "$(DESTDIR)$(INFODIR)"
	install -m644 $< -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"

.PHONY: install-pdf
install-pdf: bin/epkill.pdf
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"

.PHONY: install-ps
install-ps: bin/epkill.ps
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"

.PHONY: install-dvi
install-dvi: bin/epkill.dvi
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"


.PHONY: install-shell
install-shell: install-bash install-fish install-zsh

.PHONY: install-bash
install-bash: install-bash-epkill install-bash-epgrep install-bash-epidof \
              install-bash-dpkill install-bash-dpgrep install-bash-dpidof

.PHONY: install-bash-epkill
install-bash-epkill: bin/epkill.bash-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/bash-completion/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/epkill"

.PHONY: install-bash-epgrep
install-bash-epgrep: bin/epgrep.bash-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/bash-completion/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/epgrep"

.PHONY: install-bash-epidof
install-bash-epidof: bin/epidof.bash-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/bash-completion/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/epidof"

.PHONY: install-bash-dpkill
install-bash-dpkill: bin/dpkill.bash-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/bash-completion/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/dpkill"

.PHONY: install-bash-dpgrep
install-bash-dpgrep: bin/dpgrep.bash-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/bash-completion/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/dpgrep"

.PHONY: install-bash-dpidof
install-bash-dpidof: bin/dpidof.bash-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/bash-completion/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/dpidof"

.PHONY: install-fish
install-fish: install-fish-epkill install-fish-epgrep install-fish-epidof \
              install-fish-dpkill install-fish-dpgrep install-fish-dpidof

.PHONY: install-fish-epkill
install-fish-epkill: bin/epkill.fish-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/fish/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/fish/completions/epkill.fish"

.PHONY: install-fish-epgrep
install-fish-epgrep: bin/epgrep.fish-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/fish/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/fish/completions/epgrep.fish"

.PHONY: install-fish-epidof
install-fish-epidof: bin/epidof.fish-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/fish/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/fish/completions/epidof.fish"

.PHONY: install-fish-dpkill
install-fish-dpkill: bin/dpkill.fish-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/fish/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/fish/completions/dpkill.fish"

.PHONY: install-fish-dpgrep
install-fish-dpgrep: bin/dpgrep.fish-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/fish/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/fish/completions/dpgrep.fish"

.PHONY: install-fish-dpidof
install-fish-dpidof: bin/dpidof.fish-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/fish/completions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/fish/completions/dpidof.fish"

.PHONY: install-zsh
install-zsh: install-zsh-epkill install-zsh-epgrep install-zsh-epidof \
             install-zsh-dpkill install-zsh-dpgrep install-zsh-dpidof

.PHONY: install-zsh-epkill
install-zsh-epkill: bin/epkill.zsh-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/zsh/site-functions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_epkill"

.PHONY: install-zsh-epgrep
install-zsh-epgrep: bin/epgrep.zsh-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/zsh/site-functions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_epgrep"

.PHONY: install-zsh-epidof
install-zsh-epidof: bin/epidof.zsh-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/zsh/site-functions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_epidof"

.PHONY: install-zsh-dpkill
install-zsh-dpkill: bin/dpkill.zsh-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/zsh/site-functions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_dpkill"

.PHONY: install-zsh-dpgrep
install-zsh-dpgrep: bin/dpgrep.zsh-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/zsh/site-functions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_dpgrep"

.PHONY: install-zsh-dpidof
install-zsh-dpidof: bin/dpidof.zsh-completion
	install -dm755 -- "$(DESTDIR)$(DATADIR)/zsh/site-functions"
	install -m644 $< -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_dpidof"



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
	-rm -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/epkill"
	-rm -- "$(DESTDIR)$(DATADIR)/fish/completions/epkill.fish"
	-rm -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_epkill"
	-rm -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/dpkill"
	-rm -- "$(DESTDIR)$(DATADIR)/fish/completions/dpkill.fish"
	-rm -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_dpkill"
	-rm -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/epgrep"
	-rm -- "$(DESTDIR)$(DATADIR)/fish/completions/epgrep.fish"
	-rm -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_epgrep"
	-rm -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/dpgrep"
	-rm -- "$(DESTDIR)$(DATADIR)/fish/completions/dpgrep.fish"
	-rm -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_dpgrep"
	-rm -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/epidof"
	-rm -- "$(DESTDIR)$(DATADIR)/fish/completions/epidof.fish"
	-rm -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_epidof"
	-rm -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/dpidof"
	-rm -- "$(DESTDIR)$(DATADIR)/fish/completions/dpidof.fish"
	-rm -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_dpidof"



.PHONY: clean
clean:
	-rm -r obj bin

