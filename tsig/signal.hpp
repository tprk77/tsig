// Copyright (c) 2019 Tim Perkins

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// “Software”), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef TSIG_SIGNAL_HPP
#define TSIG_SIGNAL_HPP

#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <vector>

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define TSIG_CHECK_RESULT __attribute__((warn_unused_result))
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
#define TSIG_CHECK_RESULT _Check_return_
#else
#define TSIG_CHECK_RESULT
#endif

namespace tsig {

// template <typename Dispatcher>
// class SignalFactory;

class Sigcon;

template <typename Func>
class Signal;

template <typename Func>
class SignalConnector;

namespace detail {

static constexpr std::size_t INVALID_HANDLER_ID = std::numeric_limits<std::size_t>::max();

class SigdatBase;

template <typename Func>
class Sigdat;

}  // namespace detail

class Sigcon {
  template <typename Func>
  friend class Signal;
  template <typename Func>
  friend class SignalConnector;

 public:
  Sigcon();
  Sigcon(const Sigcon&) = delete;
  Sigcon(Sigcon&& sigcon);
  ~Sigcon();

  Sigcon& operator=(const Sigcon&) = delete;
  Sigcon& operator=(Sigcon&& sigcon);

  void Reset();

 private:
  Sigcon(const std::weak_ptr<detail::SigdatBase>& sigdat_wptr, std::size_t handler_id);

  std::weak_ptr<detail::SigdatBase> sigdat_wptr_;
  std::size_t handler_id_;
};

template <typename... Param>
class Signal<void(Param...)> {
  friend class SignalConnector<void(Param...)>;

 public:
  using Handler = std::function<void(Param...)>;

  Signal();
  Signal(const Signal&) = delete;
  Signal(Signal&& signal) = default;

  Signal& operator=(const Signal&) = delete;
  Signal& operator=(Signal&& signal) = default;

  TSIG_CHECK_RESULT Sigcon Connect(const Handler& handler);
  TSIG_CHECK_RESULT Sigcon Connect(Handler&& handler);
  void Emit(Param&&... param) const;

 private:
  std::shared_ptr<detail::Sigdat<void(Param...)>> sigdat_ptr_;
};

// template <typename Ret, typename ...Param>
// class Signal<Ret(Param...)> : private SignalBase {
// };

template <typename Func>
class SignalConnector {
 public:
  using Handler = typename Signal<Func>::Handler;

  explicit SignalConnector(Signal<Func>& signal);
  SignalConnector(const SignalConnector&) = default;
  SignalConnector(SignalConnector&&) = default;

  SignalConnector& operator=(const SignalConnector&) = default;
  SignalConnector& operator=(SignalConnector&&) = default;

  TSIG_CHECK_RESULT Sigcon Connect(const Handler& handler);
  TSIG_CHECK_RESULT Sigcon Connect(Handler&& handler);

  TSIG_CHECK_RESULT Sigcon operator()(const Handler& handler);
  TSIG_CHECK_RESULT Sigcon operator()(Handler&& handler);

 private:
  std::weak_ptr<detail::Sigdat<Func>> sigdat_wptr_;
};

template <typename Func>
SignalConnector<Func> MakeSignalConnector(Signal<Func>& signal);

namespace detail {

class SigdatBase {
 public:
  virtual ~SigdatBase() = default;
  virtual void RemoveHandler(std::size_t handler_id) = 0;
};

template <typename... Param>
class Sigdat<void(Param...)> final : public SigdatBase {
 public:
  using Handler = std::function<void(Param...)>;

  std::size_t AddHandler(const Handler& handler);
  std::size_t AddHandler(Handler&& handler);
  void CallHandlers(Param&&... param) const;
  void RemoveHandler(std::size_t handler_id) final;

 private:
  std::size_t incremental_handler_id_ = 0u;
  std::map<std::size_t, std::shared_ptr<Handler>> handler_ptrs_;
};

}  // namespace detail

Sigcon::Sigcon() : handler_id_(detail::INVALID_HANDLER_ID)
{
  // Do nothing
}

Sigcon::Sigcon(const std::weak_ptr<detail::SigdatBase>& sigdat_wptr, std::size_t handler_id)
    : sigdat_wptr_(sigdat_wptr), handler_id_(handler_id)
{
  // Do nothing
}

Sigcon::Sigcon(Sigcon&& sigcon) : sigdat_wptr_(sigcon.sigdat_wptr_), handler_id_(sigcon.handler_id_)
{
  sigcon.sigdat_wptr_.reset();
  sigcon.handler_id_ = detail::INVALID_HANDLER_ID;
}

Sigcon::~Sigcon()
{
  const std::shared_ptr<detail::SigdatBase> sigdat_ptr = sigdat_wptr_.lock();
  if (sigdat_ptr) {
    sigdat_ptr->RemoveHandler(handler_id_);
  }
}

Sigcon& Sigcon::operator=(Sigcon&& sigcon)
{
  // Remove the handler if there was one
  const std::shared_ptr<detail::SigdatBase> sigdat_ptr = sigdat_wptr_.lock();
  if (sigdat_ptr) {
    sigdat_ptr->RemoveHandler(handler_id_);
  }
  // Transfer over the handler info
  sigdat_wptr_ = sigcon.sigdat_wptr_;
  handler_id_ = sigcon.handler_id_;
  // Clean up the old handler info
  sigcon.sigdat_wptr_.reset();
  sigcon.handler_id_ = detail::INVALID_HANDLER_ID;
  return *this;
}

void Sigcon::Reset()
{
  // Remove the handler if there was one
  const std::shared_ptr<detail::SigdatBase> sigdat_ptr = sigdat_wptr_.lock();
  if (sigdat_ptr) {
    sigdat_ptr->RemoveHandler(handler_id_);
  }
  // Clean up the handler info
  sigdat_wptr_.reset();
  handler_id_ = detail::INVALID_HANDLER_ID;
}

template <typename... Param>
Signal<void(Param...)>::Signal() : sigdat_ptr_(std::make_shared<detail::Sigdat<void(Param...)>>())
{
  // Do nothing
}

template <typename... Param>
Sigcon Signal<void(Param...)>::Connect(const Signal::Handler& handler)
{
  const std::size_t handler_id = sigdat_ptr_->AddHandler(handler);
  return Sigcon(sigdat_ptr_, handler_id);
}

template <typename... Param>
Sigcon Signal<void(Param...)>::Connect(Signal::Handler&& handler)
{
  const std::size_t handler_id = sigdat_ptr_->AddHandler(std::move(handler));
  return Sigcon(sigdat_ptr_, handler_id);
}

template <typename... Param>
void Signal<void(Param...)>::Emit(Param&&... param) const
{
  sigdat_ptr_->CallHandlers(std::forward<Param>(param)...);
}

template <typename Func>
SignalConnector<Func>::SignalConnector(Signal<Func>& signal) : sigdat_wptr_(signal.sigdat_ptr_)
{
  // Do nothing
}

template <typename Func>
Sigcon SignalConnector<Func>::Connect(const Handler& handler)
{
  const std::shared_ptr<detail::Sigdat<Func>> sigdat_ptr = sigdat_wptr_.lock();
  if (!sigdat_ptr) {
    return {};
  }
  const std::size_t handler_id = sigdat_ptr->AddHandler(handler);
  return Sigcon(sigdat_ptr, handler_id);
}

template <typename Func>
Sigcon SignalConnector<Func>::Connect(Handler&& handler)
{
  const std::shared_ptr<detail::Sigdat<Func>> sigdat_ptr = sigdat_wptr_.lock();
  if (!sigdat_ptr) {
    return {};
  }
  const std::size_t handler_id = sigdat_ptr->AddHandler(std::move(handler));
  return Sigcon(sigdat_ptr, handler_id);
}

template <typename Func>
Sigcon SignalConnector<Func>::operator()(const Handler& handler)
{
  return Connect(handler);
}

template <typename Func>
Sigcon SignalConnector<Func>::operator()(Handler&& handler)
{
  return Connect(std::move(handler));
}

template <typename Func>
SignalConnector<Func> MakeSignalConnector(Signal<Func>& signal)
{
  return SignalConnector<Func>(signal);
}

namespace detail {

template <typename... Param>
std::size_t Sigdat<void(Param...)>::AddHandler(const Handler& handler)
{
  handler_ptrs_.emplace(incremental_handler_id_, std::make_shared<Handler>(handler));
  return incremental_handler_id_++;
}

template <typename... Param>
std::size_t Sigdat<void(Param...)>::AddHandler(Handler&& handler)
{
  handler_ptrs_.emplace(incremental_handler_id_, std::make_shared<Handler>(std::move(handler)));
  return incremental_handler_id_++;
}

template <typename... Param>
void Sigdat<void(Param...)>::CallHandlers(Param&&... param) const
{
  // Copy handlers to prevent in-flight modifications
  const auto handler_ptrs = handler_ptrs_;
  // TODO It would be more efficient to only copy if a modification takes place
  for (const auto& handler_pair : handler_ptrs) {
    const std::shared_ptr<Handler> handler_ptr = std::get<1>(handler_pair);
    // TODO Better exception handling
    (*handler_ptr)(std::forward<Param>(param)...);
  }
}

template <typename... Param>
void Sigdat<void(Param...)>::RemoveHandler(std::size_t handler_id)
{
  handler_ptrs_.erase(handler_id);
}

}  // namespace detail

}  // namespace tsig

#undef TSIG_CHECK_RESULT

#endif  // TSIG_SIGNAL_HPP
