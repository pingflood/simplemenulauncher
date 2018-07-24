#
# Makefile for Linux
# It's mostly for conveniance and also so i don't have to recompile SDL2 each time.
#

CC = gcc
OUTPUTNAME = filebrws.elf

DEFINES = 
INCLUDES = -I.
OPT_FLAGS  = -O0 -g

CFLAGS = $(DEFINES) $(INCLUDES) $(OPT_FLAGS) -std=gnu11 
LDFLAGS = -lSDL -lSDL_image

# Redream (main engine)
OBJS =  main.o graphics.o
 
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< 
	

all: executable

executable : $(OBJS)
	$(CC) -o $(OUTPUTNAME) $(OBJS) $(CFLAGS) $(LDFLAGS)

clean:
	rm $(OBJS) $(OUTPUTNAME)
