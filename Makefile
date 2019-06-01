
# prompt> make
# builds everything and links in test program test_rb
# needs deepstate, with libfuzzer and afl support

SRCS = test_red_black_tree.c red_black_tree.c stack.c misc.c

HDRS = red_black_tree.h stack.h misc.h

OBJS = red_black_tree.o stack.o test_red_black_tree.o misc.o

OBJSJOHNFUZZ = red_black_tree.o stack.o fuzz_red_black_tree.o misc.o container.o

OBJSDS = red_black_tree.o stack.o misc.o container.o

OBJSDSLF = lf_red_black_tree.o lf_stack.o lf_misc.o lf_container.o

OBJSDSAFL = afl_red_black_tree.o afl_stack.o afl_misc.o afl_container.o

OBJSDSSAN = san_red_black_tree.o san_stack.o san_misc.o san_container.o

ifeq ($(origin CC),default)
CC = clang
endif

ifeq ($(origin CXX),default)
CXX = clang++
endif

CFLAGS = -O3 -Wall -pedantic -g

UNIT = test_rb

JOHNFUZZ = fuzz_rb

# DeepState executable
DS = ds_rb

# DeepState executable with sanitizers (can't use with Eclipser)
DSSAN = ds_rb_san

# libFuzzer executable
DSLF = ds_rb_lf

# AFL executable
DSAFL = ds_rb_afl

# easy fuzzer
EASY = easy_ds_rb

all: $(UNIT) $(JOHNFUZZ) $(DS) $(DSLF) $(DSSAN) $(EASY)

$(UNIT): 	$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) -o $(UNIT) $(DMALLOC_LIB)

$(JOHNFUZZ): 	$(OBJSJOHNFUZZ)
		$(CC) $(CFLAGS) $(OBJSJOHNFUZZ) -o $(JOHNFUZZ) $(DMALLOC_LIB)

$(DS): 	$(OBJSDS) deepstate_harness.cpp
		$(CXX) -std=c++14 $(CFLAGS) -o $(DS) deepstate_harness.cpp $(OBJSDS) -ldeepstate

$(DSSAN): 	$(OBJSDSSAN) deepstate_harness.cpp
		$(CXX) -std=c++14 $(CFLAGS) -fsanitize=undefined,integer,address -o $(DSSAN) deepstate_harness.cpp $(OBJSDSSAN) -ldeepstate

$(DSLF): 	$(OBJSDSLF) deepstate_harness.cpp
		$(CXX) -std=c++14 $(CFLAGS) -o $(DSLF) deepstate_harness.cpp $(OBJSDSLF) -ldeepstate_LF -fsanitize=fuzzer,undefined,integer,address

$(DSAFL): 	$(OBJSDSAFL) deepstate_harness.cpp
		afl-clang++ -std=c++14 $(CFLAGS) -o $(DSAFL) deepstate_harness.cpp $(OBJSDSAFL) -ldeepstate_AFL

$(EASY): 	$(OBJSDS) easy_deepstate_fuzzer.cpp
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

afl_red_black_tree.o:	red_black_tree.h stack.h red_black_tree.c stack.c misc.h misc.c
			afl-clang $(CFLAGS) -c -o lf_red_black_tree.o red_black_tree.c

afl_stack.o:		stack.c stack.h misc.h misc.c
			afl-clang $(CFLAGS) -c -o lf_stack.o stack.c

afl_container.o:		container.c container.h
			afl-clang $(CFLAGS) -c -o lf_container.o container.c

afl_misc.o:		misc.h misc.c
			afl-clang $(CFLAGS) -c -o lf_misc.o misc.c

san_red_black_tree.o:	red_black_tree.h stack.h red_black_tree.c stack.c misc.h misc.c
			$(CC) $(CFLAGS) -c -o san_red_black_tree.o red_black_tree.c -fsanitize=undefined,address,integer

san_stack.o:		stack.c stack.h misc.h misc.c
			$(CC) $(CFLAGS) -c -o san_stack.o stack.c -fsanitize=undefined,address,integer

san_container.o:		container.c container.h
			$(CC) $(CFLAGS) -c -o san_container.o container.c -fsanitize=undefined,address,integer

san_misc.o:		misc.h misc.c
			$(CC) $(CFLAGS) -c -o san_misc.o misc.c -fsanitize=undefined,address,integer

clean:			
	rm -f *.o *~ $(UNIT) $(JOHNFUZZ) $(DS) $(DSSAN) $(DSLF) $(EASY) *.gcda *.gcno *.gcov




