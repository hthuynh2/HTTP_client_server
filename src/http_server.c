/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define RESPONSE_CODE_404 "HTTP/1.1 404 Not Found\r\n\r\n"
#define RESPONSE_CODE_404_SIZE 26
#define RESPONSE_CODE_400 "HTTP/1.1 400 Bad Request\r\n\r\n"
#define RESPONSE_CODE_400_SIZE 28
#define RESPONSE_CODE_200 "HTTP/1.1 200 OK\r\n\r\n"
#define RESPONSE_CODE_200_SIZE 19


#define BACKLOG 10	 // how many pending connections queue will hold

#define MAX_STR_LEN 1024

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, const char* argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
    
    char port[MAX_STR_LEN];
    
    //If user does not specify port, use port 80 as default
    if(argc == 2){
        memcpy(port, argv[1], strlen(argv[1]));
        port[strlen(argv[1])] = '\0';
    }
    else{
        port[0] = '8';
        port[1] = '0';
        port[2] = '\0';
    }
    
    
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		printf("server: got connection from %s\n", s);
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
            char buf[MAX_STR_LEN];
            
            //get request msg from client
            recv(new_fd, buf, MAX_STR_LEN, 0);
            char path[MAX_STR_LEN];
            int path_idx = 0;
            
            //Extract path from msg
            for(int i = 0 ; i < strlen(buf); i++){
                if(buf[i] != '/'){
                    continue;
                }
                else{
                    i++;
                    while(buf[i] != ' ' && i < MAX_STR_LEN){
                        path[path_idx] = buf[i];
                        path_idx++;
                        i++;
                    }
                    path[path_idx] = '\0';
                    break;
                }
            }
            //Open file
            FILE* fp = fopen(path, "r");
            
            if(fp == NULL){
                //File not found. Send back error response code
                if (send(new_fd, RESPONSE_CODE_404, RESPONSE_CODE_404_SIZE, 0) == -1)
                    perror("send");
                close(new_fd);
                exit(0);
            }
            else{
                //Send back OK response code
                if (send(new_fd, RESPONSE_CODE_200, RESPONSE_CODE_200_SIZE, 0) == -1){
                    perror("send");
                    fclose(fp);
                    close(new_fd);
                    exit(0);
                }
                //Read from file and send data
                int numbytes;
                while((numbytes = fread(buf, 1, MAX_STR_LEN, fp)) != 0){
                    if (send(new_fd, buf, numbytes, 0) == -1){
                        perror("send");
                        fclose(fp);
                        close(new_fd);
                        exit(0);
                    }
                }
            }
            close(new_fd);
            exit(0);
		}
        close(new_fd);  // parent doesn't need this
	}

	return 0;
}








