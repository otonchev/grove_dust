CC=gcc
CFLAGS=-I. -Werror -pthread -g
DEPS = lngpio.h air_utils.h
OBJ = lngpio.o air_utils.o test.o
OBJ_ASYNC = lngpio.o air_utils.o test_async.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) -lm -lpthread

test_async:  $(OBJ_ASYNC)
	gcc -o $@ $^ $(CFLAGS) -lm -lpthread
