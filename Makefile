# mcpiss makefile
# based off https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

CC=gcc
PROGCFLAGS=$(CFLAGS) -I.
LDFLAGS=-lyyjson
DEPS=mcformat.h
OBJ=main.o mcformat.o
OUT=mcpiss

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(PROGCFLAGS)

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(PROGCFLAGS) $(LDFLAGS)

.PHONY: clean install

clean:
	rm -vf *.o $(OUT)

install:
	cp -v $(OUT) /usr/bin/$(OUT)
