# tracy makefile

STD=-std=c99
WFLAGS=-Wall -Wextra
OPT=-O2
IDIR=-I. -Ispxe
LIBS=mass fract utopia photon imgtool
CC=gcc
NAME=tracy
SRC=src/*.c

CLI=main_cli.c
RUN=main_run.c

LDIR=lib
IDIR += $(patsubst %,-I%/,$(LIBS))
LSTATIC=$(patsubst %,lib%.a,$(LIBS))
LPATHS=$(patsubst %,$(LDIR)/%,$(LSTATIC))
LFLAGS=$(patsubst %,-L%,$(LDIR))
LFLAGS += $(patsubst %,-l%,$(LIBS))
LFLAGS += -lz -lpng -ljpeg
GL=-lglfw

SCRIPT=build.sh

OS=$(shell uname -s)
ifeq ($(OS),Darwin)
    #OSFLAGS=-mmacos-version-min=10.10
    GL+=-framework OpenGL
else 
	OSFLAGS=-lm -lpthread -D_POSIX_C_SOURCE=199309L
    GL+=-lGL -lGLEW
endif

CFLAGS=$(STD) $(WFLAGS) $(OPT) $(IDIR)

$(NAME): $(LPATHS) $(SRC)
	$(CC) -o $@ $(SRC) $(CLI) $(CFLAGS) $(LFLAGS) $(OSFLAGS)

$(LDIR)/$(LDIR)%.a: $(LDIR)%.a $(LDIR)
	mv $< $(LDIR)/

$(LDIR): 
	@[ -d $@ ] || mkdir $@ && echo "mkdir $@"

$(LDIR)%.a: %
	cd $^ && make && mv $@ ../

cli:
	$(CC) -o $(NAME) $(SRC) $(CLI) $(CFLAGS) $(LFLAGS) $(OSFLAGS)

run:
	$(CC) -o $(NAME) $(SRC) $(RUN) $(CFLAGS) $(LFLAGS) $(GL) $(OSFLAGS)

clean: $(SCRIPT)
	./$^ $@
    
install: $(SCRIPT)
	./$^ $@
 
uninstall: $(SCRIPT)
	./$^ $@ 
