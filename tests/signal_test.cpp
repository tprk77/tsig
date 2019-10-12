// Copyright (c) 2019 Tim Perkins

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "tsig/signal.hpp"

using MySignal = tsig::Signal<void(int, int, const std::string&)>;

TEST(Signal, Construct)
{
  MySignal signal;
}

TEST(Signal, ConstructMove)
{
  MySignal signal;
  MySignal signal2 = std::move(signal);
}

TEST(Signal, Connect)
{
  std::size_t num_called = 0;
  MySignal signal;
  signal.Connect([&](int x, int y, const std::string& str) {
    fmt::print("{}, {}, {}", x, y, str);
    ++num_called;
  });
}

TEST(Signal, Emit)
{
  std::size_t num_called = 0;
  MySignal signal;
  tsig::Sigcon sigcon = signal.Connect([&](int x, int y, const std::string& str) {
    fmt::print("{}, {}, {}\n", x, y, str);
    ++num_called;
  });
  signal.Emit(1, 2, "arstarst");
  EXPECT_EQ(num_called, std::size_t{1});
  signal.Emit(3, 4, "qwfpqwfp");
  EXPECT_EQ(num_called, std::size_t{2});
  sigcon = tsig::Sigcon();
  // This should not actually call anything
  signal.Emit(5, 6, "zxcvzxcv");
  EXPECT_EQ(num_called, std::size_t{2});
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
