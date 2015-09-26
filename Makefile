MAKEFLAGS += --no-builtin-rules

.SUFFIXES:
.SECONDEXPANSION:


EXE_SRC := nw.c
SRC := $(EXE_SRC)

OBJ := $(SRC:%.c=%.o)
EXE := $(EXE_SRC:%.c=%)

CC := gcc
CFLAGS := -std=c99 -g -Wall -Wextra -Werror -Wno-unused-function \
	-I /usr/include/libnl3

LDFLAGS := -lnl-genl-3 -lnl-3


all: $(EXE) $(EXTRA_EXE)

$(OBJ): $$(patsubst %.o,%.c,$$@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXE) $(EXTRA_EXE):
	$(CC) $(LDFLAGS) -o $@ $^

$(EXE): $$@.o

clean:
	rm -f $(OBJ) $(EXE)


#nw:


.DEFAULT_GOAL := all
.PHONY: all clean
