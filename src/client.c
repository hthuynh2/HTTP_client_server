/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: c\n");
	    exit(1);
	}

    
    char cmd[MAX_STR_LEN];
    memcpy(cmd, &argv[1], strlen(argv[1]));
    char host_name[MAX_STR_LEN] ;
    char port[MAX_STR_LEN];
    char path[MAX_STR_LEN];
    int i = 0;
    int idx = 0 ;
    int flag = 0;
    int host_name_idx = 0;
    int port_idx = 0;
    int path_idx = 0;
    for(i = 0 ; i < strlen(cmd); i++){
        if(flag <2){          // "http://"
            char c = cmd[i];
            if(cmd[i] == '/')
                flag ++;
        }
        else if(flag == 2){     // "host name & port"
            if(cmd[i] == ':'){
                flag++;
                continue;
            }
            if(cmd[i] == '/'){
                flag += 2;
                continue;
            }
            host_name[host_name_idx] = cmd[i];
            host_name_idx++;
        }
        else if(flag == 3){     //port
            if(cmd[i] == '/'){
                flag++;
                continue;
            }
            port[port_idx] = cmd[i];
            port_idx++;
        }
        else if (flag == 4){    //file path
            path[path_idx] = cmd[i];
            path_idx++;
        }
    }
    
    if(flag!= 4){
        fprintf(stderr,"usage: c\n");
        exit(1);
    }
    
    
    
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    if(port_idx == 0){
        if ((rv = getaddrinfo(host_name, PORT, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }
    }
    else{
        if ((rv = getaddrinfo(host_name, port, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }
    }


	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

    
    //
    send(sockfd, path, strlen(path), 0);
    
    //
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n",buf);

	close(sockfd);

	return 0;
}












