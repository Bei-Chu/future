#include "Server.h"

Server::Server() {
  thread_ = std::thread([this]() {
    run();
  });
}

Server::~Server() {
  input_channel_.Close();
  thread_.join();
}

future<int> Server::serve(int input) {
  if (promises_.count(input)) {
    throw input;
  }

  auto &promise = promises_[input];
  input_channel_.Put(input);
  return promise.get_future();
}

future<void> Server::serve() {
  if (void_promise_) {
    void_promise_->set_value();
  }
  void_promise_ = promise<void>();
  return void_promise_->get_future();
}

void Server::update() {
  if (void_promise_) {
    void_promise_->set_value();
    void_promise_ = std::nullopt;
  }
  auto result_opt = output_channel_.TryTake();
  if (result_opt) {
    auto &result = *result_opt;
    promises_[result.first].set_value(result.second);
  }
}

void Server::run() {
  while (true) {
    try {
      int input = input_channel_.Take();
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1s);
      output_channel_.Put({input, input * 2});
    } catch (const ChannelClosedError&) {
      break;
    }
  }
}
