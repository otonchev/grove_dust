CC=gcc
CFLAGS=-I. -Werror -pthread -g
LDFLAGS=-lm -lpthread

MYSQL_CFLAGS=`mysql_config --cflags`
MYSQL_LDFLAGS=`mysql_config --libs`

DEPS = lngpio.h air_utils.h
OBJ = lngpio.o air_utils.o test.o
OBJ_ASYNC = lngpio.o air_utils.o test_async.o
OBJ_MYSQL = lngpio.o air_utils.o test_mysql.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

test_async:  $(OBJ_ASYNC)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

test_mysql:  CFLAGS := $(MYSQL_CFLAGS)
test_mysql:  LDFLAGS := $(MYSQL_LDFLAGS)
test_mysql:  $(OBJ_MYSQL)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)
