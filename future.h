#ifndef FUTURE_H
#define FUTURE_H

#include <functional>
#include <memory>

template<typename T>
class future;

namespace detail {

template<typename T>
struct futurize {
  using type = future<T>;
};

template<typename T>
struct futurize<future<T>> {
  using type = future<T>;
};

template<typename F, typename T>
struct next_future {
  using type = typename futurize<std::invoke_result_t<F, T>>::type;
};

template<typename F>
struct next_future<F, void> {
  using type = typename futurize<std::invoke_result_t<F>>::type;
};

template<typename F, typename T>
using next_future_t = typename next_future<F, T>::type;

template<typename T>
struct unfuturize {
  using type = T;
};

template<typename T>
struct unfuturize<future<T>> {
  using type = T;
};

template<typename T>
using unfuturize_t = typename unfuturize<T>::type;

template<typename T>
struct state {
  std::function<void(T)> callback;
};

template<typename U, typename T>
struct call_on_future_or_value {
  call_on_future_or_value(std::shared_ptr<state<U>> state, T t) {
    if (state->callback) {
      state->callback(std::move(t));
    }
  }
};

template<typename U, typename T>
struct call_on_future_or_value<U, future<T>> {
  call_on_future_or_value(std::shared_ptr<state<U>> state, future<T> t) {
    t.state_->callback = state->callback;
  }
};

template<typename F, typename T, typename U>
struct chain {
  chain(F next_step, std::shared_ptr<state<U>> next_state)
    : next_step_(std::move(next_step)), next_state_(std::move(next_state)) {}

  void operator()(T t) const {
    auto value_or_future = next_step_(std::move(t));
    call_on_future_or_value(std::move(next_state_), std::move(value_or_future));
  }

private:
  mutable F next_step_;
  mutable std::shared_ptr<state<U>> next_state_;
};

template<typename F, typename U>
struct chain<F, void, U> {
  chain(F next_step, std::shared_ptr<state<U>> next_state)
    : next_step_(std::move(next_step)), next_state_(std::move(next_state)) {}

  void operator()() const {
    auto value_or_future = next_step_();
    call_on_future_or_value(std::move(next_state_), std::move(value_or_future));
  }

private:
  mutable F next_step_;
  mutable std::shared_ptr<state<U>> next_state_;
};

template<typename F, typename T>
struct chain<F, T, void> {
  chain(F next_step, std::shared_ptr<state<void>> next_state)
    : next_step_(std::move(next_step)), next_state_(std::move(next_state)) {}

  void operator()(T t) const {
    next_step_(std::move(t));
    if (next_state_->callback) {
      next_state_->callback();
    }
  }

private:
  mutable F next_step_;
  mutable std::shared_ptr<state<void>> next_state_;
};

template<typename F>
struct chain<F, void, void> {
  chain(F next_step, std::shared_ptr<state<void>> next_state)
    : next_step_(std::move(next_step)), next_state_(std::move(next_state)) {}

  void operator()() const {
    next_step_();
    if (next_state_->callback) {
      next_state_->callback();
    }
  }

private:
  mutable F next_step_;
  mutable std::shared_ptr<state<void>> next_state_;
};

} // namespace detail

template<typename T>
class promise;

template<typename T>
class future {
public:
  using result_t = T;

  template<typename F>
  detail::next_future_t<F, T> then(F next_step);

private:
  template<typename T>
  friend class promise;

  template<typename U>
  friend class future;

  template<typename U, typename T>
  friend struct detail::call_on_future_or_value;

  future(std::shared_ptr<detail::state<T>> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::state<T>> state_;
};

template<typename T>
template<typename F>
inline detail::next_future_t<F, T> future<T>::then(F next_step) {
  using next_future_t = detail::next_future_t<F, T>;
  using next_result_t = typename next_future_t::result_t;
  auto next_state = std::make_shared<detail::state<next_result_t>>();
  state_->callback = detail::chain<F, T, next_result_t>(std::move(next_step), next_state);
  return next_future_t(std::move(next_state));
}

template<typename T>
class promise {
public:
  promise();
  future<T> get_future();
  void set_value(T value);

private:
  std::shared_ptr<detail::state<T>> state_;
  bool future_generated_{false};
  bool promise_fulfilled{false};
};

template<>
class promise<void> {
public:
  promise();
  future<void> get_future();
  void set_value();

private:
  std::shared_ptr<detail::state<void>> state_;
  bool future_generated_{ false };
  bool promise_fulfilled{ false };
};

template<typename T>
promise<T>::promise() {
  state_ = std::make_shared<detail::state<T>>();
}

template<typename T>
future<T> promise<T>::get_future() {
  if (future_generated_) {
    throw "I've promised someone else";
  }

  if (promise_fulfilled) {
    throw "I've fulfilled my promise";
  }

  future_generated_ = true;
  return future<T>(state_);
}

template<typename T>
void promise<T>::set_value(T value) {
  if (promise_fulfilled) {
    throw "I've fulfilled my promise";
  }

  if (state_->callback) {
    state_->callback(std::move(value));
  }
  promise_fulfilled = true;
}

inline promise<void>::promise() {
  state_ = std::make_shared<detail::state<void>>();
}

inline future<void> promise<void>::get_future() {
  if (future_generated_) {
    throw "I've promised someone else";
  }

  if (promise_fulfilled) {
    throw "I've fulfilled my promise";
  }

  future_generated_ = true;
  return future<void>(state_);
}

inline void promise<void>::set_value() {
  if (promise_fulfilled) {
    throw "I've fulfilled my promise";
  }

  if (state_->callback) {
    state_->callback();
  }
  promise_fulfilled = true;
}

#endif
