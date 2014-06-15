PREFIX = /usr
BIN = /bin
BINDIR = $(PREFIX)$(BIN)
DATA = /share
DATADIR = $(PREFIX)$(DATA)
INFODIR = $(DATADIR)/info
LICENSES = $(DATADIR)/licenses
LOCALEDIR = $(DATADIR)/locale

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


.PHONY: clean
clean:
	-rm -r obj bin

