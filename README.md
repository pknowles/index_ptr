# pknowles index_ptr

A pointer to an item in an array will become dangling if you move the array.
However, an index would still work, because it's relative to the start of the
array. `index_ptr<&Header::array, uint32_t>` is an array index into
`Header::array` that behaves like a pointer. An advantage is you can't
accidentally index the wrong array. Another is marginaly shorter code than real
indices to traverse complex data structures.

> [!CAUTION]
> `index_ptr` is more of an exploration of an idea than a properly useful
> object. You probably want
> [offset_pointer](https://github.com/pknowles/offset_ptr) instead.

>I was so preoccupied with whether or not I could, I didn't stop to think if I
>should.

Use cases:
- Zero overhead when loading a structure-of-arrays memory mapped file
- Sharing data between address spaces, e.g. shared memory or CUDA
- Compatible with limited languages such as GLSL

```
Header* header = (Header*)mmap("file"); // pseudocode

// no deserialization work, simply start accessing relational data
header->foos[3]->bar->baz...;
```

Limitations:

- No nice way to implicitly know the target array instance
- I haven't figured out const
- Really just syntactical salt

**Example 1**

```
TEST(IndexPtr, Readme) {
  struct Header {
    std::string letters = "Hello World!";
    nodecode::index_ptr<&Header::letters> important;
  };

  Header header;

  // Use this instance for future indexing
  nodecode::bound_header bound(header);

  // "Dereference" to get the object pointed to
  EXPECT_EQ(*header.important, 'H');

  // Still acts like a regular integer index
  header.important = 6;
  EXPECT_EQ(*header.important, 'W');

  // The scoped nodecode::bound_header is a shortcut and entirely optional
  header.important += 5;
  EXPECT_EQ(*header.important.bind(header), '!');
}
```

**Example 2**

```
TEST(IndexPtr, Chain) {
  struct Foo;
  struct Bar;

  struct Header {
    std::span<Foo> foos;
    std::span<Bar> bars;
  };

  // Foo has an index_ptr to a Bar
  struct Foo {
    std::string              data;
    index_ptr<&Header::bars> bar;
  };

  // Bar has an index_ptr to a Foo
  struct Bar {
    std::string              data;
    index_ptr<&Header::foos> foo;
  };

  // Make some concrete data for a Header
  std::vector<Foo> foos{
      {"foo0", 0},
      {"foo1", 1}
  };
  std::vector<Bar> bars{
      {"bar0", 1},
      {"bar1", 0}
  };
  Header       header{foos, bars};

  // Bind it and start traversing the data structure
  bound_header bound(header);
  EXPECT_EQ(header.foos[0].data, "foo0");
  EXPECT_EQ(header.foos[0].bar->data, "bar0");
  EXPECT_EQ(header.foos[0].bar->foo->data, "foo1");
  EXPECT_EQ(header.foos[0].bar->foo->bar->data, "bar1");
  EXPECT_EQ(header.foos[0].bar->foo->bar->foo->data, "foo0");
}
```

## Contributing

Issues and pull requests are most welcome, thank you! Note the
[DCO](CONTRIBUTING) and MIT [LICENSE](LICENSE).
