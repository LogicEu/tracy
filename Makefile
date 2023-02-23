# tracy makefile

NAME = tracy
CLINAME = tracy_cli

CC = gcc
STD = -std=c99
WFLAGS = -Wall -Wextra -pedantic
OPT = -O2
INC = -I.
LIB = utopia fract photon mass imgtool

RTSRC = rt.c
CLISRC = cli.c

SRCDIR = src
TMPDIR = tmp
LIBDIR = lib

SCRIPT = build.sh

SRC = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(TMPDIR)/%.o,$(SRC))
LIBS = $(patsubst %,$(LIBDIR)/lib%.a,$(LIB))
DLIB = $(patsubst %,-L%, $(LIBDIR))
DLIB += $(patsubst %,-l%, $(LIB))
INC += $(patsubst %,-I%,$(LIB))
INC += -Ispxe

DLIB += -lz -lpng -ljpeg
OPNGL = -lglfw

OS=$(shell uname -s)
ifeq ($(OS),Darwin)
	OPNGL += -framework OpenGL
else
	DLIB += -lm
	OPNGL += -lGL -lGLEW
endif

CFLAGS = $(STD) $(WFLAGS) $(OPT) $(INC)

$(NAME): $(OBJS) $(LIBS) $(RTSRC)
	$(CC) $(CFLAGS) $(DLIB) $(OPNGL) $(OBJS) $(RTSRC) -o $@

.PHONY: cli all clean

$(CLINAME): $(OBJS) $(LIBS) $(CLISRC)
	$(CC) $(CFLAGS) $(DLIB) $(OBJS) $(CLISRC) -o $(CLINAME)

cli: $(CLINAME)

all: $(NAME) $(CLINAME)

$(LIBDIR)/lib%.a: %
	cd $^ && $(MAKE) && mv bin/*.a ../$(LIBDIR)

$(LIBS): | $(LIBDIR)

$(TMPDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS): | $(TMPDIR)

$(TMPDIR):
	mkdir -p $@

$(LIBDIR):
	mkdir -p $@

clean: $(SCRIPT)
	./$^ $@
