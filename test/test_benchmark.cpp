// Copyright (c) 2024 Pyarelal Knowles, MIT License

#include <cstddef>
#include <iterator>
#define ANKERL_NANOBENCH_IMPLEMENT
#include <algorithm>
#include <nodecode/index_ptr.hpp>
#include <gtest/gtest.h>
#include <nanobench.h>
#include <random>
#include <limits>
#include <string>

using namespace ankerl;
using namespace nodecode;

struct Header {
  template <class Range>
  Header(Range&& data, Range&& indices)
      : m_data(data.begin(), data.end()), m_indices(indices.begin(), indices.end()),
        m_indexptrs(indices.begin(), indices.end()), m_dataspan(m_dataspan.from_range(m_data, *this)) {}
  std::vector<uint32_t>                 m_data;
  std::vector<uint32_t>                 m_indices;
  std::vector<index_ptr<&Header::m_data>> m_indexptrs;
  index_span<&Header::m_data>             m_dataspan;
};

template<class T>
std::vector<T> uniform_random_vector(size_t size, T max_value)
{
  std::random_device                 rd;
  std::mt19937                       gen(rd());
  std::uniform_int_distribution<int> distribution(std::numeric_limits<T>::min(), max_value);
  std::vector<uint32_t>              result(size);
  std::ranges::generate(result, [&]() { return distribution(gen); });
  return result;
}

TEST(Benchmark, SumIndices) {
  auto           data = uniform_random_vector<uint32_t>(1000000, 100);
  auto   indices = uniform_random_vector<uint32_t>(data.size(), data.size() - 1);
  Header header(data, indices);
  bound_header bound(header);

  uint32_t sum0 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("std::accumulate std::vector<uint32_t>", [&] {
        sum0 = std::accumulate(header.m_indices.begin(), header.m_indices.end(), 0);
        ankerl::nanobench::doNotOptimizeAway(sum0);
      });

  uint32_t sum1 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("std::accumulate std::vector<index_ptr>", [&] {
        sum1 = std::accumulate(header.m_indexptrs.begin(), header.m_indexptrs.end(), 0);
        ankerl::nanobench::doNotOptimizeAway(sum1);
      });
}

TEST(Benchmark, SumData) {
  auto data    = uniform_random_vector<uint32_t>(1000000, 100);
  auto indices = uniform_random_vector<uint32_t>(data.size(), data.size() - 1);
  Header       header(data, indices);
  bound_header bound(header);

  uint32_t sum0 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("std::vector<uint32_t>", [&] {
        sum0 = std::accumulate(header.m_data.begin(), header.m_data.end(), 0);
        ankerl::nanobench::doNotOptimizeAway(sum0);
      });

  uint32_t sum1 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("index_span", [&] {
        sum1 = std::accumulate(header.m_dataspan.begin(), header.m_dataspan.end(), 0);
        ankerl::nanobench::doNotOptimizeAway(sum1);
      });

  EXPECT_EQ(sum0, sum1);
}

TEST(Benchmark, SumIndexedData) {
  auto data    = uniform_random_vector<uint32_t>(1000000, 100);
  auto indices = uniform_random_vector<uint32_t>(data.size(), data.size() - 1);
  Header       header(data, indices);
  bound_header bound(header);

  uint32_t sum0 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("foreach += m_data[index]", [&] {
        sum0 = 0;
        for(auto& index : header.m_indices)
        {
          sum0 += header.m_data[index];
        }
        ankerl::nanobench::doNotOptimizeAway(sum0);
      });

  uint32_t sum1 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("foreach += m_data[ptr]", [&] {
        sum1 = 0;
        for(auto& ptr : header.m_indexptrs)
          sum1 += header.m_data[ptr];
        ankerl::nanobench::doNotOptimizeAway(sum1);
      });

  uint32_t sum2 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("foreach += *ptr.bind(header)", [&] {
        sum2 = 0;
        for(auto& ptr : header.m_indexptrs)
          sum2 += *ptr.bind(header);
        ankerl::nanobench::doNotOptimizeAway(sum2);
      });

  uint32_t sum3 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("foreach += *ptr", [&] {
        sum3 = 0;
        for(auto& ptr : header.m_indexptrs)
          sum3 += *ptr;
        ankerl::nanobench::doNotOptimizeAway(sum3);
      });

  uint32_t sum4 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("std::accumulate *ptr", [&] {
        sum4 = std::accumulate(
            header.m_indexptrs.begin(), header.m_indexptrs.end(), 0,
            [](uint32_t sum, auto& ptr) { return sum + (*ptr); });
        ankerl::nanobench::doNotOptimizeAway(sum4);
      });

  uint32_t sum5 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("std::accumulate *ptr.bind(header)", [&] {
        sum5 = std::accumulate(
            header.m_indexptrs.begin(), header.m_indexptrs.end(), 0,
            [&header](uint32_t sum, auto& ptr) { return sum + *ptr.bind(header); });
        ankerl::nanobench::doNotOptimizeAway(sum5);
      });

  uint32_t sum6 = 0;
  nanobench::Bench()
      .minEpochTime(std::chrono::milliseconds(50))
      .run("std::accumulate header.m_data[index]", [&] {
        sum6 = std::accumulate(
            header.m_indices.begin(), header.m_indices.end(), 0,
            [&header](uint32_t sum, const uint32_t& index) { return sum + header.m_data[index]; });
        ankerl::nanobench::doNotOptimizeAway(sum6);
      });

  EXPECT_EQ(sum0, sum1);
  EXPECT_EQ(sum0, sum2);
  EXPECT_EQ(sum0, sum3);
  EXPECT_EQ(sum0, sum4);
  EXPECT_EQ(sum0, sum5);
  EXPECT_EQ(sum0, sum6);
}
