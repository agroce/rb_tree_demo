rb_tree_demo adapted for DeepState
============

This was originally code accompanying a blog post about fuzzing a red-black tree implementation:

http://blog.regehr.org/archives/896

John Regehr posted it on GitHub:

https://github.com/regehr/rb_tree_demo

The original code is still there, but this adds a new file,
`deepstate_harness.cpp`, that uses [DeepState](https://github.com/trailofbits/deepstate) instead of a C random
number generator, to perform (I think) the same testing as John's
harness.  The Makefile will build two executables, `ds_rb` and
`ds_rb_lf`, the first of which is for symbolic execution, test replay,
AFL fuzzing, etc., and the second of which is for libFuzzer fuzzing.

To fuzz this, you will want to do something like:

```shell
$ ./ds_rb_lf corpus -use_value_profile=1 -detect_leaks=0
```

The leak detection disabling is because when the test terminates early due to violated assumes in ranges, etc., this will leak memory.

To try mutation testing on this, you can install the [universalmutator](https://github.com/agroce/universalmutator) and mutate the red black tree implementation:

```shell
$ pip install universalmutator
$ mutate red_black_tree.c
```

Then you can analyze a set of tests generated like this:

```shell
$ analyze_mutants red_black_tree.c "python findfailures.py corpus"
```
