#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/producerconsumerstream.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <shared_mutex>
#include <thread>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
using namespace concurrency;

class SSEManager {
 private:
  std::vector<streams::producer_consumer_buffer<char>*> subscribers;
  std::shared_mutex subscriptionMutex;

 public:
  void subscribe(streams::producer_consumer_buffer<char>* subscriber) {
    std::unique_lock lock(subscriptionMutex);
    subscribers.push_back(subscriber);
  }

  void unsubscribe(streams::producer_consumer_buffer<char>* subscriber) {
    std::unique_lock lock(subscriptionMutex);
    subscribers.erase(
        std::remove(subscribers.begin(), subscribers.end(), subscriber),
        subscribers.end());
  }

  void broadcast(const std::string& message) {
    std::shared_lock lock(subscriptionMutex);
    std::string sseMessage = "data: " + message + "\n\n";
    for (auto& subscriber : subscribers) {
      subscriber->putn_nocopy(sseMessage.data(), sseMessage.size()).wait();
      subscriber->sync().wait();
    }
  }
};

static SSEManager manager;

// SSE example - send date every second until client disconnects over SSE
// protocol
void handle_get(http_request request) {
  http_response response(status_codes::OK);
  response.headers().add(header_names::content_type, U("text/event-stream"));
  response.headers().add(header_names::cache_control, U("no-cache"));
  response.headers().add(header_names::connection, U("keep-alive"));

  streams::producer_consumer_buffer<char> buffer;
  streams::basic_istream<uint8_t> instream(buffer);
  response.set_body(instream);
  auto task = request.reply(response);
  try {
    manager.subscribe(&buffer);
    task.wait();
    manager.unsubscribe(&buffer);
  } catch (const std::exception& e) {
    manager.unsubscribe(&buffer);
    std::cerr << "An error occurred while replying to the request: " << e.what()
              << std::endl;
  }
}

int main() {
  uri_builder uri(U("http://localhost:34568"));
  auto addr = uri.to_uri().to_string();
  http_listener listener(addr);

  listener.support(methods::GET, handle_get);

  try {
    listener.open()
        .then([&addr]() {
          std::cout << "Starting to listen at: " << addr << std::endl;
          // example infinite log messages
          std::thread([] {
            for (int i = 0;; i++) {
              manager.broadcast(std::to_string(i));
              std::this_thread::sleep_for(std::chrono::seconds(1));
            }
          }).detach();
        })
        .wait();
    std::string line;
    std::getline(std::cin, line);
    listener.close().wait();
  } catch (const std::exception& e) {
    std::cerr << "An error occurred while setting up the server: " << e.what()
              << std::endl;
  }

  return 0;
}
