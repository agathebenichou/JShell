#
# Makefile for JShell
#

SOURCES  = jshell.c 
HEADERS  = csapp.h
OBJECTS  = $(SOURCES:.c=.o)
RESULT   = ../install/bin/jshell
RESULT   = jshell

CC = gcc 
# CC = gcc Wall

# CFLAGS = -g 
# LDFLAGS = -g
CPPFLAGS = -I.
RM = rm -f 

.SUFFIXES:
.SUFFIXES: .c .o

# $(RESULT): $(OBJECTS)
# 	$(CC) -o $@ $(LDFLAGS) $(OBJECTS)
$(RESULT): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

# .c.o:
# 	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

.c.o:
	$(CC) $(CPPFLAGS) -c $<

clean:
	$(RM) $(RESULT) *.o *.core core *~

# csapp.o: csapp.c $(HEADERS)