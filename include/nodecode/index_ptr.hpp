// Copyright (c) 2024 Pyarelal Knowles, MIT License

#pragma once

#include <cinttypes>
#include <iterator>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace nodecode {

template <class Header>
class bound_header {
public:
  bound_header(Header& header) {
    stack().push_back(s_current);
    s_current = &header;
  }
  ~bound_header() {
    s_current = stack().back();
    stack().pop_back();
  }
  static Header* get() {
    if (!s_current)
      throw std::runtime_error("No header bound");
    return s_current;
  }

private:
  static thread_local Header* s_current;
  static auto&                stack() {
    thread_local static std::vector<Header*> s_headers;
    return s_headers;
  }
};

// DANGER: may break if bound_header() were used across translation units
template <class Header>
thread_local Header* bound_header<Header>::s_current = nullptr;

template <class T>
struct member_traits_helper;

template <class C, class T>
struct member_traits_helper<T C::*> {
  using value_type = T;
  using class_type = C;
};

template <class T>
struct member_traits
    : member_traits_helper<typename std::remove_cvref<T>::type> {};

template <class T> using member_type_t = member_traits<T>::value_type;

template <class T> using member_class_t = member_traits<T>::class_type;

template <auto ObjectsPtr, class IndexType = uint32_t, bool ConstHeader = false>
#if 0
  // cannot validate range concept as end() will likely require pointer
  // arithmetic and thus a complete type
  requires std::ranges::random_access_range<member_type_t<decltype(ObjectsPtr)>>
#endif
class index_ptr {
public:
  using range_type          = member_type_t<decltype(ObjectsPtr)>;
  using mutable_header_type = member_class_t<decltype(ObjectsPtr)>;
  using header_type = std::conditional_t<ConstHeader, const mutable_header_type,
                                         mutable_header_type>;
  using iterator = std::ranges::iterator_t<range_type>;
  #if 0
  // cannot validate range concept as end() will likely require pointer
  // arithmetic and thus a complete type
  using value_type =
      std::remove_reference_t<std::ranges::range_reference_t<range_type>>;
  #else
  using value_type = std::remove_reference_t<std::iter_reference_t<iterator>>;
  #endif
  using index_type = IndexType;
  index_ptr()      = default;
  index_ptr(const index_type& index) : m_index(index) {}
  iterator bind(header_type& header) const {
    return std::ranges::begin(header.*ObjectsPtr) + m_index;
  }
  iterator get() const { return bind(*bound_header<header_type>::get()); }
  value_type&   operator[](index_type pos) const { return get()[pos]; }
  value_type&   operator*() const { return *get(); }
  auto*         operator->() const { return get().operator->(); }
  operator index_type&() { return m_index; }
  operator const index_type&() const { return m_index; }

private:
  index_type m_index = 0;
};

// std::span equivalent but implemented with an index_ptr
template <auto ObjectsPtr, class IndexType = uint32_t, bool ConstHeader = false>
class index_span {
public:
  using pointer_type  = index_ptr<ObjectsPtr, IndexType, ConstHeader>;
  using range_type    = typename pointer_type::range_type;
  using iterator = typename pointer_type::iterator;
  using header_type   = typename pointer_type::header_type;
  using begin_result  = decltype(std::begin(std::declval<header_type>().*ObjectsPtr));
  using value_type    = typename pointer_type::value_type;
  using index_type    = IndexType;
  index_span()        = default;
  index_span(const index_type& index, const index_type& size)
      : m_index(index), m_size(size) {}
  static index_span
  from_pointer(value_type* pointer, index_type size,
               header_type& header = *bound_header<header_type>::get()) {
    auto base = std::ranges::data(header.*ObjectsPtr);
    return {static_cast<index_type>(pointer - base), size};
  }
  static index_span
  from_pointer(value_type* begin, value_type* end,
               header_type& header = *bound_header<header_type>::get()) {
    return from_pointer(
        begin, static_cast<index_type>(std::distance(begin, end)), header);
  }
  static index_span
  from_iterator(begin_result begin, index_type size,
                header_type& header = *bound_header<header_type>::get()) {
    auto base = std::ranges::begin(header.*ObjectsPtr);
    return {static_cast<index_type>(begin - base), size};
  }
  static index_span
  from_iterator(begin_result begin, begin_result end,
                header_type& header = *bound_header<header_type>::get()) {
    return from_iterator(
        begin, static_cast<index_type>(std::distance(begin, end)), header);
  }
  template <class Range>
    requires std::ranges::contiguous_range<Range>
  static index_span
  from_range(Range&&      range,
             header_type& header = *bound_header<header_type>::get()) {
    return from_pointer(std::ranges::data(range),
                        static_cast<index_type>(std::ranges::size(range)),
                        header);
  }
  value_type*       data() const { return &*m_index.get(); }
  const index_type& index() const { return m_index; }
  const index_type& size() const { return m_size; }
  auto              begin() const { return m_index.get(); }
  auto              end() const { return m_index.get() + size(); }
  value_type&       operator[](index_type pos) const { return begin()[pos]; }
  std::span<value_type> bind(const range_type& base) const {
    return {m_index.bind(base), m_size};
  }
  std::span<value_type> span() const { return {m_index.get(), m_size}; }
  std::span<value_type> subspan(index_type offset) const {
    return {m_index.get() + offset, m_size - offset};
  }
  bool operator==(const index_span& other) {
    return m_index == other.m_index && m_size == other.m_size;
  }
  template <class Range>
  bool operator==(Range&& other) const {
    return std::ranges::equal(*this, other);
  }

private:
  pointer_type m_index;
  index_type   m_size = 0;
};

} // namespace nodecode
