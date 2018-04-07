#include <iostream>
#include <memory>

#include "epoll.h"
#include "logger.h"
#include "server_socket.h"
#include "signal_handler.h"
#include "terminal_error.h"

using epoll::Epoll;
using epoll::SignalHandler;
using std::cout;
using std::endl;


void printUsage() {
  cout << "Usage: ./proxy port" << endl;
  cout << "port - port which your proxy will listen to." << endl;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printUsage();
    return 0;
  }

  InitFileLogger("proxy.log");
  std::shared_ptr<Epoll> epoll_ptr;
  try {
    epoll_ptr = std::make_shared<Epoll>();
    std::unique_ptr<SignalHandler> handler_ptr(new SignalHandler(epoll_ptr));
    sockets::ServerSocket server(epoll_ptr, 4, atoi(argv[1]));
    for (;;) {
      epoll_ptr->Process();
    }
  } catch (TerminalError& error) {}

  epoll_ptr = nullptr;
  LOGI << "TERMINATION";
  return 0;
}
