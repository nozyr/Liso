################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for Recitation 1.             #
#                                                                              #
# Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                          #
#          Wolf Richter <wolf@cs.cmu.edu>                                      #
#                                                                              #
################################################################################


SOURCE=src
VPATH=$(SOURCE)
LFLAGS=-lssl

OBJECTS = liso.o
OBJECTS += conn.o
OBJECTS += daemon.o
OBJECTS += log.o
OBJECTS += parse.o
OBJECTS += parse.o
OBJECTS += response.o
OBJECTS += socket.o

default: lisod

lisod: $(OBJECTS)
	@gcc -o lisod -Wall -Werror $(LFLAGS) $(OBJECTS)

.phony: clean
clean:
	@rm $(OBJECTS)
