CXX = g++
CCC = g++
CFLAGS = -std=c++0x -ggdb -Wall -Wextra -pedantic -Werror
CXXFLAGS = $(CFLAGS)
DEPS = httpd.h connection.h util.h http.h server.h mocks.h listener.h request_handlers.h file_repository.h
SRCS = httpd.cpp connection.cpp util.cpp http.cpp server.cpp mocks.cpp listener.cpp request_handlers.cpp file_repository.cpp
OBJ_DIR = build
MAIN_SRCS = main.cpp $(SRCS)
MAIN_OBJS = $(MAIN_SRCS:%.cpp=$(OBJ_DIR)/%.o)
TEST_SRCS = test.cpp $(SRCS)
TEST_OBJS = $(TEST_SRCS:%.cpp=$(OBJ_DIR)/%.o)


.PHONY: default run test dirs clean


default: dirs httpd

$(OBJ_DIR)/%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

httpd: $(MAIN_OBJS)
	$(CXX) $(CXXFLAGS) -o httpd $(MAIN_OBJS) -lpthread

run: dirs httpd
	./httpd 6060 files

test_httpd: $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o test_httpd $(TEST_OBJS) -lpthread

itest: dirs httpd
	./integration_test.py

test: dirs test_httpd
	./test_httpd

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf httpd test_httpd *.o $(OBJ_DIR)

dirs:
	mkdir -p $(OBJ_DIR)
