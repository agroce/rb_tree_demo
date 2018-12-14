# Fuzzing an API with DeepState

In 2013, John Regehr wrote a blog post on ["How to Fuzz an ADT Implementation."](https://blog.regehr.org/archives/896)  John talked at some length about general issues in gaining confidence that a data-type implementation is reliable, discussing code coverage, test oracles, and differential testing.  The article is well worth reading (it would be a good idea to read it before reading the rest of this article, in fact), and it gives a good overview of how to construct a simple custom fuzzer for an ADT, or, for that matter, any fairly self-contained API where there are good ways to check for correctness.

The general problem is simple.  Suppose we have a piece of software that is, essentially, a library: a red-black tree, an AVL tree, a [file-system](https://github.com/agroce/testfs), an [in-memory store](https://github.com/redis/hiredis), or even a [crypto library](https://botan.randombit.net/).  The software provides a set of functions (or methods on objects), and we have some expectations about what will happen when we call those functions.  Traditional unit testing addresses this problem by having the developer (or us) write a series of small functions that look like:

```
result1 = foo(3, "hello");
result2 = bar(result1, "goodbye")
assert(result2 == DONE);
```

That is, the tests each are of the form: "do something, then check that it did the right thing."   This approach has two problems.  First, it's a lot of work.  Second, the return on investment for that work is not as good as you would hope; each test does one specific thing, and if the author of the tests doesn't happen to think of a potential problem, the tests are very unlikely to catch that problem.  These unit tests are insufficient for the same reasons that [AFL](http://lcamtuf.coredump.cx/afl/) and other fuzzers have been so successful at finding security vulnerabilities in widely-used programs:  humans are too slow to write many tests, and are limited in their ability to imagine insane, harmful inputs.  The randomness of fuzzing makes it possible to produce many tests very quickly and results in tests that go far outside the "expected uses."  Fuzzing is often thought of as generating files or packets, but it can also generate sequences of API calls to test software libraries.  Such fuzzing is often referred to as [random](https://www.cs.tufts.edu/~nr/cs257/archive/john-hughes/quick.pdf) or [randomized](https://agroce.github.io/icse07.pdf) testing, but _fuzzing is fuzzing_.  Instead of a series of unit tests doing one specific thing, a fuzzer test (also known as a [property-based test](https://fsharpforfunandprofit.com/posts/property-based-testing/) or a [parameterized unit test](https://www.microsoft.com/en-us/research/publication/parameterized-unit-tests/)) looks more like:

```
foo_result = NULL;
bar_result = NULL;
switch (choice):
  choose_foo:
    foo_result = foo(randomInt(), randomString());
    break;
  choose_bar:
    bar_result = bar(foo_result, randomString();
    break;
  choose_baz:
    baz_result = baz(foo_result, bar_result);
    break;  
checkInvariants();
```

That is, the fuzzer repeatedly (randomly) chooses some function call to make, and then makes it, perhaps storing the results for use in other function calls, and includes lots of generalized assertions about how the system should behave.  The most obvious such checks are any assertions in the code, but there are numerous other possibilities.  For a data structure, this will come in the form of a `repOK` function that makes sure, e.g., you have a valid red-black tree.  For a file system, you may expect that `chkdsk` will never find any errors after a series of valid file system operations.  In a crypto library (or a JSON parser, for that matter) you may want to check round-trip properties -- `message == decode(encode(message, key), key)`.  In many cases, such as with ADTs and file systems, you can use another implementation of (almost) the same functionality, and compare results.  Such _differential_ testing is extremely powerful, because it lets you write a very complete specification of correctness with relatively little work.

John's post doesn't just give general advice, it includes links to a [working fuzzer for a red-black tree](https://github.com/regehr/rb_tree_demo).  The fuzzer is effective and a good example of how to really hammer an API.  However, it's also not a completely practical testing tool.  It generates inputs, and tests the red-black tree, but when the fuzzer finds a bug, it simply prints an error message and crashes.  You don't learn anything but "Yes, your code has a bug, and here is the symptom."  This is not ideal.  Modifying the code to print out the test steps as they happen slightly improves the situation, but there are likely to be hundreds or thousands of steps before the failure.  It would be much more convenient if the fuzzer automatically stored failing test sequences in a file, minimized the sequences to make debugging easy, and it made it possible to replay old failing tests in a regression suites.  Writing the code to support all this (boring but useful) infrastructure is no fun, especially in C/C++, and dramatically increases the amount of work required for your testing effort.  Handling the more subtle aspects, such as avoiding saving tests unless they fail to increase testing speed, but trapping assertion violations and hard crashes so that you write the test to the file system before terminating, is also hard to get right.

AFL and other general-purpose fuzzers usually provide this kind of functionality, which makes fuzzing a much more practical tool in debugging.  Unfortunately, such fuzzers are not convenient for testing APIs.  They typically generate a file or byte buffer, and expect that the program being testing will take that file as input.  Turning a series of bytes into a red-black tree test is probably easier and more fun than writing all the machinery for saving, replaying, and reducing tests, but it still seems like a lot of work that isn't directly relevant to your real task:  figuring out how to describe valid sequences of API calls, and how to check for correct behavior.  What you really want is a unit testing framework like [GoogleTest](https://github.com/abseil/googletest), but with support for using a fuzzer to generate input values.

## Enter DeepState

That is precisely what [DeepState](https://github.com/trailofbits/deepstate) is.  Well, actually, DeepState also lets you use symbolic execution to generate inputs, but we'll come back to that, later.

Translating John's fuzzer into a DeepState test is relatively easy.  [Here is a DeepState version of "the same fuzzer."](https://github.com/agroce/rb_tree_demo)  The primary changes for DeepState, all found in the file [`deepstate_harness.cpp`](https://github.com/agroce/rb_tree_demo/blob/master/deepstate_harness.cpp), are:

- Remove `main` and replace it with a named test (`TEST(RBTree, GeneralFuzzer)`)
   - A DeepState file can contain more than one named test, though it is fine to only have one test

- Rather than looping over creation of a red-black tree, just create one tree in each test
   - Instead of a fuzzing loop, our tests are closer to very generalized unit tests:  each test does one sequence of interesting API calls
   - DeepState will handle running multiple tests; the fuzzer or symbolic execution engine will provide the "outer loop"

- Replace various `rand() % NNN` calls with `DeepState_Int()`, `DeepState_Char()` and `DeepState_IntInRange(...)` calls
   - DeepState provides calls to generate most of the basic data types you want, optionally over restricted ranges
   - Using `rand()` in DeepState will create a nondeterministic test of little value; let DeepState make "random" choices for you

- Replace the `switch` statement choosing what API call to make with DeepState's `Oneof` construct
   - `OneOf` takes a list of C++ lambdas, and chooses one to execute 
   - This change is not strictly required, but using `OneOf` simplifies the code and allows optimization of choices and smart test reduction
   - Another version of `Oneof` takes a fixed-size array as input, and returns some value in it; e.g., `OneOf("abcd")` will produce a char, either `a`, `b`, `c`, or `d`

There are a number of other cosmetic (e.g. formatting, variable naming) changes, but the essence of the fuzzer is clearly preserved here.  With these changes, the fuzzer works almost as before, except that instead of running the `fuzz_rb` executable, we'll use DeepState to run the test we've defined and generate input values that choose which function calls to make, what values to insert in the red-black tree, and all the other decisions represented by `DeepState_Int`, `OneOf` and other calls:

```
int GetValue() {
  if (!restrictValues) {
    return DeepState_Int();
  } else {
    return DeepState_IntInRange(0, valueRange);
  }
}
```

```
  for (int n = 0; n < LENGTH; n++) {
    OneOf(
	  [&] {
	    int key = GetValue();
	    int* ip = (int*)malloc(sizeof(int));
	    *ip = key;
	    if (!noDuplicates || !containerFind(*ip)) {
	      void* vp = voidP();
	      LOG(INFO) << n << ": INSERT:" << *ip << " " << vp;
	      RBTreeInsert(tree, ip, vp);
	      containerInsert(*ip, vp);
	    } else {
	      LOG(INFO) << n << ": AVOIDING DUPLICATE INSERT:" << *ip;
	      free(ip);	      
	    }
	  },
	  [&] {
	    int key = GetValue();
	    LOG(INFO) << n << ": FIND:" << key;
	    if ((node = RBExactQuery(tree, &key))) {
	      ASSERT(containerFind(key)) << "Expected to find " << key;
	    } else {
	      ASSERT(!containerFind(key)) << "Expected not to find " << key;
	    }
	  },
```

### Installing DeepState

The [DeepState GitHub repository](https://github.com/trailofbits/deepstate) provides more details and dependencies, but on my MacBook Pro, installation is simple:

```shell
git clone https://github.com/trailofbits/deepstate
cd deepstate
mkdir build
cd build
cmake ..
sudo make install
```

Building a version with libFuzzer enabled is slightly more involved:

```shell
brew install llvm@6
git clone https://github.com/trailofbits/deepstate
cd deepstate
mkdir build
cd build
CC=/usr/local/opt/llvm\@6/bin/clang CXX=/usr/local/opt/llvm\@6/bin/clang++ BUILD_LIBFUZZER=TRUE cmake ..
sudo make install
```

AFL can also be used to generate inputs for DeepState, but most of the time, raw speed (due to not needing to fork), decomposition of compares, and value profiles seem to give libFuzzer an edge for this kind of API testing, in our (limited experimentally!) experience.  For more on using AFL and other file-based fuzzers with DeepState, see the [DeepState README](https://github.com/trailofbits/deepstate/blob/master/README.md).

### Using the DeepState Red-Black Tree Fuzzer

Once you have installed DeepState, building the red-black tree fuzzer(s) is also simple:

```shell
git clone https://github.com/agroce/rb_tree_demo
cd rb_tree_demo
make
```
The makefile compiles everything with all the sanitizers I could think of (address, undefined, and integer) in order to catch more bugs in fuzzing.  This has a performance penalty, but is usually worth it.

If you are on a Mac and using a non-Apple clang in order to get libFuzzer support, change `CC` and `CXX` in the [Makefile](https://github.com/agroce/rb_tree_demo/blob/master/Makefile) to point to the clang you are using (e.g. `/usr/local/opt/llvm\@6/bin/clang`).

This will give you a few different executables of interest.  One, `fuzz_rb` is simply [John's fuzzer](https://github.com/agroce/rb_tree_demo/blob/master/fuzz_red_black_tree.c), modified to use a 60 second timeout instead of a fixed number of "meta-iterations."  The `ds_rb` executable is the DeepState executable.  You can fuzz the red-black tree using a simple brute-force fuzzer (that behaves very much like John's original fuzzer):

```shell
mkdir tests
./ds_rb --fuzz --timeout 60 --log_level 3 --output_test_dir tests
```

If you leave out the `log_level` specification, you can see all the tests DeepState generates and runs.  The `tests` directory should be empty at the termination of fuzzing, since the red-black tree code in the repo (to my knowledge) has no bugs.  If you add `--fuzz_save_passing` to the options, you will end up with a large number of files for passing tests in the directory.

Finally, we can use libFuzzer to generate tests:

```shell
mkdir corpus
./ds_rb_lf corpus -use_value_profile=1 -detect_leaks=0 -max_total_time=60
```

This will run libFuzzer for 60 seconds, and place any interesting inputs (including test failures) in the corpus directory.  If there is a crash, it will leave a `crash-` file in the current directory.

We can replay any DeepState-generated tests (from libFuzzer or DeepState's fuzzer) easily:

```shell
./ds_rb --input_test_file <file>
```

Or replay an entire directory of tests:
```shell
./ds_rb --input_test_files_dir <dir>
```

Adding a `-abort_on_fail` flag when replaying an entire directory lets you stop the testing as soon as you hit a failing or crashing test.  This approach can easily be used to add failures found with DeepState (or interesting passing tests, or perhaps corpus tests from libFuzzer) to automatic regression tests for a project, including in CI.

### Adding a Bug

This is all fine, but it doesn't (or at least shouldn't) give us much confidence in John's fuzzer or in DeepState.  Even if we changed the Makefile to let us see code coverage, it would be easy to write a fuzzer that doesn't actually check for correct behavior -- it covers everything, but doesn't find any bugs other than crashes.  To see the fuzzers in action (and see more of what DeepState gives us), we can add a moderately subtle bug.  Go to line 267 of `red_black_tree.c` and change the 1 to a 0.  The diff of the new file and the original should look like:

```
267c267
< 	x->parent->parent->red=0;
---
> 	x->parent->parent->red=1;
```

Do a `make` to rebuild all the fuzzer with the new, broken `red_black_tree.c`.

Running the original fuzzer will fail almost immediately:

```
time ./fuzz_rb
Assertion failed: (left_black_cnt == right_black_cnt), function checkRepHelper, file red_black_tree.c, line 702.
Abort trap: 6

real	0m0.100s
user	0m0.008s
sys	0m0.070s
```

Using the DeepState fuzzer will produce results almost as quickly (we'll let it show us the testing by leaving off the `--log_level` option, but telling it to stop as soon as it finds a failing test):

```
time ./ds_rb --fuzz --abort_on_fail --output_test_dir tests
INFO: Starting fuzzing
INFO: No test specified, defaulting to last test defined
INFO: Running: RBTree_GeneralFuzzer from deepstate_harness.cpp(78)
INFO: deepstate_harness.cpp(122): 0: DELETE:-747598508
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(122): 1: DELETE:831257296
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(134): 2: PRED:1291220586
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(154): 4: SUCC:-1845067087
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(113): 6: FIND:-427918646
INFO: deepstate_harness.cpp(190): checkRep...
...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(103): 44: INSERT:-1835066397 0x00000000ffffff9c
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(154): 46: SUCC:-244966140
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(103): 48: INSERT:1679127713 0x00000000ffffffa4
INFO: deepstate_harness.cpp(190): checkRep...
Assertion failed: (left_black_cnt == right_black_cnt), function checkRepHelper, file red_black_tree.c, line 702.
ERROR: Crashed: RBTree_GeneralFuzzer
INFO: Saved test case to file `tests/6de8b2ffd42af6878875833c0cbfa9ea09617285.crash`
...
real	0m0.148s
user	0m0.011s
sys	0m0.131s
```

I've omitted much of the output above, since showing all 49 steps before the detection of the problem is a bit much, and the details of your output will certainly vary.  The big difference, besides the verbose output, from John's fuzzer, is the fact that DeepState _saved a test case_.  The name of your saved test case will, of course, be different, since the names are uniquely generated for each saved test.  To replay the test, I would do this:

```shell
./ds_rb --input_test_file tests/6de8b2ffd42af6878875833c0cbfa9ea09617285.crash
```

and I would get to see the whole disaster again, in gory detail.  As we said above, this isn't the most helpful output for seeing what's going on.  DeepState can help us here:

```
deepstate-reduce ./ds_rb tests/6de8b2ffd42af6878875833c0cbfa9ea09617285.crash minimized.crash 
ORIGINAL TEST HAS 8192 BYTES
LAST BYTE READ IS 509
SHRINKING TO IGNORE UNREAD BYTES
ONEOF REMOVAL REDUCED TEST TO 502 BYTES
ONEOF REMOVAL REDUCED TEST TO 494 BYTES
...
ONEOF REMOVAL REDUCED TEST TO 18 BYTES
ONEOF REMOVAL REDUCED TEST TO 2 BYTES
BYTE RANGE REMOVAL REDUCED TEST TO 1 BYTES
BYTE REDUCTION: BYTE 0 FROM 168 TO 0
NO (MORE) REDUCTIONS FOUND
PADDING TEST WITH 49 ZEROS

WRITING REDUCED TEST WITH 50 BYTES TO minimized.crash
```

Again, we omit some of the lengthy process of reducing the test.  The new test is easier to understand:

```
./ds_rb --input_test_file minimzed.crash
INFO: No test specified, defaulting to first test
INFO: Initialized test input buffer with data from `minimized.crash`
INFO: Running: RBTree_GeneralFuzzer from deepstate_harness.cpp(78)
INFO: deepstate_harness.cpp(103): 0: INSERT:0 0x0000000000000000
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(103): 1: INSERT:0 0x0000000000000000
INFO: deepstate_harness.cpp(190): checkRep...
INFO: deepstate_harness.cpp(192): RBTreeVerify...
INFO: deepstate_harness.cpp(103): 2: INSERT:0 0x0000000000000000
INFO: deepstate_harness.cpp(190): checkRep...
Assertion failed: (left_black_cnt == right_black_cnt), function checkRepHelper, file red_black_tree.c, line 702.
ERROR: Crashed: RBTree_GeneralFuzzer
```

We just need to insert three identical values into the tree to expose the problem.  Remember to fix your `red_black_tree.c` before proceeding!

## Mutation Testing

Introducing one bug by hand is fine, and we could try it again, but "the plural of anecdote is not data."  However, this is not strictly true.  If we have enough anecdotes, we can probably call it data (the field of "big multiple anecdotes" is due to take off any day now).  In software testing, creating multiple "fake bugs" has a name, _mutation testing_ (or _mutation analysis_).  Mutation testing works by automatically generating lots of small changes to a program, in the expectation that most such changes will make the program incorrect.  A test suite or fuzzer is better if it detects  more of these changes.  In the lingo of mutation testing, a detected mutant is "killed."  The phrasing is a bit harsh on mutants, but in testing a certain hard-heartedness towards bugs is perhaps in order.

There are many tools for mutation testing available, especially for Java.  The tools for C code are less robust, or more difficult to use, in general.  We recently released a tool, the [universalmutator](https://github.com/agroce/universalmutator), that uses regular expressions to allow mutation for many languages, including C and C++ (and Swift, Solidity, Rust, and numerous other languages previously without mutation testing tools).  We'll use the universalmutator to see how well our fuzzers do at detecting artificial red-black tree bugs.  Besides generality, one advantage of universalmutator is that it produces lots of mutants, including ones that are likely equivalent but can produce very subtle bugs, that are not supported in most mutation systems.  For high-stakes software, this can be worth the additional effort in analyzing and examining the mutants.

Installing universalmutator and generating some mutants is easy:

```shell
pip install universalmutator
mkdir mutants
mutate red_black_tree.c --mutantDir mutants
```

This will generate a large number of mutants, most of which won't compile (the universalmutator doesn't parse, or "know" C, so it's no surprise many of its mutants are not valid C).  We can discover the compiling mutants by running "mutation analysis" on the mutants, with "does it compile?" as our "test":

```shell
analyze_mutants red_black_tree.c "make clean; make" --mutantDir mutants
```

This will produce two files, `killed.txt` containing mutants that don't compile, and `notkilled.txt` containing the 1120 mutants that actually compile.  To see if a mutant is killed, the analysis tool just determines whether the command in quotes returns a non-zero exit code, or times out (the default timeout is 30 seconds; unless you have a very slow machine, this is plenty of time to compile our code here).

If we copy the `notkilled.txt` file containing valid (compiling) mutants to another file, we can then do some real mutation testing:

```shell
cp notkilled.txt compile.txt
analyze_mutants red_black_tree.c "make clean; make fuzz_rb; ./fuzz_rb" --mutantDir mutants --verbose --timeout 120--fromFile compile.txt
```

Output will look something like:

```
ANALYZING red_black_tree.c
COMMAND: ** ['make clean; make fuzz_rb; ./fuzz_rb'] **
#1: [0.0s 0.0% DONE]
  mutants/red_black_tree.mutant.2132.c NOT KILLED
  RUNNING SCORE: 0.0
...
Assertion failed: (left_black_cnt == right_black_cnt), function checkRepHelper, file red_black_tree.c, line 702.
/bin/sh: line 1: 30015 Abort trap: 6           ./fuzz_rb
#2: [62.23s 0.09% DONE]
  mutants/red_black_tree.mutant.1628.c KILLED IN 1.78541398048
  RUNNING SCORE: 0.5
...
```

Similar commands will run mutation testing on the DeepState fuzzer and libFuzzer.  Just change `make fuzz_rb; ./fuzz_rb` to `make ds_rb; ./ds_rb --fuzz --timeout 60 --log_level 3 --abort_on_fail` to use the built-in DeepState fuzzer.  For libFuzzer, to speed things up, we'll want to set the environment variable`LIBFUZZER_ABORT_ON_FAIL` to `TRUE`, and pipe output to `/dev/null` since libFuzzer's verbosity will hide our actual mutation results:

```shell
export LIBFUZZER_ABORT_ON_FAIL=TRUE
analyze_mutants red_black_tree.c "make clean; make ds_rb_lf; ./ds_rb_lf -use_value_profile=1 -detect_leaks=0 -max_total_time=60 >& /dev/null" --mutantDir mutants --verbose --timeout 120 --fromFile compile.txt
```

The tool generates 2,602 mutants, but only 1,120 of these actually compile.  Analyzing those mutants with a test budget of 60 seconds, we can get a better idea of the quality of our fuzzing efforts.  The DeepState brute-force fuzzer kills 797 of these mutants (71.16%).  John's original fuzzer kills 822 (73.39%).  Fuzzing the mutants not killed by these fuzzers another 60 seconds doesn't kill any additional mutants.  The performance of libFuzzer is strikingly similar:  60 seconds of libFuzzer (starting from an empty corpus) kills 797 mutants, exactly the same as the brute force fuzzer -- the same mutants, in fact.

### "There ain't no such thing as a free lunch" (or is there?)

DeepState's native fuzzer is, for a given amount of time, it appears, not as effective as John's "raw" fuzzer.  This shouldn't be a surprise: in fuzzing, speed is king.  Because DeepState is parsing a bytestream, forking in order to save crashes, and producing extensive, user-controlled logging (among other things), it is impossible for it to generate and execute tests as quickly as John's bare-bones fuzzer.

libFuzzer is even slower; in addition to all the services (except forking for crashes, which is handled by libFuzzer itself) provided by the DeepState fuzzer, libFuzzer is determining the code coverage and computing value profiles for every test, and performing computations needed to base future testing on those evaluations of input quality.

Is this why John's fuzzer kills 25 mutants that DeepState does not?  Well, not quite.  If we examine the 25 additional mutants, we discover that every one involves changing an equality comparison on a pointer into an inequality.  E.g.:

```
<   if ( (y == tree->root) ||
---
>   if ( (y <= tree->root) ||
```

The DeepState fuzzer is not finding these because it runs each test in a fork, and so the address space doesn't allocate enough times to cause a problem for these particular checks!  Now, in theory, this shouldn't be the case for libFuzzer, which runs without forking.  And, sure enough, if we give the slow-and-steady tortoise libFuzzer 5 minutes instead of 60 seconds, it catches all of these mutants, too.  No amount of additional fuzzing will help the DeepState fuzzer.  In this case, the bug is strange enough and unlikely enough we can perhaps ignore it.  The issue is not the speed of our fuzzer, or the quality (exactly), but the fact that different fuzzing environments create subtle differences in _what tests we are actually running_.

Now that we've seen this problem, we'll add an option to DeepState to make the brute force fuzzer run in a [non-forking mode](https://github.com/trailofbits/deepstate/issues/90) (by the time you read this, the option may already exist).  Unfortunately, this is not a great solution overall:  while libFuzzer "detects" these bugs, it can't produce a good saved test case for them, since the failure depends on all the mallocs that have been issued, and the exact addresses of certain pointers.  Perhaps this bug is not really worth our trouble, especially since libFuzzer can detect it.  I think we can say that, for most intents and purposes, DeepState is as powerful as John's "raw" fuzzer, as easy to implement, and considerably more convenient for debugging and regression testing.

### Examining the Mutants

This takes care of the differences in our fuzzers' performances.  But how about the remaining mutants?  None of them are killed by 5 minutes of fuzzing using any of our fuzzers.  Do they show holes in our testing?  There are various ways to detect _equivalent_ mutants (mutants that don't actually change the program semantics, and so can't possibly be killed), including comparing the binaries generated by an optimizing compiler.  For our purposes, we will just examine a random sample of the 298 unkilled mutants, to confirm that at least most of the unkilled mutants are genuinely uninteresting.

- The first mutant changes an `<=` in a comment.  There's no way we can kill this, and comparing compiled binaries would have proven it.

- The second mutant modifies code in the `InorderTreePrint` function, which John's fuzzer (and thus ours) explicitly chooses not to test.  This would not be detectable by comparing binaries, but it is common sense.  If our fuzzer never covers a piece of code, intentionally, it cannot very well detect bugs in that code.

- The third mutant changes the assignment to `temp->key` on line 44, in the `RBTreeCreate` function, so it assigns a 1 rather than a 0.  This is more interesting.  It will take some thought to convince ourselves this does not matter.  If we follow the code's advice and look at the comments on `root` and `nil` in the header file, we can see these are used as sentinels.  Perhaps the exact data values in `root` and `nil` don't matter, since we'll only detect them by pointer comparisons?  Sure enough, this is the case.

- The fourth mutant removes the assignment `newTree->PrintKey= PrintFunc;` on line 35; again, since we never print trees, this can't be detected.

- The fifth mutant is inside a comment.

- The sixth mutant changes a pointer comparison in an assert.
```
   686c686
<     assert (node->right->parent == node);
---
>     assert (node->right->parent >= node);
```
If we assume the assert always held for the original code, then changing `==` to the more permissive `>=` obviously cannot fail.

- The seventh mutant is in a comment.

- The eighth mutant removes an assert.  Again, removing an assert can never cause previously passing tests to fail, unless something is wrong with your assert!

- The ninth mutant changes a `red` assignment:
```
243c243
<       x->parent->parent->red=1;
---
>       x->parent->parent->red=-1;
```
Since we don't check the exact value of the `red` field, but use it to branch (so all non-zero values are the same) this is fine.

- The tenth mutant is again inside the `InorderTreePrint` function.

If we were really testing this red-black tree as a critical piece of code, we would probably at this point:

- Definitely make a tool (like a 10-line Python script, not anything heavyweight!) to throw out all mutants inside comments, inside the `InorderTreePrint` function, or that remove an assertion.

- We might compile all the mutants and compare binaries with each other and the original file, to throw out obvious equivalent mutants and redundant mutants.  This step can be a little annoying because compilers don't always produce equivalent binaries, due to timestamps generated at compile time, which is why I skipped over it in the discussion above.

- Examine the remaining mutants (maybe 200 or so) carefully, to make sure we're not missing anything.  Finding categories of "that's fine" mutants often makes this process much easier than it sounds off hand (things like "assertion removals are always ok").

The process of (1) making a test generator then (2) applying mutation testing and (3) actually looking at the surviving mutants and using them to improve our testing can be thought of as a [_falsification-driven testing_ process](https://agroce.github.io/asej18.pdf).  For highly-critical, small pieces of code, this can be a very effective way to build an effective fuzzing regimen.  It helped Paul E. McKenney [discover real bugs in the Linux kernel's RCU module](https://agroce.github.io/mutation17.pdf).

## Just Fuzz it More

Alternatively, before turning to mutant investigation, you can just fuzz the code more aggressively.  Our mutant sample suggests there won't be _many_ outstanding bugs, but perhaps there are a few.  Five minutes is not that extreme a fuzzing regimen; people expect to run AFL for days.  If we were really testing the red-black tree as a critical piece of code, we probably wouldn't give up after five minutes.  Which fuzzer would be best for this?  It's hard to know for sure, but one reasonable approach would be to first use libFuzzer to generate a large corpus of tests to seed fuzzing, that achieve high coverage on the un-mutated red-black tree.  Then, we can run a longer fuzzing run on each mutant, using the seeds to make sure we're not spending most of the time just "learning" the red-black tree API.  So, after generating a corpus on the original code for an hour, we run libFuzzer, starting from that corpus, for ten minutes.  How many additional mutants does this kill?  We can already guess it will be fewer than 30, based on our 3% sample.  A simple script, as described above, brings the number of mutants to analyze down to 174 by removing comment mutations, print function mutations, and assertion removals.

## What About Symbolic Execution?

**[Note:  this part doesn't work on Mac systems right now, unless you know enough to do a cross compile, and can get the binary analysis tools working with that.  I ran it on Linux inside docker.]**

DeepState also supports symbolic execution, which, according to [some definitions](https://arxiv.org/pdf/1812.00140.pdf), is just another kind of fuzzing (white box fuzzing).  Unfortunately, at this time, neither angr nor manticore (the two binary analysis engines we support) can scale to the full red-black tree or file system examples with a search depth anything like 100; this isn't really surprising, given the tools are trying to generate all possible paths through the code!  However, simply lowering the depth to a more reasonable number is also insufficient.  You're likely to get solver timeout errors even at depth 3.   Instead, we use [`symex.cpp`](https://github.com/agroce/rb_tree_demo/blob/master/symex.cpp), which does a much simpler insert/delete pattern, with comparisons to the reference, 3 times in a row.

```shell
clang -c red_black_tree.c container.c stack.c misc.c
clang++ -o symex symex.cpp -ldeepstate red_black_tree.o stack.o misc.o container.o -static -Wl,--allow-multiple-definition,--no-export-dynamic
deepstate-manticore ./symex
```

The result will be tests covering all paths through the code, in the `out` directory.  This may take quite some time to run, since each path can take a minute or two to generate, and there are quite a few paths.  If `deepstate-manticore` is too slow, try `deepstate-angr` (or vice versa), since different code is best suited for different symbolic execution engines (this is one of the purposes of DeepState -- to make shopping around for a good back-end easy).

```
INFO:deepstate.mcore:Running 1 tests across 1 workers
INFO:deepstate:Running RBTree_TinySymex from symex.cpp(42)
INFO:deepstate:symex.cpp(56): INSERT:0 0x0000000000000000
INFO:deepstate:symex.cpp(61): DELETE:0
INFO:deepstate:symex.cpp(74): INSERT:0 0x0000000000000000
INFO:deepstate:symex.cpp(79): DELETE:0
INFO:deepstate:symex.cpp(92): INSERT:0 0x0000000000000000
INFO:deepstate:symex.cpp(97): DELETE:-2147483648
INFO:deepstate:Passed: RBTree_TinySymex
INFO:deepstate:Input: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ...
INFO:deepstate:Saving input to out/symex.cpp/RBTree_TinySymex/89b9a0aba0287935fa5055d8cb402b37.pass
INFO:deepstate:Running RBTree_TinySymex from symex.cpp(42)
INFO:deepstate:symex.cpp(56): INSERT:0 0x0000000000000000
INFO:deepstate:symex.cpp(61): DELETE:0
INFO:deepstate:symex.cpp(74): INSERT:0 0x0000000000000000
INFO:deepstate:symex.cpp(79): DELETE:0
INFO:deepstate:symex.cpp(92): INSERT:0 0x0000000000000000
INFO:deepstate:symex.cpp(97): DELETE:0
INFO:deepstate:Passed: RBTree_TinySymex
...
```

We can see how well the 583 generated tests perform using the same mutation analysis as before:

```shell
analyze_mutants red_black_tree.c "clang -c red_black_tree.c; clang++ -o symex symex.cpp -ldeepstate red_black_tree.o stack.o misc.o container.o; ./symex --input_test_dir out --abort_on_fail --log_level 2" --verbose --fromFile compile.txt --timeout 40 --mutantDir mutants
```

The results are not great.  The tests kill 264 mutants (23.57%).  They can be somewhat improved by adding back in the `checkRep` and `RBTreeVerify` checks that were removed in order to speed symbolic execution.  Adding these checks after the final insert/delete pair kills an additional 165 mutants, bringing the kill rate up to 38.3%.  While not impressive compared to the fuzzers, there is a key point.  Six of these mutants are ones _not_ killed by any of the fuzzers, even the well-seeded ten minute libFuzzer runs:

```
703c703
<   return left_black_cnt + (node->red ? 0 : 1);
---
>   return left_black_cnt % (node->red ? 0 : 1);
```

```
701c701
<   right_black_cnt = checkRepHelper (node->right, t);
---
>   /*right_black_cnt = checkRepHelper (node->right, t);*/
```

```
700c700
<   left_black_cnt = checkRepHelper (node->left, t);
---
>   /*left_black_cnt = checkRepHelper (node->left, t);*/
```

```
703c703
<   return left_black_cnt + (node->red ? 0 : 1);
---
>   return left_black_cnt / (node->red ? 0 : 1);
```

```
703c703
<   return left_black_cnt + (node->red ? 0 : 1);
---
>   /*return left_black_cnt + (node->red ? 0 : 1);*/
```

```
236c236
<   x->red=1;
---
>   /*x->red=1;*/

```

Five of these bugs are in the `checkRep` code, so may not be considered as critical (they won't cause bad behavior, just false positives in testing), but the last one is at the very start of `RBTreeInsert`.
While generating tests using symbolic execution takes a while, producing tests like these occasionally and using them in regression can be extremely valuable.  The 583 tests take less than 20 seconds to run, and detect bugs that even aggressive fuzzing may miss.  Learning to use DeepState makes mixing fuzzing and symbolic execution in your testing easy; even if you need a new harness for symbolic execution work, it looks like, and can share code with, most of your fuzzing-based testing.  A major long-term goal for DeepState is to increase the scalability of symbolic execution for API sequence testing, using high-level strategies not dependent on the underlying engine, so you can use the same harness more often.

See the [DeepState  repo](https://github.com/trailofbits/deepstate) for more information on how to use symbolic execution.

## What About Code Coverage?

We didn't even look at code coverage in our fuzzing.  The reason is simple:  if we're willing to go to the effort of applying mutation testing, and examining all surviving mutants, there's not much additional benefit in looking at code coverage.  Under the hood, libFuzzer and the symbolic execution engines aim to maximize coverage, but for our purposes mutants work even better.  After all, if we don't cover mutated code, we can hardly kill it.  Coverage can be very useful, of course, in early stages of fuzzer harness development, where mutation testing is expensive, and you really just want to know if you are even hitting most of the code.  But for intensive testing, when you have the time to do it, mutation testing is much more thorough (you have to not only cover code, but actually test what it does).  [In fact, at present, most scientific evidence for the usefulness of code coverage relies on the greater usefulness of mutation testing.](https://agroce.github.io/onwardessays14.pdf)

## Further Reading 

For a more involved example using DeepState to test an API, see the [TestFs](https://github.com/agroce/testfs) example, which tests a user-level, ext3-like file system.  For more details on DeepState in general, see our [NDSS 2018 Binary Analysis Research Workshop paper](https://agroce.github.io/bar18.pdf). 

