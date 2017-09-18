#ifndef _NODE
#define _NODE "NODE"
#include <iostream>
#include <cstring>
#if defined(__linux__) || defined(__APPLE__)

#if defined(__linux__) && defined(kernel_version_2_4)
#include <sys/sendfile.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
typedef int SOCKET;
#define INVALID_SOCKET -1

#elif defined(_WIN32)
#include <winsock2.h>
#define close(node_sock) closesocket(node_sock)
typedef int socklen_t;
#else
#error OS Not supported
#endif

//#include "clog.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 256
#endif

#ifndef CON_MAX_ATTEMPTS
#define CON_MAX_ATTEMPTS 5
#endif

#ifndef CLOG_H
#define log_inf(...)
#define log_err(...)
#define log_per(...)
#define log_fat(...)
#endif

namespace node
{
  class Node
  {
  protected:
    SOCKET node_sock;
    SOCKET start_server (int);
    SOCKET connect_server (const char *, int);
  public:
#define SH_BUFSIZE 1024		//shell read buffer size
#define SH_TOK_BUFSIZE 64
#define SH_TOK_DELIM " \t\r\n\a"	//shell token delimeter

    typedef struct
    {
      const char *word;
      const char letter;
      const char *description;
    } option;

    typedef struct
    {
      const char *command;
      const char *info;
      int (*function) (int, char **, Node *);
      int opt_length;
      option *options;
    } job;


      Node (void);
      Node (SOCKET);
     ~Node (void);

    int writeln (const char *);
    int writeln (const char *, int);
    char *readln (void);
    int write_data (void *, int);
    void *read_data ();
    int write_file (char *, char *);
    int read_file (char *, char *);

    Node *accept (int);
    int connect (const char *, int);

    int process (int, job *, const char *);
    //int disconnect_al(void);
  };

    Node::Node ()
  {
#if defined(_WIN32)
    //irritating winsock initialization
    WSADATA wsa;
      printf ("\nInitialising Winsock...");
    if (WSAStartup (MAKEWORD (2, 2), &wsa) != 0)
      {
	log_fat (_NODE_H, "Failed. Error Code : %d", WSAGetLastError ());
      }
#endif
  }

  Node::Node (SOCKET node_sock)
  {
    this->node_sock = node_sock;
  }

  Node::~Node ()
  {
    close (node_sock);
    //irritatin winsock cleanup
#if defined(_WIN32)
    WSACleanup ();
#endif
  }

  int Node::process (int jlen, job * jobs, const char *cmd)
  {
    if (cmd == NULL)
      return -1;
    int count = -1;
    int len = strlen (cmd);
    if (len == 0)
      return 0;
    log_inf ("SERVER", "Request received: %s", cmd);
    char *line = (char *) malloc (len);
    strncpy (line, cmd, len);
    ssize_t arg_buffsize = 64;	//chage variably
    char **args = (char **) calloc (arg_buffsize, SH_BUFSIZE);

    char *token = strtok (line, SH_TOK_DELIM);
    while (token != NULL)
      {
	args[++count] = token;
	if (count == arg_buffsize)
	  {
	    arg_buffsize += 64;	//change variably
	    args = (char **) realloc (args, sizeof (char) * arg_buffsize);
	  }
	token = strtok (NULL, SH_TOK_DELIM);
      }
    if (args[0] == NULL)
      return 0;
    for (int i = 0; i < jlen; i++)
      {
	if (strcmp (*args, jobs[i].command) == 0)
	  {
	    return (jobs[i].function) (count, ++args, this);
	  }
      }
    writeln ("you clearly need 'help'\n");
    return 0;			//FIXME add custom codes to identify returns and errors
  }

  int Node::writeln (const char *buf)
  {
    return writeln (buf, strlen (buf));
  }

  int Node::writeln (const char *buf, int len)
  {
    int bwrite = write (node_sock, buf, len);
    //log_inf("CLIENT", "written buf: %s, sent: %d", buf, bwrite);
    if (bwrite > 0)
      {
	if (bwrite < len)
	  {
	    return writeln (buf + bwrite, len - bwrite);
	  }
	if (bwrite == len)
	  {
	    send (node_sock, "\n", 1, 0);
	    //log_inf("CLIENT", "write sucessful");
	    return 0;
	  }
      }
    else if (bwrite == 0)
      {
	log_inf ("CLIENT", "Unspecified write error");
      }
    else if (bwrite < 0)
      {
	log_inf ("CLIENT", "write error, exiting...");
      }
    return -1;
  }

  char *Node::readln ()
  {
    char *buf = (char *) malloc (256);
    int ptr = 0;
    memset (buf, '\0', 256);
    int quit = 0;
    for (ptr = 0; quit != 1; ptr++)
      {
	int bread = read (node_sock, buf + ptr, 1);
	if (bread > 0)
	  {
	    //log_inf("SERVER", "Content read[%d]: %c", ptr, buf[ptr]);
	    if (buf[ptr] == '\n' || buf[ptr] == '\r')
	      {
		buf[ptr] = '\0';
		return buf;
	      }
	  }
	else if (bread == 0)
	  {			//EOF hit, client disconnected
	    log_inf ("SERVER", "EOF hit");
	    return NULL;
	  }
	else if (bread < 0)
	  {			//read error
	    log_inf ("SERVER", "read error exiting...");
	    return NULL;
	  }
      }
    return NULL;
  }

  int Node::connect (const char *hostname, int port)
  {
    if (connect_server (hostname, port) == INVALID_SOCKET){
			return -1;
		}
		return 0;
  }

  SOCKET Node::connect_server (const char *hostname, int port)
  {
    struct sockaddr_in serv_addr;
    struct hostent *server;
#if defined(_WIN32)
    WSAData wsadata;
    WSAStartup (MAKEWORD (2, 2), &wsadata);
    if (LOBYTE (wsadata) != 2 || HIBYTE (wsadata) != 2)
      {
	log_fat (_NODE, "Cannot initialize Winsock2 library, error code: %d",
		 WSAGetLastError ());
	return -1;
      }
#endif

    //checking whether port is between 0 and 65536
    if (port < 0 || port > 65535)
      {
	log_err (_NODE,
		 "invalid port number, port number should be between 0 and 65536");
	return -1;
      }
    //Create socket
    node_sock = socket (AF_INET, SOCK_STREAM, 0);
    if (node_sock == INVALID_SOCKET)
      {
	log_err (_NODE, "Could not create socket");
	return -1;
      }

    log_inf (_NODE, "Socket created");
    if ((server = gethostbyname (hostname)) == NULL)
      {
	log_err (_NODE, "no such host found");
	return -1;
      }
    memset ((char *) &serv_addr, 0, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy ((char *) &serv_addr.sin_addr.s_addr, (char *) server->h_addr,
	    server->h_length);
    serv_addr.sin_port = htons (port);
    int i = 0;
    while (::
	   connect (node_sock, (struct sockaddr *) &serv_addr,
		    sizeof (serv_addr)) == -1)
      {
	if (i++ > CON_MAX_ATTEMPTS)
	  {
	    //guess other hostnames for the user
	    close (node_sock);
	    log_err (_NODE, "cannot establish connection to %s on port %d",
		     hostname, port);
	    return -1;
	  }
      }
    log_inf (_NODE, "connection established successfully to %s on port %d",
	     hostname, port);
    return node_sock;
  }

#ifndef SERV_BACKLOG
#define SERV_BACKLOG 10
#endif

  Node *Node::accept (int port)
  {
    return new Node (start_server (port));
  }

//TODO support multiple server options
/** Starts the server with the standard IPv4 and TCP stack
 * @param port Port number for the server to start
 * @return Node of the started server
 */
  SOCKET Node::start_server (int port)
  {
    static int cont;
    static SOCKET node_sock;

    struct sockaddr_in server, client;
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons (port);
    socklen_t cli_size = sizeof (struct sockaddr_in);

    if (cont == port)
      {
	SOCKET cli_sock =::accept (node_sock, (struct sockaddr *) &client,
				   &cli_size);
	log_inf (_NODE, "Connection accepted");
	return cli_sock;
      }
    if (cont == 0)
      cont = port;
    //Create socket
    node_sock = socket (PF_INET, SOCK_STREAM, 0);
    if (node_sock == INVALID_SOCKET)
      {
	log_err (_NODE, "could not create socket");
	return -1;
      }

    int reuseaddr = 1;
    if (setsockopt
	(node_sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof (int)) == -1)
      {
	log_err ("SERVER", "cannot reuse socket");
	return -1;
      }

    //Bind
    if (bind (node_sock, (struct sockaddr *) &server, sizeof (server)) < 0)
      {
	log_err (_NODE, "bind failed");
	return -1;
      }
    //Listen
    listen (node_sock, SERV_BACKLOG);
    //Accept and incoming connection
    log_inf (_NODE, "Waiting for incoming connections...");
    //accept connection from an incoming client
    int clifd =::accept (node_sock, (struct sockaddr *) &client, &cli_size);
    if (clifd < 0)
      {
	log_inf (_NODE, "Accept failed");
#if defined(__linux__) || defined(__APPLE__)
	close (node_sock);
#elif defined(_WIN32)
	closesocket (node_sock);
#endif
	return -1;
      }
    log_inf (_NODE, "Connection accepted");
    return clifd;
  }
};
#endif
