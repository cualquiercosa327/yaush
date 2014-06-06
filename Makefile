CFILES = $(wildcard *.c)
TARGETS = $(CFILES:.c=)
CC = gcc
CFLAGS = -lreadline -lncurses  -g3

all: $(TARGETS)
	
$(TARGETS): $(CFILES)
	$(CC) $(@:=.c) $(CFLAGS) -o $@
clean: $(TARGETS)
	rm $(TARGETS)
