CC=gcc
CFLAGS=-I.
DEPS = lngpio.h
OBJ = lngpio.o test.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) -lm
