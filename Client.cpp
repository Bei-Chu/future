#include "Client.h"
#include <iostream>
#include "Server.h"

void f() {

}

Client::Client(Server &server) : server_(server) {}

void Client::start() {
  auto request = std::bind(&Client::request, this, std::placeholders::_1);
  request(1).then(request).then(request).then([this](int i) {
    std::cout << "Result: " << i << std::endl;
  }).then([this]() {
    return server_.serve();
  }).then([]() {
    return 100;
  }).then(request).then(request).then(request).then([this](int i) {
    std::cout << "Result " << i << std::endl;
  }).then([this]() {
    return server_.serve();
  }).then([this]() {
    finished_ = true;
  });
}

bool Client::finished() const {
  return finished_;
}

future<int> Client::request(int i) {
  return server_.serve(i);
}
