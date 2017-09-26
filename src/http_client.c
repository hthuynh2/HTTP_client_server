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

/* This function read one line from the socketfd
 *Argument:     fd: fd to read from
 *              line: pointer to buffer to store data
 *Return:       Number of byte read
 */
int getline_(int fd, char* line)
{
    char c = '\0';
    char buf[MAX_STR_LEN];
    int buf_idx = 0;
    int flag = 0;
    while(read(fd,&c,1) != 0){
        buf[buf_idx] = c;
        buf_idx++;
        if(c == '\r'){
            flag = 1;
        }
        else if(c == '\n'){
            if(flag == 1){
                break;
            }
            else{
                flag = 0;
            }
        }
    }
    memcpy(line,buf,buf_idx);
    return buf_idx;
}

int main(int argc, char *argv[])
{
    int sockfd;
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
    
    //Parse user input
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
    
    //Construct the HTTP Request
    strncpy(buf, "GET /", 5);
    strncpy((char*)(buf + 5), path, strlen(path));
    strncpy((char*)(buf + 5+ strlen(path)), " HTTP/1.1\r\n", 11);
    strncpy((char*)(buf + 5+ 11+strlen(path)), "Host: ", 6);
    strncpy((char*)(buf + 5+ 11+strlen(path) + 6), host_name, strlen(host_name));
    strncpy((char*)(buf + 5+ 11+strlen(path) + 6 + strlen(host_name)), "\r\nConnection: Keep-Alive\r\n\r\n", 28);

    
    printf("%s", buf);
    
    //Send HTTP Request
    if (send(sockfd, buf, strlen(buf), 0) == -1){
        perror("send");
        close(sockfd);
        exit(0);
    }
    
    //Clear buffer
    memset(buf, 0 , MAX_STR_LEN);

    char line[MAX_STR_LEN];
    int idx = getline_(sockfd, line);
    int is_ok = 0;
    int is_chunked = 0;
    
    line[idx] = '\0';
    printf("%s", line);
    
    
    //Check if status = OK
    if(strcmp(line,"HTTP/1.1 200 OK\r\n") == 0){
        is_ok = 1;
    }

    //Read and print out Header of the response
    while(1){
        int idx = getline_(sockfd, line);
        line[idx] = '\0';
        printf("%s", line);
        
        if(strcmp(line,"Transfer-Encoding: chunked\r\n") == 0){
            is_chunked = 1;
        }

        if((line[0] == '\r' && line[1] == '\n'))
            break;
    }
    
    //If REPONSE STATUS NOT OK, dont need to write to file
    if(is_ok != 1){
        return 0;
    }
    //Open file
    FILE * fp = fopen("output", "w");
    if(fp == NULL){
        fprintf(stderr,"Cannot open file\n");
        exit(1);
    }
    
    memset(line, 0, MAX_STR_LEN);
    
    if(is_chunked){         //If Transfer-Encoding is Chunked
        while(1){
            //Read line to determine size of Chunk
            int idx  = getline_(sockfd, line);
            line[idx] = '\0';
            int chunk_size = strtol(line, NULL, 16);
            if(chunk_size == 0){
                break;
            }
            int n = 0;
            char * my_buf = malloc(chunk_size+3);
            
            //Read chunk and write to file
            while(n < chunk_size){
                int temp;
                if((temp = read(sockfd, (char*)(my_buf+n), chunk_size-n+2)) == -1){ //+2 to include "/r/n" at the end of chunk
                    perror("Cannot read\n");
                    exit(1);
                }
                n += temp;
            }
            fwrite(my_buf, sizeof(char), chunk_size, fp);
            free(my_buf);
        }
    }
    else{               //If Transfer-Encoding is not chunked
        while(1){
            int temp;
            char * my_buf = malloc(MAX_STR_LEN);
            
            //Read from socketfd
            if((temp = read(sockfd, my_buf, MAX_STR_LEN)) == -1){
                perror("Cannot read\n");
                exit(1);
            }
            //Reach EOF
            if(temp == 0)
                break;
            //Write to file
            fwrite(my_buf, sizeof(char), temp, fp);
            free(my_buf);
        }
    }
    
    fclose(fp);
    return 0;
}






