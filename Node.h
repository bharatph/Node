#ifndef _NODE
#define _NODE "NODE"
#include <iostream>
#include <cstdlib>
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
typedef int ssize_t;
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
#if defined(__linux__) || defined(__APPLE__)
    int write_data (void *, int);
    void *read_data ();
    int write_file (char *, char *);
    int read_file (char *, char *);
#endif

    Node *accept (int);
    int connect (const char *, int);

    int process (int, job *, const char *);
    //int disconnect_al(void);
  };
};
#endif
