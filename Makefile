#general compilation
CC = gcc
CFLAGS = -pipe -Wall -Wextra -std=c99 -pedantic-errors -Werror -O3
CFLAGS = -pipe -Wall -Wextra -std=c99 -pedantic -g

LNFLAGS =

# To build the library
AR = ar
ARFLAGS = rcv

# code-correctness checks
LINT = splint
LINTFLAGS = +weak +posixstrictlib

# Set RANLIB to ranlib on systems that require it (Sun OS < 4, Mac OSX)
RANLIB  = ranlib
RANLIB = true

TARGET = ccbor.a
TARGET = test_program

.SUFFIXES:
SUFFIXES =

OBJS = ccbor.o cbor_int.o cbor_str.o cbor_arr.o cbor_map.o

LIBRARIES = 

.PHONY : all
all : $(TARGET)

.PHONY : clean
clean :
	-rm ccbor.a
	-rm $(OBJS) program.o
	-find . -iname "*.h.gch" -execdir rm {} \;

test_program : program.o ccbor.a
	$(CC) $(CFLAGS) -fwhole-program $^ $(LNFLAGS) -o $@

ccbor.a : $(OBJS)
	$(AR) $(ARFLAGS) ccbor.a $(OBJS)
	$(RANLIB) ccbor.a

%.o : %.c %.h.gch
	#-$(LINT) $(LINTFLAGS) $<
	$(CC) $(CFLAGS) -o $@ -c $<

%.h.gch : %.h
	$(CC) $(CFLAGS) -o $@ -c $^

