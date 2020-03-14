#ifndef CHANNEL_H

#include <exception>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

class ChannelClosedError : public std::exception {

};

template<typename T>
class Channel {
public:
  Channel();
  void Put(T data);
  T Take();
  std::optional<T> TryTake();
  void Close();

private:
  std::queue<T> buffer_;
  bool closed_{false};
  std::mutex mutex_;
  std::condition_variable cv_;
};

template<typename T>
Channel<T>::Channel() = default;

template<typename T>
void Channel<T>::Put(T data) {
  std::lock_guard lock(mutex_);
  if (closed_) {
    throw ChannelClosedError();
  }

  buffer_.push(std::move(data));
  cv_.notify_one();
}

template<typename T>
T Channel<T>::Take() {
  std::unique_lock lock(mutex_);
  cv_.wait(lock, [this]() {
    return !buffer_.empty() || closed_;
  });

  if (closed_) {
    throw ChannelClosedError();
  }

  T result = std::move(buffer_.front());
  buffer_.pop();
  return result;
}

template<typename T>
std::optional<T> Channel<T>::TryTake() {
  std::lock_guard lock(mutex_);
  if (closed_) {
    throw ChannelClosedError();
  }

  if (buffer_.empty()) {
    return {};
  } else {
    std::optional<T> result = std::move(buffer_.front());
    buffer_.pop();
    return result;
  }
}

template<typename T>
void Channel<T>::Close() {
  std::lock_guard lock(mutex_);
  closed_ = true;
  cv_.notify_one();
}

#endif
