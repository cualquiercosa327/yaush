CFILES = $(wildcard *.c)
TARGETS = $(CFILES:.c=)
CC = gcc
#CFLAGS = -lreadline -lncurses -Wall -D DEBUG -g3
CFLAGS = -lreadline -lncurses -Wall

all: $(TARGETS)
	
$(TARGETS): $(CFILES)
	$(CC) $(@:=.c) $(CFLAGS) -o $@
clean: $(TARGETS)
	rm $(TARGETS)
