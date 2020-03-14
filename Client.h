#include "future.h"

class Server;

class Client {
public:
  Client(Server &server);

  void start();

  bool finished() const;

private:
  future<int> request(int i);

private:
  Server &server_;
  bool finished_{false};
};
