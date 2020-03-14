#include "Server.h"
#include "Client.h"

int main() {
  Server server;
  Client client(server);
  client.start();
  while (true) {
    server.update();
    if (client.finished()) {
      break;
    }
  }
}
