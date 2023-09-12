// Copyright (c) 2024 Pyarelal Knowles, MIT License

#include <bitset>
#include <gtest/gtest.h>
#include <nodecode/index_ptr.hpp>
#include <iostream>
#include <iterator>
#include <ranges>
#include <stdexcept>

using namespace nodecode;

struct ArrayHeader {
  const char base[11] = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
};

struct StringHeader {
  std::string base = "hello world";
};

// verify iterators
template <class iterator>
concept input_and_output_iterator =
    std::input_iterator<iterator> &&
    std::output_iterator<iterator,
                         typename std::iterator_traits<iterator>::value_type>;
static_assert(std::ranges::random_access_range<index_span<&ArrayHeader::base>>);
static_assert(std::ranges::sized_range<index_span<&ArrayHeader::base>>);
static_assert(std::input_iterator<
              typename index_span<&ArrayHeader::base, uint32_t>::iterator>);
static_assert(input_and_output_iterator<
              typename index_span<&StringHeader::base>::iterator>);

namespace {
// Can't bind to a wildly different type
static_assert(!std::is_invocable_v<
              decltype(&index_ptr<&ArrayHeader::base>::bind), StringHeader>);

// Can't bind to another class with the same type member
struct ArrayHeader2 {
  const char base[11] = {};
};
static_assert(!std::is_invocable_v<
              decltype(&index_ptr<&ArrayHeader::base>::bind), ArrayHeader2>);
} // namespace

TEST(IndexPtr, Default) {
  index_ptr<&ArrayHeader::base> ptr;
  EXPECT_EQ(ptr, 0);
}

TEST(IndexPtr, Integer) {
  index_ptr<&ArrayHeader::base> ptr(1);
  EXPECT_EQ(ptr, 1);
}

TEST(IndexPtr, Replace) {
  index_ptr<&ArrayHeader::base> ptr(1);
  ptr = 42;
  EXPECT_EQ(ptr, 42);
}

TEST(IndexPtr, Add) {
  index_ptr<&ArrayHeader::base> ptr(1);
  ptr += 41;
  EXPECT_EQ(ptr, 42);
}

TEST(IndexPtr, Difference) {
  index_ptr<&ArrayHeader::base> ptr1(1);
  index_ptr<&ArrayHeader::base> ptr2(43);
  EXPECT_EQ(ptr2 - ptr1, 42);
}

TEST(IndexPtr, Throw) {
  index_ptr<&ArrayHeader::base> ptr;
  EXPECT_THROW({ (void)*ptr; }, std::runtime_error);
}

TEST(IndexPtr, ReadManualBoundArray) {
  index_ptr<&ArrayHeader::base> ptr;
  ArrayHeader                   header;
  EXPECT_EQ(*ptr.bind(header), 'h');
}

TEST(IndexPtr, ReadManualBoundString) {
  index_ptr<&StringHeader::base> ptr;
  StringHeader                   header;
  EXPECT_EQ(*ptr.bind(header), 'h');
}

TEST(IndexPtr, WriteManualBoundString) {
  index_ptr<&StringHeader::base> ptr;
  StringHeader                   header;
  *ptr.bind(header) = 'x';
  ptr = 1;
  *ptr.bind(header) = 'z';
  EXPECT_EQ(header.base[0], 'x');
  EXPECT_EQ(header.base[1], 'z');
}

TEST(BoundHeader, Unbound) {
  EXPECT_THROW({ bound_header<ArrayHeader>::get(); }, std::runtime_error);
}

TEST(BoundHeader, One) {
  ArrayHeader header;
  EXPECT_THROW({ bound_header<ArrayHeader>::get(); }, std::runtime_error);
  {
    bound_header bound(header);
    EXPECT_EQ(bound_header<ArrayHeader>::get(), &header);
  }
  EXPECT_THROW({ bound_header<ArrayHeader>::get(); }, std::runtime_error);
}

TEST(BoundHeader, Stack) {
  {
    ArrayHeader  header1;
    bound_header bound(header1);
    EXPECT_EQ(bound_header<ArrayHeader>::get(), &header1);
    {
      ArrayHeader  header2;
      bound_header bound(header2);
      EXPECT_EQ(bound_header<ArrayHeader>::get(), &header2);
    }
    EXPECT_EQ(bound_header<ArrayHeader>::get(), &header1);
  }
  EXPECT_THROW({ bound_header<ArrayHeader>::get(); }, std::runtime_error);
}

struct CircleOfFifths {
  struct Key;

  std::span<Key> keys;

  struct Key {
    index_ptr<&CircleOfFifths::keys> next;
  };

  std::vector<Key> keys_ = {{7}, {8}, {9}, {10},  {11}, {0},
                            {1}, {2}, {3}, {4}, {5},  {6}};

  CircleOfFifths() { keys = keys_; }
};

TEST(IndexPtr, SelfReference) {
  static const std::map<uint32_t, std::string> noteToKey{
      { 0,  "A"},
      { 1, "Bb"},
      { 2,  "B"},
      { 3,  "C"},
      { 4, "C#"},
      { 5,  "D"},
      { 6, "D#"},
      { 7,  "E"},
      { 8,  "F"},
      { 9, "F#"},
      {10,  "G"},
      {11, "G#"}
  };
  std::vector<std::string> result = {"A",  "E",  "B", "F#", "C#", "G#",
                                     "D#", "Bb", "F", "C",  "G",  "D"};
  CircleOfFifths circleOfFifths;
  bound_header bound(circleOfFifths);
  auto key = circleOfFifths.keys[5];
  for(int i = 0; i < 12; ++i)
  {
    EXPECT_EQ(noteToKey.at(key.next), result[i]);
    key = *key.next;
  }
}

TEST(IndexPtr, StackBind) {
  index_ptr<&ArrayHeader::base> ptr;
  ArrayHeader                   header;
  {
    bound_header bound(header);
    EXPECT_EQ(bound.get()->base, header.base);
    EXPECT_EQ(*ptr.bind(*bound_header<ArrayHeader>::get()), 'h');
    EXPECT_EQ(*ptr.get(), 'h');
    EXPECT_EQ(ptr.get(), std::begin(header.base));
    EXPECT_EQ(*ptr, 'h');
    EXPECT_EQ(ptr[1], 'e');
  }
  EXPECT_THROW({ (void)*ptr; }, std::runtime_error);
}

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

TEST(Span, ConstructDefault) {
  index_span<&ArrayHeader::base> hello;
  EXPECT_EQ(hello.index(), 0);
  EXPECT_EQ(hello.size(), 0);
}

TEST(Span, ConstructIndexSize) {
  index_span<&ArrayHeader::base> hello(0, 5);
  EXPECT_EQ(hello.index(), 0);
  EXPECT_EQ(hello.size(), 5);
}

TEST(Span, ConstructPointers) {
  ArrayHeader  header;
  bound_header bound(header);
  auto world = index_span<&ArrayHeader::base>::from_pointer(&header.base[6], 5);
  EXPECT_EQ(world.index(), 6);
  EXPECT_EQ(world.size(), 5);
  auto world2 = index_span<&ArrayHeader::base>::from_pointer(
      &header.base[6], &header.base[6 + 5]);
  EXPECT_EQ(world2.index(), 6);
  EXPECT_EQ(world2.size(), 5);
}

TEST(Span, ConstructIterators) {
  StringHeader header;
  bound_header bound(header);
  auto         world =
      index_span<&StringHeader::base>::from_iterator(header.base.begin() + 6, 5);
  EXPECT_EQ(world.index(), 6);
  EXPECT_EQ(world.size(), 5);
  auto world2 = index_span<&StringHeader::base>::from_iterator(
      header.base.begin() + 6, header.base.begin() + 6 + 5);
  EXPECT_EQ(world2.index(), 6);
  EXPECT_EQ(world2.size(), 5);
}

TEST(Span, ConstructRange) {
  StringHeader header;
  bound_header bound(header);
  auto         world =
      index_span<&StringHeader::base>::from_range(std::span(header.base).subspan(6, 5));
  EXPECT_EQ(world.index(), 6);
  EXPECT_EQ(world.size(), 5);
}

TEST(Span, Iterate) {
  index_span<&ArrayHeader::base> hello(0, 5);
  ArrayHeader                    header;
  bound_header                   bound(header);
  EXPECT_EQ(hello, std::string("hello"));
}

TEST(Span, InlineObjects) {
  struct Header {
    std::string               base  = "hello world";
    index_span<&Header::base> hello = {0, 5};
    index_span<&Header::base> world = {6, 5};
  } header;
  bound_header bound(header);
  std::string  hello(std::begin(header.hello), std::end(header.hello));
  std::string  world(std::begin(header.world), std::end(header.world));
  EXPECT_EQ(hello, "hello");
  EXPECT_EQ(world, "world");
}

TEST(Span, InlineArray) {
  struct Header {
    std::string               base     = "hello world";
    index_span<&Header::base> words[2] = {
        {0, 5},
        {6, 5}
    };
  } header;
  bound_header bound(header);
  EXPECT_EQ(header.words[0], std::string("hello"));
  EXPECT_EQ(header.words[1], std::string("world"));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
