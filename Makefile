# SPDX-License-Identifier: MIT
# Copyright (c) 2023, Russ Dill <russ.dill@gmail.com>

CFLAGS = -O2 -Werror -Wall -g
CC = gcc

OBJECTS = uc-con.o uc-cmd.o example.o

all: example

example: $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJECTS): uc-con.h uc-cmd.h uc-io.h config.h Makefile

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(OBJECTS) example
