# mcpiss makefile
# based off https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

IDIR    = ./include
CC      = gcc
_CFLAGS = $(CFLAGS) -I$(IDIR)
LDFLAGS = -lyyjson

OUT     = mcpiss

ODIR    = obj
LDIR    = ../lib

_DEPS   = mcformat.h rw.h
DEPS    = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ    = main.o mcformat.o rw.o
OBJ     = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: src/%.c $(DEPS)
	mkdir -p obj
	$(CC) -c -o $@ $< $(_CFLAGS) $(LDFLAGS)

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(_CFLAGS) $(LDFLAGS)

.PHONY: clean install

clean:
	rm -rf $(ODIR) $(OUT)
install:
	cp -v $(OUT) /usr/bin/$(OUT)
