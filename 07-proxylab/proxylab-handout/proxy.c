#include <stdio.h>
#include <stdlib.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* max line of request content */
#define MAX_CONTENT 128

#define DEBUG

#ifdef DEBUG
#define PRINTLOG(...) printf(__VA_ARGS__)
#else
#define PRINTLOG()
#endif

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *my_version = "HTTP/1.0";

void doit(int fd);
int transform_request(rio_t *rp, char *content, char*host, char*port, char*path);
void parse_url(char *url, char*host, char*port, char*path);
int read_all(rio_t *rp, void *content, size_t *length);


int main(int argc, char * argv[])
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2){
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 0;
    }
    PRINTLOG("Port: %s\n", argv[1]);

    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        if((connfd = accept(listenfd, (SA *)&clientaddr, &clientlen)) < 0){
            PRINTLOG("Accept failed.\n");
            continue;
        }
        if(getnameinfo((SA *) &clientaddr, clientlen, 
        hostname, MAXLINE, port, MAXLINE, 0)!=0){
            PRINTLOG("getnameinfo failed.\n");
            continue;
        }
        PRINTLOG("Accepted connection from (%s, %s)\n", hostname, port);

        doit(connfd);

        if(close(connfd)<0){
            PRINTLOG("Close client connection failed.\n");
        }
    }
    
    return 0;
}

void doit(int fd){
    char host[MAXLINE], port[MAXLINE], path[MAXLINE];
    char request_content[MAX_CONTENT * MAXLINE], response_content[MAX_OBJECT_SIZE];
    rio_t rio, lc_rio;
    int local_client_fd;
    size_t n, total_size;

    Rio_readinitb(&rio, fd);

    // read until eof.
    while(transform_request(&rio, request_content, host, port, path) > 0){
        PRINTLOG("---- NEW REQUEST ----\n");

        PRINTLOG("Request info: %s %s %s\n", host, port, path);
        if((local_client_fd = Open_clientfd(host, port)) <= 0){
            PRINTLOG("Open remote socket failed.\n");
            continue;
        }
  
        PRINTLOG("Sending...\n");
        // PRINTLOG("%s", request_content);
        Rio_writen(local_client_fd, request_content, strlen(request_content));
        PRINTLOG("Request sent.\n");

        Rio_readinitb(&lc_rio, local_client_fd);
        total_size = 0;
        while((n = Rio_readnb(&lc_rio, response_content, MAX_OBJECT_SIZE))>0){
            PRINTLOG("Received %zu byte(s).\n", n);   
            Rio_writen(fd, response_content, n);
            total_size += n;
        }

        Close(local_client_fd);
        PRINTLOG("Server Connection closed.\n");
    }
    return;
}

int read_all(rio_t *rp, void *content, size_t *length){
    *length = 0;
    //ssize_t r;
    return rio_readnb(rp, content, MAXLINE*MAX_CONTENT);
    // while(1){
    //     r = rio_readnb(rp, content, MAXLINE);
    //     if(r<0) return r;
    //     else{
    //         *length += r;
    //         if(r < MAXLINE) break;
    //     }
    // }
}


int transform_request(rio_t *rp, char *content, char*host, char*port, char*path){
    char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];

    if(Rio_readlineb(rp, buf, MAXLINE) <= 0) return -1;
    PRINTLOG("Origin Header: %s", buf);

    sscanf(buf, "%s %s %s", method, url, version);
    if(strcasecmp(method, "GET")){
        PRINTLOG("Unsupported method: %s", method);
        return -1;
    }
    parse_url(url, host, port, path);
    PRINTLOG("Parse URL: %s %s %s\n", host, port, path);

    //int i = 0;
    sprintf(buf, "%s %s %s\r\n", method, path, my_version);
    strcpy(content, buf);
    content += strlen(buf);
    strcpy(content, user_agent_hdr);
    content += strlen(user_agent_hdr);
    while(Rio_readlineb(rp, buf, MAXLINE) > 0){
        if (strncmp("Proxy-Connection:", buf, strlen("Proxy-Connection:")) == 0) continue;
        if (strncmp("Keep-Alive:", buf, strlen("Proxy-Connection:")) == 0) continue;
        strcpy(content, buf);
        content += strlen(buf);
        if(!strcmp(buf, "\r\n")) break;
    }
    *content = '\0';

    return 1;
}

void parse_url(char *url, char*host, char*port, char*path){
    char *host_start, *port_start, *path_start, buf[MAXLINE];

    strcpy(buf, url);
    // remove '\r'
    //*(buf + strlen(buf) -1) = '\0';

    host_start = strstr(buf, "//");
    if(host_start == NULL) host_start = buf;
    else host_start += 2;

    port_start = strstr(host_start, ":");
    path_start = strstr(host_start, "/");

    if (path_start == NULL){
        strcpy(path, "/");
        path_start = buf + strlen(buf);
    }
    else strcpy(path, path_start);
    
    *path_start = '\0';

    if (port_start == NULL){
        strcpy(port, "80");
        port_start = buf + strlen(buf);
    }
    else strcpy(port, port_start+1);

    *port_start = '\0';
    strcpy(host, host_start);

    return;
}