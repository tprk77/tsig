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

template <typename TesterType>
class CopyMoveWrapper {
 public:
  CopyMoveWrapper(TesterType& tester)
      : tester_(tester),
        copy_count_ptr_(std::make_shared<std::size_t>(0)),
        move_count_ptr_(std::make_shared<std::size_t>(0))
  {
    // Do nothing
  }

  CopyMoveWrapper(const CopyMoveWrapper& other)
      : tester_(other.tester_),
        copy_count_ptr_(other.copy_count_ptr_),
        move_count_ptr_(other.move_count_ptr_)
  {
    ++(*copy_count_ptr_);
  }

  CopyMoveWrapper(CopyMoveWrapper&& other)
      : tester_(other.tester_),
        copy_count_ptr_(std::move(other.copy_count_ptr_)),
        move_count_ptr_(std::move(other.move_count_ptr_))
  {
    ++(*move_count_ptr_);
  }

  void operator()(const std::string& str, int x, int y)
  {
    tester_(str, x, y);
  }

  void ResetCounts()
  {
    *copy_count_ptr_ = 0;
    *move_count_ptr_ = 0;
  }

  TesterType& Tester() const
  {
    return *tester_;
  }

  std::size_t CopyCount() const
  {
    return *copy_count_ptr_;
  }

  std::size_t MoveCount() const
  {
    return *move_count_ptr_;
  }

 private:
  TesterType& tester_;
  std::shared_ptr<std::size_t> copy_count_ptr_;
  std::shared_ptr<std::size_t> move_count_ptr_;
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

TEST(Signal, ConnectHandlerCopy)
{
  VoidSignal signal;
  VoidSignalTester tester;
  CopyMoveWrapper<VoidSignalTester> wrapper(tester);
  const tsig::Sigcon sigcon = signal.Connect(wrapper);
  EXPECT_EQ(tester.NumCalls(), 0u);
  EXPECT_EQ(wrapper.CopyCount(), 1u);
  // NOTE The wrapper will get moved inside std::function
  EXPECT_EQ(wrapper.MoveCount(), 1u);
}

TEST(Signal, ConnectHandlerMove)
{
  VoidSignal signal;
  VoidSignalTester tester;
  CopyMoveWrapper<VoidSignalTester> wrapper(tester);
  CopyMoveWrapper<VoidSignalTester> move_wrapper(wrapper);
  wrapper.ResetCounts();
  const tsig::Sigcon sigcon = signal.Connect(std::move(move_wrapper));
  EXPECT_EQ(tester.NumCalls(), 0u);
  EXPECT_EQ(wrapper.CopyCount(), 0u);
  // NOTE The wrapper will get moved inside std::function
  EXPECT_EQ(wrapper.MoveCount(), 2u);
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

TEST(Signal, DropDuringEmit)
{
  VoidSignal signal;
  std::vector<VoidSignalTester> testers(NUM_MULTI_TESTERS);
  std::vector<tsig::Sigcon> sigcons;
  for (std::size_t ii = 0; ii < NUM_MULTI_TESTERS; ++ii) {
    // Each tester is riggid to disconnect the next one
    VoidSignalTester& tester = testers.at(ii);
    tester.SetPostHandler([&, ii]() {
      const std::size_t next_index = (ii + 1) % NUM_MULTI_TESTERS;
      sigcons.at(next_index).Reset();
    });
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
    ASSERT_EQ(tester.NumCalls(), 1u);
    EXPECT_EQ(tester.CalledArgs(0), VoidSignalArgs("BLUE", 1, 2));
  }
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
