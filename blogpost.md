# Fuzzing an ADT with DeepState

In 2013, John Regehr wrote a blog post on ["How to Fuzz an ADT Implementation"](https://blog.regehr.org/archives/896).  John talked at some length about general issues in gaining confidence that a data-type implementation is reliable, discussing code coverage, test oracles, and differential testing.  The article is well worth reading, and gives a good overview of how to construct a simple custom fuzzer for an ADT, or, for that matter, any fairly self-contained API where there are good ways to check for correctness.

The general problem is simple.  Suppose we have a piece of software that is, essentially, a library: a red-black tree, an AVL tree, a file-system, an [in-memory store](https://github.com/redis/hiredis), or even a [crypto library](https://botan.randombit.net/).  The software provides a set of functions (or methods on objects), and we have some expectations about what will happen when we call those functions.  Traditional unit testing addresses this problem by having the developer (or us) write a series of small functions that look like:

```
result1 = foo(3, "hello");
result2 = bar(result1, "goodbye")
assert(result2 == DONE);
```

That is, "do something, then check that it did the right thing."  

John's post doesn't just give general advice, it includes links to a [working fuzzer for a red-black tree](https://github.com/regehr/rb_tree_demo).  The fuzzer is effective and a good example of how to really hammer an API.
