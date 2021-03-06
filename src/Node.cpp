#include <Node/Node.hpp>

node::Node::Node() {
#if defined(_WIN32)
  // irritating winsock initialization
  WSADATA wsa;
  log_inf(_NODE_H, "\nInitialising Winsock...");
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    log_fat(_NODE_H, "Failed. Error Code : %d", WSAGetLastError());
  }
#endif
}

node::Node::Node(SOCKET node_sock) { 
  this->node_sock = node_sock;
  std::async(std::launch::async, &node::Node::_read, this);
}

node::Node::~Node() {
  ::close(node_sock);
#if defined(_WIN32)
  WSACleanup();
#endif
}

int node::Node::wait(){
	while(isNodeConnected);
	return 0;
}

int node::Node::_read(){
	while(isNodeConnected){
		if(node_sock != INVALID_SOCKET){
			std::string buffer = readln();
			this->buffer = buffer;
			fireEvent(Events::ReadLine, this);
		}
	}
	return 0;
}


void node::Node::setOnEventListener(node::OnEventListener *eventListener){
	eventListeners.push_back(eventListener);
}

void node::Node::setServerPort(int serverPort){
  this->serverPort = serverPort > 0 && serverPort <= 65536 ? serverPort : NODE_DEFAULT_PORT;
}

void node::Node::start(){
  accept(serverPort);
}

void node::Node::fireEvent(Events events, Node *n){
	for ( auto &c : eventListeners ){
		if(c){
			c->onEvent(events, n);
		}
	}
}

int node::Node::writeln(std::string buf) { return writeln(buf.c_str(), strlen(buf.c_str())); }

int node::Node::writeln(const char *buf, int len) {
  int bwrite = send(node_sock, buf, len, 0);
  // log_inf("CLIENT", "written buf: %s, sent: %d", buf, bwrite);
  if (bwrite > 0) {
    if (bwrite < len) {
      return writeln(buf + bwrite, len - bwrite);
    }
    if (bwrite == len) {
      send(node_sock, "\n", 1, 0);
      // log_inf("CLIENT", "write sucessful");
      return 0;
    }
  } else if (bwrite == 0) {
    log_inf("CLIENT", "Unspecified write error");
  } else if (bwrite < 0) {
    log_inf("CLIENT", "write error, exiting...");
  }
  return -1;
}

const char *node::Node::readln() {
  char *buf = (char *)malloc(256);
  int ptr = 0;
  memset(buf, '\0', 256);
  int quit = 0;
  for (ptr = 0; quit != 1; ptr++) {
    int bread = recv(node_sock, buf + ptr, 1, 0);
    if (bread > 0) {
      // log_inf("SERVER", "Content read[%d]: %c", ptr, buf[ptr]);
      if (buf[ptr] == '\n' || buf[ptr] == '\r') {
        buf[ptr] = '\0';
        return buf;
      }
    } else if (bread == 0) { // EOF hit, client disconnected
      log_inf("SERVER", "EOF hit");
      return NULL;
    } else if (bread < 0) { // read error
      log_inf("SERVER", "read error exiting...");
      return NULL;
    }
  }
  return NULL;
}

int node::Node::connect(const char *hostname, int port) {
  if (connect_server(hostname, port) == INVALID_SOCKET) {
    return -1;
  }
  fireEvent(Connect, this);
  return 0;
}

SOCKET node::Node::connect_server(const char *hostname, int port) {
  struct sockaddr_in serv_addr;
  struct hostent *server;
#if defined(_WIN32)
  WSAData wsadata;
  WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

  // checking whether port is between 0 and 65536
  if (port < 0 || port > 65535) {
    log_err(_NODE,
            "invalid port number, port number should be between 0 and 65536");
    return -1;
  }
  // Create socket
  node_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (node_sock == INVALID_SOCKET) {
    log_err(_NODE, "Could not create socket");
    return -1;
  }

  log_inf(_NODE, "Socket created");
  if ((server = gethostbyname(hostname)) == NULL) {
    log_err(_NODE, "no such host found");
    return -1;
  }
  memset((char *)&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr,
         server->h_length);
  serv_addr.sin_port = htons(port);
  int i = 0;
  while (::connect(node_sock, (struct sockaddr *)&serv_addr,
                   sizeof(serv_addr)) == -1) {
    if (i++ > CON_MAX_ATTEMPTS) {
      // guess other hostnames for the user
      ::close(node_sock);
      log_err(_NODE, "cannot establish connection to %s on port %d", hostname,
              port);
      return -1;
    }
  }
  log_inf(_NODE, "connection established successfully to %s on port %d",
          hostname, port);
  return node_sock;
}

#ifndef SERV_BACKLOG
#define SERV_BACKLOG 10
#endif

bool node::Node::accept(int port) {
  SOCKET node_status = start_server(port);
  if (node_status < 0){
    log_err(_NODE, "Cannot establish service");
    return false;
  }
  fireEvent(Events::Accept, new Node(node_status));
  return true;
}

// TODO support multiple server options
/** Starts the server with the standard IPv4 and TCP stack
 * @param port Port number for the server to start
 * @return node::Node::Node of the started server
 */
SOCKET node::Node::start_server(int port) {
  static int cont;
  static SOCKET node_sock;

  struct sockaddr_in server, client;
  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  socklen_t cli_size = sizeof(struct sockaddr_in);

  if (cont == port) {
    SOCKET cli_sock =
        ::accept(node_sock, (struct sockaddr *)&client, &cli_size);
    log_inf(_NODE, "Connection accepted");
    return cli_sock;
  }
  if (cont == 0)
    cont = port;
  // Create socket
  node_sock = socket(PF_INET, SOCK_STREAM, 0);
  if (node_sock == INVALID_SOCKET) {
    log_err(_NODE, "could not create socket");
    return -1;
  }

  char reuseaddr = 1;
  if (setsockopt(node_sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
                 sizeof(int)) == -1) {
    log_err("SERVER", "cannot reuse socket");
    return -1;
  }

  // Bind
  if (bind(node_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    log_err(_NODE, "bind failed");
    return -1;
  }
  // Listen
  listen(node_sock, SERV_BACKLOG);
  // Accept and incoming connection
  log_inf(_NODE, "Waiting for incoming connections...");
  // accept connection from an incoming client
  int clifd = ::accept(node_sock, (struct sockaddr *)&client, &cli_size);
  if (clifd < 0) {
    log_inf(_NODE, "Accept failed");
#if defined(__linux__) || defined(__APPLE__)
    ::close(node_sock);
#elif defined(_WIN32)
    ::close(node_sock);
#endif
    return -1;
  }
  log_inf(_NODE, "Connection accepted");
  return clifd;
}

void node::Node::close() {
  ::close(node_sock);
  isNodeConnected = false;
}
