# tracy makefile

STD=-std=c99
WFLAGS=-Wall -Wextra
OPT=-O2
IDIR=-I.
LIBS=fract utopia photon mass imgtool
CC=gcc
NAME=tracy
SRC=src/*.c

LDIR=lib
IDIR += $(patsubst %,-I%/,$(LIBS))
LSTATIC=$(patsubst %,lib%.a,$(LIBS))
LPATHS=$(patsubst %,$(LDIR)/%,$(LSTATIC))
LFLAGS=$(patsubst %,-L%,$(LDIR))
LFLAGS += $(patsubst %,-l%,$(LIBS))
LFLAGS += -lz -lpng -ljpeg

SCRIPT=build.sh

OS=$(shell uname -s)
ifeq ($(OS),Darwin)
	OSFLAGS=-mmacos-version-min=10.10
else 
	OSFLAGS=-lm -lpthread -D_POSIX_C_SOURCE=199309L
endif

CFLAGS=$(STD) $(WFLAGS) $(OPT) $(IDIR)

$(NAME): $(LPATHS) $(SRC)
	$(CC) -o $@ $(SRC) $(CFLAGS) $(LFLAGS) $(OSFLAGS)

$(LDIR)/$(LDIR)%.a: $(LDIR)%.a $(LDIR)
	mv $< $(LDIR)/

$(LDIR): 
	@[ -d $@ ] || mkdir $@ && echo "mkdir $@"

$(LDIR)%.a: %
	cd $^ && make && mv $@ ../

exe:
	$(CC) -o $(NAME) $(SRC) $(CFLAGS) $(LFLAGS) $(OSFLAGS)

clean: $(SCRIPT)
	./$^ $@
    
install: $(SCRIPT)
	./$^ $@
 
uninstall: $(SCRIPT)
	./$^ $@ 
