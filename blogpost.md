# Fuzzing an API with DeepState

In 2013, John Regehr wrote a blog post on ["How to Fuzz an ADT Implementation."](https://blog.regehr.org/archives/896)  John talked at some length about general issues in gaining confidence that a data-type implementation is reliable, discussing code coverage, test oracles, and differential testing.  The article is well worth reading (it would be a good idea to read it before reading the rest of this article, in fact), and it gives a good overview of how to construct a simple custom fuzzer for an ADT, or, for that matter, any fairly self-contained API where there are good ways to check for correctness.

The general problem is simple.  Suppose we have a piece of software that is, essentially, a library: a red-black tree, an AVL tree, a [file-system](https://github.com/agroce/testfs), an [in-memory store](https://github.com/redis/hiredis), or even a [crypto library](https://botan.randombit.net/).  The software provides a set of functions (or methods on objects), and we have some expectations about what will happen when we call those functions.  Traditional unit testing addresses this problem by having the developer (or us) write a series of small functions that look like:

```
result1 = foo(3, "hello");
result2 = bar(result1, "goodbye")
assert(result2 == DONE);
```

That is, the tests each are of the form: "do something, then check that it did the right thing."   This approach has two problems.  First, it's a lot of work.  Second, the return of investment on that work is not as good as you would hope; each test does one specific thing, and if the author of the tests doesn't happen to think of a potential problem, the tests are very unlikely to catch that problem.  These unit tests are insufficient for the same reasons that AFL and other fuzzers have been so successful at finding security vulnerabilities in widely-used programs:  humans are too slow to write many tests, and are limited in their ability to imagine insane, harmful inputs.  The randomness of fuzzing makes it possible to produce many tests very quickly and results in tests that go far outside the "expected uses."  Fuzzing is often thought of as generating files or packets, but it can also generate sequences of API calls to test software libraries.  Such fuzzing is often referred to as [random](https://www.cs.tufts.edu/~nr/cs257/archive/john-hughes/quick.pdf) or [randomized](https://agroce.github.io/icse07.pdf) testing, but _fuzzing is fuzzing_.  Instead of a series of unit tests doing one specific thing, a fuzzer test (also known as a [property-based test](https://fsharpforfunandprofit.com/posts/property-based-testing/) or a [parameterized unit test](https://www.microsoft.com/en-us/research/publication/parameterized-unit-tests/)) looks more like:

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

Translating John's fuzzer into DeepState is relatively easy.  [Here is a DeepState version of "the same fuzzer."](https://github.com/agroce/rb_tree_demo)  The primary changes for DeepState are:

- Remove `main` and replace it with a named test (`TEST(RBTree, GeneralFuzzer)`)

- Rather than looping over creation of a red-black tree, just create one tree in each test
   - DeepState will handle running multiple tests

- Replace various `rand() % NNN` calls with `DeepState_Int()` and `DeepState_IntInRange(...)` calls

- Replace the `switch` statement choosing what API call to make with DeepState's `Oneof` construct

There are a number of other cosmetic (e.g. formatting, variable naming) changes, but the essence of the fuzzer is clearly preserved here.

### Installing DeepState

### Using the DeepState Red-Black Tree Fuzzer

## Mutation Testing

The tool generates 2,602 mutants, but only 1,120 of these actually compile.  Analyzing those mutants with a test budget of 60 seconds, we can get a better idea of the quality of our fuzzing efforts.  The DeepState brute-force fuzzer kills 797 of these mutants (71.16%).  John's original fuzzer kills 822 (73.39%).  Fuzzing the mutants not killed by these fuzzers another 60 seconds doesn't kill any additional mutants.  The performance of libFuzzer is strikingly similar:  60 seconds of libFuzzer (starting from an empty corpus) kills 797 mutants, exactly the same as the brute force fuzzer -- the same mutants, in fact.

### "There ain't no such thing as a free lunch" -- in the short run

DeepState's native fuzzer is, for a given amount of time, not as effective as John's "raw" fuzzer.  This shouldn't be a surprise: in fuzzing, speed is king.  Because DeepState is parsing a bytestream, forking in order to save crashes, and producing extensive, user-controlled logging (among other things), it is impossible for it to generate and execute tests as quickly as John's bare-bones fuzzer.

libFuzzer is even slower; in addition to all the services (except forking for crashes, which is handled by libFuzzer itself) provided by the DeepState fuzzer, libFuzzer is determining the code coverage and computing value profiles for every test, and performing computations needed to base future testing on those evaluations of input quality.
