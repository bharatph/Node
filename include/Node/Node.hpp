#ifndef _NODE
#define _NODE "NODE"
#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <future>
#if defined(__linux__) || defined(__APPLE__)

#if defined(__linux__) && defined(kernel_version_2_4)
#include <sys/sendfile.h>
#endif

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int SOCKET;
#define INVALID_SOCKET -1

#elif defined(_WIN32)
#include <winsock2.h>
#define close(node_sock) closesocket(node_sock)
typedef int socklen_t;
typedef int ssize_t;
#else
#error OS Not supported
#endif

namespace node {
class Node;
}

#include <Node/Events.hpp>
#include <Node/listener/OnEventListener.hpp>

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 256
#endif

#ifndef CON_MAX_ATTEMPTS
#define CON_MAX_ATTEMPTS 5
#endif

#define log_inf(...)
#define log_err(...)
#define log_per(...)
#define log_fat(...)

namespace node {
class Node {
private:
  SOCKET node_sock;
  SOCKET start_server(int);
  SOCKET connect_server(const char *, int);
  bool isNodeConnected = true;
  std::vector<OnEventListener*> eventListeners;

public:
  std::string buffer;
  int _read();

  Node(void);
  Node(SOCKET);
  ~Node(void);

  int wait();

  void setOnEventListener(OnEventListener *eventListener);
  void fireEvent(Events, Node *);

  int writeln(std::string);
  int writeln(const char *, int);
  const char *readln(void);
#if defined(__linux__) || defined(__APPLE__)
  int write_data(void *, int);
  void *read_data();
  int write_file(char *, char *);
  int read_file(char *, char *);
#endif

  bool accept(int);
  int connect(const char *, int);
  void close(void);
};
} // namespace node
#endif
