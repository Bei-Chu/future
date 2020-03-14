# future
An experimental future implementation

## future allows you to write following code:

```
future<Buffer> buffer = read_file_async("some_file_name");

future<Response> response = buffer.then([](Buffer buffer) {
  make_request_async(std::move(buffer));`
});

response.then([](Response response) {
  std::cout << "Got response " << response << std::endl;
});
```
