CFILES = $(wildcard *.c)
CHEADERS = $(wildcard *.h)
OBJECTS = $(patsubst %c, %o, $(CFILES))
TARGETS = yaush 
CC = gcc
#CFLAGS = -lreadline -lncurses -Wall -D DEBUG -g3
CFLAGS = -lreadline -lncurses -Wall

all: $(TARGETS)

$(TARGETS): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) -o $@
	
$(OBJECTS): $(CFILES) $(CHEADERS)
	$(CC) $(@:.o=.c) $(CFLAGS) -c

clean: $(TARGETS) $(OBJECTS)
	rm $(TARGETS)
	rm $(OBJECTS)
