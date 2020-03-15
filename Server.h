#include <functional>
#include <thread>
#include "future.h"
#include "Channel.h"

class Server {
public:
  Server();
  ~Server();

  future<int> serve(int input);
  future<> serve();
  void update();

private:
  void run();

  Channel<int> input_channel_;
  Channel<std::pair<int, int>> output_channel_;
  std::thread thread_;

  std::unordered_map<int, promise<int>> promises_;
  std::optional<promise<>> void_promise_;
};
