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
OBJECTS += log.o
OBJECTS += parse.o
OBJECTS += response.o
OBJECTS += socket.o
OBJECTS += io.o
OBJECTS += https.o
OBJECTS += cgi.o

default: lisod

lisod: $(OBJECTS)
	@gcc -o lisod -Wall -Werror $(LFLAGS) $(OBJECTS)

.phony: clean

run:
	./lisod 1440 1441 ./log/log lock www flaskr/flaskr.py ./cert/yuruiz.key ./cert/yuruiz.crt

clean:
	@rm $(OBJECTS)
