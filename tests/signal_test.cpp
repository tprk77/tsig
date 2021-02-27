// Copyright (c) 2019 Tim Perkins

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "tsig/signal.hpp"

constexpr std::size_t NUM_MULTI_TESTERS = 10;

using VoidSignal = tsig::Signal<void(const std::string&, int, int)>;

struct VoidSignalArgs {
  VoidSignalArgs(const std::string& str, int x, int y) : str(str), x(x), y(y) {}

  bool operator==(const VoidSignalArgs& args) const
  {
    return str == args.str && x == args.x && y == args.y;
  }

  std::string str;
  int x;
  int y;
};

class VoidSignalTester {
 public:
  VoidSignalTester() = default;
  VoidSignalTester(const VoidSignalTester&) = delete;
  VoidSignalTester(VoidSignalTester&&) = delete;

  void operator()(const std::string& str, int x, int y)
  {
    called_args_.emplace_back(str, x, y);
    if (post_handler_) {
      post_handler_();
    }
  }

  std::size_t NumCalls() const
  {
    return called_args_.size();
  }

  VoidSignalArgs CalledArgs(std::size_t index) const
  {
    return called_args_.at(index);
  }

  void SetPostHandler(const std::function<void(void)>& post_handler)
  {
    post_handler_ = post_handler;
  }

 private:
  std::vector<VoidSignalArgs> called_args_;
  std::function<void(void)> post_handler_;
};

TEST(Signal, Construct)
{
  VoidSignal signal;
}

TEST(Signal, ConstructMove)
{
  VoidSignal signal;
  VoidSignal other_signal(std::move(signal));
}

TEST(Signal, AssignmentMove)
{
  VoidSignal signal;
  VoidSignal other_signal;
  other_signal = std::move(signal);
}

TEST(Signal, Connect)
{
  VoidSignal signal;
  VoidSignalTester tester;
  const tsig::Sigcon sigcon = signal.Connect(std::ref(tester));
  EXPECT_EQ(tester.NumCalls(), 0u);
}

TEST(Signal, ConnectDropped)
{
  VoidSignalTester tester;
  {
    tsig::Sigcon sigcon;
    {
      VoidSignal signal;
      sigcon = signal.Connect(std::ref(tester));
    }
  }
  EXPECT_EQ(tester.NumCalls(), 0u);
}

TEST(Signal, Emit)
{
  VoidSignal signal;
  VoidSignalTester tester;
  tsig::Sigcon sigcon = signal.Connect(std::ref(tester));
  EXPECT_EQ(tester.NumCalls(), 0u);
  signal.Emit("BLUE", 1, 2);
  ASSERT_EQ(tester.NumCalls(), 1u);
  EXPECT_EQ(tester.CalledArgs(0), VoidSignalArgs("BLUE", 1, 2));
  signal.Emit("RED", 3, 4);
  ASSERT_EQ(tester.NumCalls(), 2u);
  EXPECT_EQ(tester.CalledArgs(1), VoidSignalArgs("RED", 3, 4));
}

TEST(Signal, EmitNoConnection)
{
  VoidSignal signal;
  VoidSignalTester tester;
  tsig::Sigcon sigcon;
  EXPECT_EQ(tester.NumCalls(), 0u);
  signal.Emit("BLUE", 1, 2);
  EXPECT_EQ(tester.NumCalls(), 0u);
}

TEST(Signal, EmitMoved)
{
  VoidSignal signal;
  VoidSignalTester tester;
  tsig::Sigcon sigcon;
  {
    tsig::Sigcon other_sigcon = signal.Connect(std::ref(tester));
    EXPECT_EQ(tester.NumCalls(), 0u);
    signal.Emit("BLUE", 1, 2);
    ASSERT_EQ(tester.NumCalls(), 1u);
    EXPECT_EQ(tester.CalledArgs(0), VoidSignalArgs("BLUE", 1, 2));
    sigcon = std::move(other_sigcon);
  }
  signal.Emit("RED", 3, 4);
  ASSERT_EQ(tester.NumCalls(), 2u);
  EXPECT_EQ(tester.CalledArgs(1), VoidSignalArgs("RED", 3, 4));
}

TEST(Signal, EmitDropped)
{
  VoidSignal signal;
  VoidSignalTester tester;
  {
    tsig::Sigcon sigcon = signal.Connect(std::ref(tester));
    EXPECT_EQ(tester.NumCalls(), 0u);
    signal.Emit("BLUE", 1, 2);
    ASSERT_EQ(tester.NumCalls(), 1u);
    EXPECT_EQ(tester.CalledArgs(0), VoidSignalArgs("BLUE", 1, 2));
  }
  signal.Emit("RED", 3, 4);
  ASSERT_EQ(tester.NumCalls(), 1u);
  EXPECT_EQ(tester.CalledArgs(0), VoidSignalArgs("BLUE", 1, 2));
}

TEST(Signal, EmitReset)
{
  VoidSignal signal;
  VoidSignalTester tester;
  tsig::Sigcon sigcon = signal.Connect(std::ref(tester));
  EXPECT_EQ(tester.NumCalls(), 0u);
  signal.Emit("BLUE", 1, 2);
  ASSERT_EQ(tester.NumCalls(), 1u);
  EXPECT_EQ(tester.CalledArgs(0), VoidSignalArgs("BLUE", 1, 2));
  sigcon.Reset();
  signal.Emit("RED", 3, 4);
  ASSERT_EQ(tester.NumCalls(), 1u);
  EXPECT_EQ(tester.CalledArgs(0), VoidSignalArgs("BLUE", 1, 2));
}

TEST(Signal, EmitMultiple)
{
  VoidSignal signal;
  std::vector<VoidSignalTester> testers(NUM_MULTI_TESTERS);
  std::vector<tsig::Sigcon> sigcons;
  for (VoidSignalTester& tester : testers) {
    sigcons.push_back(signal.Connect(std::ref(tester)));
  }
  for (VoidSignalTester& tester : testers) {
    EXPECT_EQ(tester.NumCalls(), 0u);
  }
  signal.Emit("BLUE", 1, 2);
  for (VoidSignalTester& tester : testers) {
    ASSERT_EQ(tester.NumCalls(), 1u);
    EXPECT_EQ(tester.CalledArgs(0), VoidSignalArgs("BLUE", 1, 2));
  }
  signal.Emit("RED", 3, 4);
  for (VoidSignalTester& tester : testers) {
    ASSERT_EQ(tester.NumCalls(), 2u);
    EXPECT_EQ(tester.CalledArgs(1), VoidSignalArgs("RED", 3, 4));
  }
}

TEST(Signal, MoveConnected)
{
  VoidSignal signal;
  VoidSignalTester tester;
  tsig::Sigcon sigcon;
  {
    VoidSignal other_signal;
    sigcon = other_signal.Connect(std::ref(tester));
    EXPECT_EQ(tester.NumCalls(), 0u);
    other_signal.Emit("BLUE", 1, 2);
    ASSERT_EQ(tester.NumCalls(), 1u);
    EXPECT_EQ(tester.CalledArgs(0), VoidSignalArgs("BLUE", 1, 2));
    signal = std::move(other_signal);
  }
  signal.Emit("RED", 3, 4);
  ASSERT_EQ(tester.NumCalls(), 2u);
  EXPECT_EQ(tester.CalledArgs(1), VoidSignalArgs("RED", 3, 4));
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
