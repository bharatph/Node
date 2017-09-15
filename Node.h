#ifndef _NODE
#define _NODE "NODE"
#include <iostream>

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
#define INVALID_SOCKET -1

#elif defined(_WIN32)
#include <winsock2.h>
typedef int socklen_t;
#else
#error OS Not supported
#endif

#include "clog.h"
#include "shell.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 256
#endif

#ifndef CON_MAX_ATTEMPTS
#define CON_MAX_ATTEMPTS 5
#endif

class Node {

protected:
	#if defined(__linux__) || defined(__APPLE__)
	int sockfd = 0;
	int clients[100];
	#elif defined(_WIN32)
	SOCKET sockfd;
	SOCKET clients[100];
	#endif
	const char *sh_process(int, job, const char *, int);
public:
	Node(void);
	Node(int);

	~Node(void);

	int writeln(char *);
	char *readln(void);
	int write_data(void *, int);
	void *read_data();
	int write_file(char *, char *);
	int read_file(char *, char *);

	int start_server(int);
	int connect_server(const char *, int);

	Node accept(int);
	Node connect(const char *, int);

	const char *process(int, job, char *);
	//int disconnect_al(void);
};

Node::Node(){
#if defined(_WIN32)
			WSADATA wsa;
    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        log_fat(_NODE_H, "Failed. Error Code : %d",WSAGetLastError());
    }
		#endif
	}

	Node::Node(int sockfd){
		this->sockfd = sockfd;
	}


Node	Node::accept(int port){
		Node n(start_server(port));
		return n;
	}

	Node::~Node(){
	#if defined(_WIN32)
		WSACleanup();
		#endif
	}
const char *Node::sh_process(int jlen, job *jobs, const char *c_line, int sockfd){
		int count = -1;
		char *line = (char *)malloc(strlen(c_line));
		strcpy(line, c_line);
		ssize_t arg_buffsize = 64; //chage variably
	    char **args = (char **)calloc(arg_buffsize, SH_BUFSIZE);

		 char *token = strtok(line, SH_TOK_DELIM);
	    while(token != NULL){

			args[++count] = token;
			if(count == arg_buffsize){
				arg_buffsize += 64;//change variably
				args = (char **)realloc(args, sizeof(char) * arg_buffsize);
			}
			token = strtok(NULL, SH_TOK_DELIM);
	    }
		for (int i = 0; i < jlen; i++) {
	        	if (strcmp(*args, jobs[i].command) == 0) {
	            		return ((jobs[i].function)(count, ++args, sockfd)).msg;
	        	}
		}
		printf("you clearly need 'help'\n");
	    return "\0";//FIXME add custom codes to identify returns and errors
	}

const char *Node::process(int jlen, job *jobs, char *cmd){
	return Node::sh_process(jlen, jobs, cmd, sockfd);
}

int Node::writeln(char *buf, int len=-1){
	if(len == -1)len=strlen(buf);
	int bwrite = write(sockfd, buf, len);
	//log_inf("CLIENT", "written buf: %s, sent: %d", buf, bwrite);
	if(bwrite > 0){
		if(bwrite < len){
			return writeln(sockfd, buf+bwrite, len - bwrite);
		}
		if(bwrite == len){
			write(sockfd, "\n", 1);
			//log_inf("CLIENT", "write sucessful");
			return 0;
		}
	}
	else if(bwrite == 0){
		log_inf("CLIENT", "Unspecified write error");
		return -1;
	}
	else if(bwrite < 0){
		log_inf("CLIENT", "write error, exiting...");
		return -1;
	}
}

char *Node::readln(){
		char *buf = (char *)malloc(256);
		int ptr = 0;
		memset(buf, '\0', 256);
		int quit=0;
		for(ptr=0;quit!=1;ptr++){
			int bread = read(sockfd, buf+ptr, 1);
			if(bread > 0){
				//log_inf("SERVER", "Content read[%d]: %c", ptr, buf[ptr]);
				if(buf[ptr] == '\n'){
					buf[ptr] = '\0';
					quit = 1;
				}
			}
			else if(bread == 0){//EOF hit, client disconnected
				log_inf("SERVER", "EOF hit");
				return NULL;
			}
			else if(bread < 0){ //read error
				log_inf("SERVER", "read error exiting...");
				return NULL;
			}
		}

Node Node::connect(const char *hostname, int port){
	Node n(connect_server(hostname, port));
	return n;
}
int Node::connect_server(const char * hostname, int port){
	struct sockaddr_in serv_addr;
	struct hostent *server;
#if defined(__linux__) || defined(__APPLE__)
	int sockfd;
#elif defined(_WIN32)
	SOCKET sockfd;
	WSAData wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	if(LOBYTE(wsadata) != 2 || HIBYTE(wsadata) != 2){
		log_fat(_NODE, "Cannot initialize Winsock2 library, error code: %d", WSAGetLastError());
		return -1;
	}
#endif

	//checking whether port is between 0 and 65536
	if (port < 0 || port > 65535){
		log_err (_NODE, "invalid port number, port number should be between 0 and 65536");
		return -1;
	}
	//Create socket
	sockfd = socket(AF_INET , SOCK_STREAM , 0);
	if (sockfd == INVALID_SOCKET){
		log_err(_NODE, "Could not create socket");
		return -1;
	}
	log_inf(_NODE, "Socket created");
	if((server = gethostbyname(hostname))==NULL){
		log_err(_NODE, "no such host found");
		return -1;
	}
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
	serv_addr.sin_port = htons( port );
	int i = 0;
	while (connect (sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1){
		if(i++ > CON_MAX_ATTEMPTS){
			//guess other hostnames for the user
			close(sockfd);
			log_err(_NODE, "cannot establish connection to %s on port %d", hostname, port);
			return NULL;
		}
	}
	log_inf(_NODE, "connection established successfully to %s on port %d", hostname, port);
	return Node()
}

#ifndef SERV_BACKLOG
#define SERV_BACKLOG 10
#endif


//TODO support multiple server options
/** Starts the server with the standard IPv4 and TCP stack
 * @param port Port number for the server to start
 * @return Socket descriptor of the started server
 */
int Node::start_server(int port){

	static int cont;
#if defined(__linux__) || defined(__APPLE__)
	static int servfd;
#elif defined(_WIN32)
	static SOCKET servfd;
	WSAData wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	if(LOBYTE(wsadata) != 2 || HIBYTE(wsadata) != 2){
		log_fat(_NODE, "Cannot initialize Winsock2 library, error code: %d", WSAGetLastError());
		return -1;
	}
#endif

	struct sockaddr_in server, client;
    	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( port );
	socklen_t cli_size = sizeof(struct sockaddr_in);

	if(cont == port){
		log_inf(_NODE, "Connection accepted");
		return accept(servfd, (struct sockaddr *)&client, &cli_size);
	}
	if(cont == 0)
		cont = port;
	//Create socket
	servfd = socket(PF_INET , SOCK_STREAM , 0);
	if (servfd == INVALID_SOCKET){
		log_err(_NODE, "could not create socket");
		return -1;
	}
	//Bind
	if( bind(servfd,(struct sockaddr *)&server , sizeof(server)) < 0){
		log_err(_NODE, "bind failed");
		return -1;
	}
	//Listen
	listen(servfd , SERV_BACKLOG);
	//Accept and incoming connection
	log_inf(_NODE, "Waiting for incoming connections...");
	//accept connection from an incoming client
	int clifd = accept(servfd, (struct sockaddr *)&client, &cli_size);
	if (clifd < 0){
		log_inf(_NODE, "Accept failed");
#if defined(__linux__) || defined(__APPLE__)
		close(servfd);
#elif defined(_WIN32)
		closesocket(servfd);
#endif
		return -1;
	}
	log_inf(_NODE, "Connection accepted");
	return clifd;
}
#endif
