rb_tree_demo adapted for DeepState
============

This was originally code accompanying a blog post about fuzzing a red-black tree implementation:

http://blog.regehr.org/archives/896

John Regehr posted it on GitHub:

https://github.com/regehr/rb_tree_demo

The original code is still there, but this adds a new file,
`deepstate_harness.cpp`, that uses [DeepState](https://github.com/trailofbits/deepstate) instead of a C random
number generator, to perform (I think) the same testing as John's
harness.

The easiest way to use this is with the [DeepState docker](https://github.com/trailofbits/deepstate#DOCKER).

The Makefile will build three DeepState executables, `ds_rb`,
`ds_rb_lf`, `ds_rb_afl`, the first of which is for symbolic execution, test replay,
Eclipser fuzzing, the second of which is for libFuzzer fuzzing, and
the third of which is for AFL fuzzing.

To fuzz this, you will want to do something like:

```shell
$ ./ds_rb_lf corpus -use_value_profile=1 -detect_leaks=0
```

The leak detection disabling is because when the test terminates early due to violated assumes in ranges, etc., this will leak memory.

Much more information is available in
[a pair of blog posts](https://blog.trailofbits.com/2019/01/22/fuzzing-an-api-with-deepstate-part-1/).
Some of the instructions on how to run things may have changed a bit
since then, but it should be easy to figure out how.


