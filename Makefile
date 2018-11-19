
# prompt> make
# builds everything and links in test program test_rb
#
# prompt> make mem_check
# Rebuilds everything using dmalloc and does memory testing.
# Only works if you have dmalloc installed (see http://dmalloc.com).

SRCS = test_red_black_tree.c red_black_tree.c stack.c misc.c

HDRS = red_black_tree.h stack.h misc.h

OBJS = red_black_tree.o stack.o test_red_black_tree.o misc.o

OBJS2 = red_black_tree.o stack.o fuzz_red_black_tree.o misc.o container.o

OBJSDS = red_black_tree.o stack.o misc.o container.o

CC = clang -fsanitize=undefined,integer,address
#CC = clang -fsanitize=integer

#CFLAGS = -g -O0 -coverage -fprofile-arcs -Wall -pedantic
CFLAGS = -O3 -Wall -pedantic

PROGRAM = test_rb

PROGRAM2 = fuzz_rb

# DeepState executable
DS1 = ds_rb

# libFuzzer executable
DS2 = ds_rb_lf

.PHONY:	mem_check clean

all: $(PROGRAM) $(PROGRAM2)

$(PROGRAM): 	$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(DMALLOC_LIB)

$(PROGRAM2): 	$(OBJS2)
		$(CC) $(CFLAGS) $(OBJS2) -o $(PROGRAM2) $(DMALLOC_LIB)

$(DS1): 	$(OBJSDS) deepstate_harness.cpp
		clang++ $(CFLAGS) -o $(DS1) deepstate_harness.cpp $(OBJSDS) -ldeepstate -fsanitize=undefined,integer,address

$(DS2): 	$(OBJSDS) deepstate_harness.cpp
		clang++ $(CFLAGS) -o $(DS2) deepstate_harness.cpp $(OBJSDS) -ldeepstate_LF -fsanitize=fuzzer,undefined,integer,address

mem_check:	
		@if [ -e makefile.txt ] ; then \
			echo "Using makefile.txt" ; \
			$(MAKE) clean -f makefile.txt ; \
			$(MAKE) $(PROGRAM) "CFLAGS=$(CFLAGS) -DDMALLOC" "DMALLOC_LIB=-ldmalloc" -f makefile.txt ; \
		else \
			echo "Using default makefile (i.e. no -f flag)." ; \
			$(MAKE) clean ; \
			$(MAKE) $(PROGRAM) "CFLAGS=$(CFLAGS) -DDMALLOC" "DMALLOC_LIB=-ldmalloc" ; \
		fi
		./simple_test.sh
		@if [ -s unfreed.txt ] ; then \
			echo " " ; \
			echo "Leaked some memory.  See logfile for details." ;\
		else \
			echo " " ; \
			echo "No memory leaks detected. " ;\
			echo " " ; \
			echo "Test passed. " ; \
			echo " " ; \
		fi


test_red_black_tree.o:	test_red_black_tree.c red_black_tree.c stack.c stack.h red_black_tree.h misc.h

red_black_tree.o:	red_black_tree.h stack.h red_black_tree.c stack.c misc.h misc.c

stack.o:		stack.c stack.h misc.h misc.c

clean:			
	rm -f *.o *~ $(PROGRAM) $(PROGRAM2) *.gcda *.gcno *.gcov unfreed.txt







