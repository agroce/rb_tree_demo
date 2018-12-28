
# prompt> make
# builds everything and links in test program test_rb

SRCS = test_red_black_tree.c red_black_tree.c stack.c misc.c

HDRS = red_black_tree.h stack.h misc.h

OBJS = red_black_tree.o stack.o test_red_black_tree.o misc.o

OBJS2 = red_black_tree.o stack.o fuzz_red_black_tree.o misc.o container.o

OBJSDS = red_black_tree.o stack.o misc.o container.o

OBJSDSLF = lf_red_black_tree.o lf_stack.o lf_misc.o lf_container.o

ifeq ($(origin CC),default)
CC = clang
endif

ifeq ($(origin CXX),default)
CXX = clang++
endif

CFLAGS = -O3 -Wall -pedantic -g -fsanitize=undefined,integer,address

PROGRAM = test_rb

PROGRAM2 = fuzz_rb

# DeepState executable
DS1 = ds_rb

# libFuzzer executable
DS2 = ds_rb_lf

# easy fuzzer
EASY = easy_ds_rb

all: $(PROGRAM) $(PROGRAM2) $(DS1) $(DS2) $(EASY)

$(PROGRAM): 	$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(DMALLOC_LIB)

$(PROGRAM2): 	$(OBJS2)
		$(CC) $(CFLAGS) $(OBJS2) -o $(PROGRAM2) $(DMALLOC_LIB)

$(DS1): 	$(OBJSDS) deepstate_harness.cpp
		$(CXX) -std=c++14 $(CFLAGS) -o $(DS1) deepstate_harness.cpp $(OBJSDS) -ldeepstate

$(DS2): 	$(OBJSDSLF) deepstate_harness.cpp
		$(CXX) -std=c++14 $(CFLAGS) -o $(DS2) deepstate_harness.cpp $(OBJSDSLF) -ldeepstate_LF -fsanitize=fuzzer,undefined,integer,address

$(EASY): 	$(OBJSDS) deepstate_harness.cpp
		$(CXX) -std=c++14 $(CFLAGS) -o $(EASY) easy_deepstate_fuzzer.cpp $(OBJSDS) -ldeepstate

test_red_black_tree.o:	test_red_black_tree.c red_black_tree.c stack.c stack.h red_black_tree.h misc.h

red_black_tree.o:	red_black_tree.h stack.h red_black_tree.c stack.c misc.h misc.c

stack.o:		stack.c stack.h misc.h misc.c

lf_red_black_tree.o:	red_black_tree.h stack.h red_black_tree.c stack.c misc.h misc.c
			$(CC) $(CFLAGS) -c -o lf_red_black_tree.o red_black_tree.c -fsanitize=fuzzer-no-link,undefined,address,integer

lf_stack.o:		stack.c stack.h misc.h misc.c
			$(CC) $(CFLAGS) -c -o lf_stack.o stack.c -fsanitize=fuzzer-no-link,undefined,address,integer

lf_container.o:		container.c container.h
			$(CC) $(CFLAGS) -c -o lf_container.o container.c -fsanitize=fuzzer-no-link,undefined,address,integer

lf_misc.o:		misc.h misc.c
			$(CC) $(CFLAGS) -c -o lf_misc.o misc.c -fsanitize=fuzzer-no-link,undefined,address,integer

clean:			
	rm -f *.o *~ $(PROGRAM) $(PROGRAM2) $(DS1) $(DS2) $(EASY) *.gcda *.gcno *.gcov




