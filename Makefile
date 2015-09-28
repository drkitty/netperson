MAKEFLAGS += --no-builtin-rules

.SUFFIXES:
.SECONDEXPANSION:


EXE_SRC := np.c
SRC := $(EXE_SRC) fail.c

OBJ := $(SRC:%.c=%.o)
EXE := $(EXE_SRC:%.c=%)

CC := gcc
CFLAGS := -std=c99 -g -Wall -Wextra -Werror -Wno-unused-function \
	-I /usr/include/libnl3

LDFLAGS :=


all: $(EXE) $(EXTRA_EXE)

$(OBJ): $$(patsubst %.o,%.c,$$@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXE) $(EXTRA_EXE):
	$(CC) -o $@ $^ $(LDFLAGS)

$(EXE): $$@.o

clean:
	rm -f $(OBJ) $(EXE)


fail.o: fail.h

np: fail.o


.DEFAULT_GOAL := all
.PHONY: all clean
