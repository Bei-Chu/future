#ifndef FUTURE_H
#define FUTURE_H

#include <type_traits>
#include <functional>

template<typename ...Args>
class future;

template<typename ...Args>
class promise;

namespace traits {

template<typename ...Args>
struct convert_to_future {
  using type = future<Args...>;
};

template<typename ...Args>
struct convert_to_future<future<Args...>> {
  using type = future<Args...>;
};

template<>
struct convert_to_future<void> {
  using type = future<>;
};

template<typename ...Args>
using convert_to_future_t = typename convert_to_future<Args...>::type;

template<typename ...Args>
struct state {
  std::function<void(Args...)> on_value;
};

template<typename State, typename T>
struct call_on_future_or_value {
  call_on_future_or_value(State state, T&& value) {
    if (state->on_value) {
      state->on_value(std::forward<T>(value));
    }
  }
};

template<typename State, typename ...Args>
struct call_on_future_or_value<State, future<Args...>> {
  call_on_future_or_value(State state, future<Args...> future) {
    future.state_->on_value = state->on_value;
  }
};

template<typename F, typename State, typename NextState>
struct chain;

template<typename F, typename ...Args, typename ...NextArgs>
struct chain<F, state<Args...>, state<NextArgs...>> {
  using next_state_ptr_t = std::shared_ptr<state<NextArgs...>>;

  chain(F next_step, next_state_ptr_t next_state)
    : next_step_(std::move(next_step)), next_state_(std::move(next_state)) {}

  void operator()(Args&& ...args) const {
    auto &&value_or_future = next_step_(std::forward<Args>(args)...);
    call_on_future_or_value(std::move(next_state_), std::forward<decltype(value_or_future)>(value_or_future));
  }

private:
  mutable F next_step_;
  mutable next_state_ptr_t next_state_;
};

template<typename F, typename ...Args>
struct chain<F, state<Args...>, state<>> {
  using next_state_ptr_t = std::shared_ptr<state<>>;

  chain(F next_step, next_state_ptr_t next_state)
    : next_step_(std::move(next_step)), next_state_(std::move(next_state)) {}

  void operator()(Args&& ...args) const {
    next_step_(std::forward<Args>(args)...);
    if (next_state_->on_value) {
      next_state_->on_value();
    }
  }

private:
  mutable F next_step_;
  mutable next_state_ptr_t next_state_;
};

}

template<typename ...Args>
class future {
public:
  future() = default;
  future(const future&) = delete;
  future(future&&) = default;
  future& operator=(const future&) = delete;
  future& operator=(future&& other) {
    std::swap(*this, other);
    return *this;
  }
  ~future() = default;

  template<typename F>
  traits::convert_to_future_t<std::invoke_result_t<F, Args...>> then(F next_step);

private:
  template<typename ...Args>
  friend class promise;

  template<typename ...Args>
  friend class future;

  template<typename State, typename T>
  friend struct traits::call_on_future_or_value;

  future(std::shared_ptr<traits::state<Args...>> state) : state_(std::move(state)) {}

  using state_t = traits::state<Args...>;
  std::shared_ptr<state_t> state_;
};

template<typename ...Args>
template<typename F>
traits::convert_to_future_t<std::invoke_result_t<F, Args...>> future<Args...>::then(F next_step) {
  if (!state_) {
    throw "I have no future";
  }

  using next_future_t = traits::convert_to_future_t<std::invoke_result_t<F, Args...>>;
  using next_state_t = typename next_future_t::state_t;
  auto next_state = std::make_shared<next_state_t>();
  state_->on_value = traits::chain<F, state_t, next_state_t>(std::move(next_step), next_state);
  return next_future_t(std::move(next_state));
}

template<typename ...Args>
class promise {
public:
  promise();
  promise(const promise&) = delete;
  promise(promise&&) = default;
  promise& operator=(const promise&) = delete;
  promise& operator=(promise&& other) {
    std::swap(*this, other);
    return *this;
  }
  ~promise() = default;

  future<Args...> get_future();

  template<typename ...ValueArgs>
  void set_value(ValueArgs&& ...value);

private:
  std::shared_ptr<traits::state<Args...>> state_;
  bool future_generated_{ false };
  bool promise_fulfilled{ false };
};

template<typename ...Args>
promise<Args...>::promise() {
  state_ = std::make_shared<traits::state<Args...>>();
}

template<typename ...Args>
future<Args...> promise<Args...>::get_future() {
  if (future_generated_) {
    throw "I've promised someone else";
  }

  if (promise_fulfilled) {
    throw "I've fulfilled my promise";
  }

  future_generated_ = true;
  return future<Args...>(state_);
}

template<typename ...Args>
template<typename ...ValueArgs>
void promise<Args...>::set_value(ValueArgs&& ...value) {
  if (promise_fulfilled) {
    throw "I've fulfilled my promise";
  }

  if (state_->on_value) {
    state_->on_value(std::forward<ValueArgs>(value)...);
  }

  promise_fulfilled = true;
}

#endif
