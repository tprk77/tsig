// Copyright (c) 2021 Tim Perkins

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

#ifndef TSIG_TN_NODE_HPP
#define TSIG_TN_NODE_HPP

#include <tsig/signal.hpp>

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define TSIG_CHECK_RESULT __attribute__((warn_unused_result))
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
#define TSIG_CHECK_RESULT _Check_return_
#else
#define TSIG_CHECK_RESULT
#endif

namespace tsig {
namespace tn {

// Handlers are used to receive data from other nodes
template <typename T>
using DataHandler = std::function<void(const T&)>;

// Sinks are used to send data to other node
template <typename T>
using DataSink = std::function<void(const T&)>;

// Connectors are used to accept connections
template <typename T>
using DataConnector = std::function<tsig::Sigcon(const DataHandler<T>&)>;

// Used by the builder and the tasks used to build nodes
template <typename... T>
using DataHandlerTuple = std::tuple<DataHandler<T>...>;

// Used by the builder and the tasks used to build nodes
template <typename... T>
using DataSinkTuple = std::tuple<DataSink<T>...>;

// Used as part of the node type DSL
template <typename...>
struct WithInputs {
  // Empty
};

// Used when a node has no inputs
using WithoutInputs = WithInputs<>;

// Used as part of the node type DSL
template <typename...>
struct WithOutputs {
  // Empty
};

// Used when a node has no outputs
using WithoutOutputs = WithOutputs<>;

// A node with inputs and outputs
template <typename...>
class Node;

// A node with inputs and outputs
template <typename... InputTypes, typename... OutputTypes>
class Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>> {
 public:
  template <size_t input_index>
  using InputAt = typename std::tuple_element<input_index, std::tuple<InputTypes...>>::type;
  template <size_t output_index>
  using OutputAt = typename std::tuple_element<output_index, std::tuple<OutputTypes...>>::type;

  template <size_t input_index>
  void RegisterHandler(const DataHandler<InputAt<input_index>>& handler);
  template <size_t output_index>
  DataSink<OutputAt<output_index>> GetSink();

  template <size_t input_index>
  void Accept(const DataConnector<InputAt<input_index>>& connector);
  template <size_t input_index, typename U>
  void Accept(U& connectable);

  template <size_t output_index>
  TSIG_CHECK_RESULT tsig::Sigcon Connect(const DataHandler<OutputAt<output_index>>& handler);
  template <size_t output_index, size_t input_index, typename U>
  void Connect(U& acceptable);

 private:
  static constexpr size_t num_inputs = sizeof...(InputTypes);
  static constexpr size_t num_outputs = sizeof...(OutputTypes);

  std::array<tsig::Sigcon, num_inputs> sigcons_;
  std::tuple<typename tsig::Signal<void(const InputTypes&)>::Handler...> handlers_;
  std::tuple<tsig::Signal<void(const OutputTypes&)>...> signals_;
};

// Used to construct a node from a task
template <typename NodeType>
class NodeBuilder;

// Used to construct a node from a task
template <typename... InputTypes, typename... OutputTypes>
class NodeBuilder<Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>>> {
 public:
  using NodeType = Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>>;

  template <typename TaskType>
  static NodeType Build(TaskType&& task);
};

namespace detail {

// Used to register all handlers of a node
template <size_t input_index = 0, typename NodeType, typename... T>
typename std::enable_if<(input_index < sizeof...(T)), void>::type  //
RegisterAllDataHandlers(NodeType& node, const std::tuple<T...>& handlers);

// Used to register all handlers of a node (terminates recursion)
template <size_t input_index = 0, typename NodeType, typename... T>
typename std::enable_if<input_index == sizeof...(T), void>::type  //
RegisterAllDataHandlers(NodeType&, const std::tuple<T...>&);

// Used to get all sinks of a node
template <size_t output_index = 0, typename NodeType, typename... T>
typename std::enable_if<(output_index < sizeof...(T)), void>::type  //
GetAllDataSinks(NodeType& node, std::tuple<T...>& sinks);

// Used to get all sinks of a node (terminates recursion)
template <size_t output_index = 0, typename NodeType, typename... T>
typename std::enable_if<output_index == sizeof...(T), void>::type  //
GetAllDataSinks(NodeType&, std::tuple<T...>&);

}  // namespace detail

template <typename... InputTypes, typename... OutputTypes>
template <size_t input_index>
void Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>>::RegisterHandler(
    const DataHandler<InputAt<input_index>>& handler)
{
  std::get<input_index>(handlers_) = handler;
}

template <typename... InputTypes, typename... OutputTypes>
template <size_t output_index>
DataSink<typename Node<WithInputs<InputTypes...>,
                       WithOutputs<OutputTypes...>>::template OutputAt<output_index>>
Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>>::GetSink()
{
  return [this](const OutputAt<output_index>& value) {  //
    std::get<output_index>(signals_).Emit(value);
  };
}

template <typename... InputTypes, typename... OutputTypes>
template <size_t input_index>
void Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>>::Accept(
    const DataConnector<InputAt<input_index>>& connector)
{
  std::get<input_index>(sigcons_) = connector(std::get<input_index>(handlers_));
}

template <typename... InputTypes, typename... OutputTypes>
template <size_t input_index, typename U>
void Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>>::Accept(U& connectable)
{
  Accept<input_index>(
      [&connectable](const DataHandler<InputAt<input_index>>& handler) -> tsig::Sigcon {
        return connectable.Connect(handler);
      });
}

template <typename... InputTypes, typename... OutputTypes>
template <size_t output_index>
tsig::Sigcon Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>>::Connect(
    const DataHandler<OutputAt<output_index>>& handler)
{
  return std::get<output_index>(signals_).Connect(handler);
}

template <typename... InputTypes, typename... OutputTypes>
template <size_t output_index, size_t input_index, typename U>
void Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>>::Connect(U& acceptable)
{
  acceptable.template Accept<input_index>(
      [this](const DataHandler<OutputAt<output_index>>& handler) -> tsig::Sigcon {
        return Connect<output_index>(handler);
      });
}

template <typename... InputTypes, typename... OutputTypes>
template <typename TaskType>
typename NodeBuilder<Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>>>::NodeType
NodeBuilder<Node<WithInputs<InputTypes...>, WithOutputs<OutputTypes...>>>::Build(TaskType&& task)
{
  NodeType node;
  DataHandlerTuple<InputTypes...> handlers = task.GetHandlers();
  detail::RegisterAllDataHandlers(node, handlers);
  DataSinkTuple<OutputTypes...> sinks;
  detail::GetAllDataSinks(node, sinks);
  task.SetSinks(sinks);
  return node;
}

namespace detail {

template <size_t input_index, typename NodeType, typename... T>
typename std::enable_if<(input_index < sizeof...(T)), void>::type  //
RegisterAllDataHandlers(NodeType& node, const std::tuple<T...>& handlers)
{
  node.template RegisterHandler<input_index>(std::get<input_index>(handlers));
  RegisterAllDataHandlers<input_index + 1>(node, handlers);
}

template <size_t input_index, typename NodeType, typename... T>
typename std::enable_if<input_index == sizeof...(T), void>::type  //
RegisterAllDataHandlers(NodeType&, const std::tuple<T...>&)
{
  // Do nothing
}

template <size_t output_index, typename NodeType, typename... T>
typename std::enable_if<(output_index < sizeof...(T)), void>::type  //
GetAllDataSinks(NodeType& node, std::tuple<T...>& sinks)
{
  std::get<output_index>(sinks) = node.template GetSink<output_index>();
  GetAllDataSinks<output_index + 1>(node, sinks);
}

template <size_t output_index, typename NodeType, typename... T>
typename std::enable_if<output_index == sizeof...(T), void>::type  //
GetAllDataSinks(NodeType&, std::tuple<T...>&)
{
  // Do nothing
}

}  // namespace detail
}  // namespace tn
}  // namespace tsig

#undef TSIG_CHECK_RESULT

#endif  // TSIG_TN_NODE_HPP
