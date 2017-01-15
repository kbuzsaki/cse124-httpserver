
CC=g++
CFLAGS=-std=c++0x -ggdb -Wall -Wextra -pedantic -Werror
CXXFLAGS=-std=c++0x
DEPS = httpd.h connection.h util.h http.h
SRCS = httpd.cpp connection.cpp util.cpp http.cpp
MAIN_SRCS = main.c $(SRCS)
MAIN_OBJS = $(MAIN_SRCS:.c=.o)
TEST_SRCS = test.c $(SRCS)
TEST_OBJS = $(TEST_SRCS:.c=.o)

default: httpd

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

httpd:    $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o httpd $(MAIN_OBJS) -lpthread

run: httpd
	./httpd 6060 files

test_httpd: $(TEST_OBJS)
	$(CC) $(CFLAGS) -o test_httpd $(TEST_OBJS) -lpthread

test: test_httpd
	./test_httpd

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f httpd test_httpd *.o
