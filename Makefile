#*************************************************************************\
#                  Copyright (C) Ed Cole 2016.                            *
#				 colege@gmail.com                         *
#                                                                         *
# This program is free software. You may use, modify, and redistribute it *
# under the terms of the GNU General Public License as published by the   *
# Free Software Foundation, either version 3 or (at your option) any      *
# later version. This program is distributed without any warranty.  See   *
# the file COPYING.gpl-v3 for details.                                    *
#                                                                         *
#*************************************************************************/
TARGET = lutctrl 
LIBS = -lm -lutil -L./lib 
CC = gcc
CFLAGS = -g -Wall
DFLAGS = -DDEBUG=0

.PHONY: default all clean install

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS) 
	$(CC) $(DFLAGS) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

lutctrl: Makefile lutctrl.c readline.c writen.c error_functions.c
	$(CC) $(DFLAGS) lutctrl.c \
		error_functions.c  \
		error.c \
		readline.c \
		writen.c \
		-Wall $(LIBS) -o lutctrl

clean:
	-rm -f *.o lutctrl

 
