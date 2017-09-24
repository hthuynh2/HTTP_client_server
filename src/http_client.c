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

//#define PORT "3490" // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once
#define MAX_STR_LEN 1024
#define PORT  "3490"

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
    char buf[MAX_STR_LEN];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    
    
    if (argc != 2) {
        fprintf(stderr,"Usage: ./http_client <url>\n");
        exit(1);
    }
    
    
    char cmd[MAX_STR_LEN];
    memcpy(cmd, argv[1], strlen(argv[1]));
    char host_name[MAX_STR_LEN] ;
    char port[MAX_STR_LEN] = "80";

    char path[MAX_STR_LEN];
    int i = 0;
    int flag = 0;
    int host_name_idx = 0;
    int port_idx = 0;
    int path_idx = 0;
    for(i = 0 ; i < strlen(cmd); i++){
        if(flag <2){          // "http://"
            if(cmd[i] == '/')
                flag ++;
        }
        else if(flag == 2){     // host name
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
    host_name[host_name_idx] = '\0';
    path[path_idx] = '\0';

    if(port_idx!=0){
        port[port_idx] = '\0';
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((rv = getaddrinfo(host_name, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
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
    
    strncpy(buf, "GET /", 5);
    strncpy((char*)(buf + 5), path, strlen(path));
    strncpy((char*)(buf + 5+ strlen(path)), " HTTP/1.1\r\n\0", 11);
    
    if (send(sockfd, buf, strlen(buf), 0) == -1){
        perror("send");
        close(sockfd);
        exit(0);
    }
    
    memset(buf, 0 , MAX_STR_LEN);
    if((numbytes = recv(sockfd, buf, MAX_STR_LEN, 0)) == -1){
        perror("receive");
        close(sockfd);
        exit(0);
    }
       
    buf[numbytes] = '\0';
    printf("%s\n",buf);
    
    FILE * fp = fopen("output", "w");
    while((numbytes = recv(sockfd, buf, MAX_STR_LEN, 0)) != 0){
        fwrite(buf, sizeof(char), numbytes, fp);
    }
    close(sockfd);
    
    return 0;
}

