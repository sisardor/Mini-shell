#Makefile 5
IDIR=../include
CC=gcc
CFLAGS=-I$(IDIR)

RPOGAM_NAME=minishell

ODIR=obj
LDIR =../lib
LIBS=-lm

_DEPS = minishell.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = minishell.o minishellfunc.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ -g $< $(CFLAGS)

$(RPOGAM_NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

run:
	./$(RPOGAM_NAME)

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~
