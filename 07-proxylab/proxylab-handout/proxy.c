#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "csapp.h"
#include "sbuf.h"
#include "cache.h"


#define THREADS 8
#define SBUFSIZE 32
#define CACHE_NUM 10
/* max line of request content */
#define MAX_CONTENT 128

//#define DEBUG

#ifdef DEBUG
#define PRINTLOG(...) printf(__VA_ARGS__)
#else
#define PRINTLOG(...)
#endif

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *my_version = "HTTP/1.0";

void *thread(void *vargp);
void doit(int fd);
int transform_request(rio_t *rp, char *content, char*host, char*port, char*path);
void parse_url(char *url, char*host, char*port, char*path);
int read_all(rio_t *rp, void *content, size_t *length);
void proxy_error(int fd);

sbuf_t sbuf;
cache_t cache;


int main(int argc, char * argv[])
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    // igonre SIGPIPE
    Signal(SIGPIPE, SIG_IGN);
    //sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);

    if (argc != 2){
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 0;
    }
    PRINTLOG("Port: %s\n", argv[1]);

    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE);
    cache_init(&cache, CACHE_NUM);
    for (int i=0;i<THREADS; i++){
        Pthread_create(&tid, NULL, thread, NULL);
    }

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
        sbuf_add(&sbuf, connfd);
        PRINTLOG("Accepting connection from (%s, %s)\n", hostname, port);
        //doit(connfd);

        // if(close(connfd)<0){
        //     PRINTLOG("Close client connection failed.\n");
        // }
    }
    
    subf_destory(&sbuf);
    cache_destory(&cache);
    return 0;
}


void *thread(void *vargp){
    Pthread_detach(pthread_self());
    while(1){
        PRINTLOG("Client connection allocated.\n");
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
        close(connfd);
        PRINTLOG("Client connection closed.\n");
    }
}

void doit(int fd){
    char host[MAXLINE], port[MAXLINE], path[MAXLINE], finger[MAXLINE];
    char request_content[MAX_CONTENT * MAXLINE], response_content[MAX_OBJECT_SIZE];
    rio_t rio, lc_rio;
    int local_client_fd;
    size_t n, total_size;

    Rio_readinitb(&rio, fd);

    // read until eof.
    if(transform_request(&rio, request_content, host, port, path) <= 0){
        proxy_error(fd);
        return;
    }
    PRINTLOG("Request info: %s %s %s\n", host, port, path);
    sprintf(finger, "%s %s %s", host, port, path);
    
    // try to get the content from cache.
    PRINTLOG("Searching cache...\n");
    if(get_obj(&cache, finger, response_content, &total_size)>=0){
        PRINTLOG("Cache hit!\n");
        if(rio_writen(fd, response_content, total_size) == total_size) 
            PRINTLOG("Finish this request by cache.\n");
        else PRINTLOG("Error happen while writing back to client.\n");

        return;
    }

    PRINTLOG("Cache miss\n");
    if((local_client_fd = open_clientfd(host, port)) <= 0){
        PRINTLOG("Open remote socket failed.\n");
        proxy_error(fd);
        return;
    }
    rio_readinitb(&lc_rio, local_client_fd);
    PRINTLOG("Sending Request...\n");
    if(rio_writen(local_client_fd, request_content, strlen(request_content))!=strlen(request_content)){
        proxy_error(fd);
        close(local_client_fd);
        return;
    }
    PRINTLOG("Request sent.\n");

    total_size = 0;
    while((n = rio_readnb(&lc_rio, response_content, MAX_OBJECT_SIZE))>0){
        PRINTLOG("Received %.3f MiB.\n", n/1024.0);   
        if (rio_writen(fd, response_content, n) != n){
            PRINTLOG("Error happen while writing back to client.\n");
            close(local_client_fd);
            return;
        }
        PRINTLOG("Send to client %.3f MiB.\n", n/1024.0);   
        total_size += n;
    }

    if (total_size <= MAX_OBJECT_SIZE){
        // cache this content
        PRINTLOG("Saving cache...\n");
        store_obj(&cache, finger, response_content, total_size);
        PRINTLOG("Cache saved: %s\n", finger);
    }

    //Close(local_client_fd);
    PRINTLOG("Finished a request.\n");

}


void doit_multi(int fd){
    char host[MAXLINE], port[MAXLINE], path[MAXLINE],
     last_host[MAXLINE]="\0", last_port[MAXLINE]="\0";
    char request_content[MAX_CONTENT * MAXLINE], response_content[MAX_OBJECT_SIZE];
    rio_t rio, lc_rio;
    int local_client_fd, last_local_client_fd = -1;
    size_t n, total_size;

    Rio_readinitb(&rio, fd);

    // read until eof.
    while(transform_request(&rio, request_content, host, port, path) > 0){
        // PRINTLOG("---- NEW REQUEST ----\n");

        PRINTLOG("Request info: %s %s %s\n", host, port, path);

        // reuse the connection
        if(!strcmp(host, last_host) && !strcmp(port, last_port)){
            PRINTLOG("Resuse Connestion %s:%s", host, port);
            local_client_fd = last_local_client_fd;
        }
        else{
            close(last_local_client_fd);
            last_local_client_fd = local_client_fd;
            strcpy(last_host, host);
            strcpy(last_port, port);
            if((local_client_fd = open_clientfd(host, port)) <= 0){
                PRINTLOG("Open remote socket failed.\n");
                proxy_error(fd);
                break;
            }
        }

  
        PRINTLOG("Sending...\n");
        // PRINTLOG("%s", request_content);
        if(rio_writen(local_client_fd, request_content, strlen(request_content))!=strlen(request_content)){
            proxy_error(fd);
            break;
        }
        PRINTLOG("Request sent.\n");

        rio_readinitb(&lc_rio, local_client_fd);
        total_size = 0;
        while((n = rio_readnb(&lc_rio, response_content, MAX_OBJECT_SIZE))>0){
            PRINTLOG("Received %zu byte(s).\n", n);   
            if (rio_writen(fd, response_content, n) < 0){
                break;
            }
            total_size += n;
        }

        //Close(local_client_fd);
        PRINTLOG("Finished a request.\n");
    }
    close(local_client_fd);
}


int transform_request(rio_t *rp, char *content, char*host, char*port, char*path){
    char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];
    int contain_host = 0;

    if(rio_readlineb(rp, buf, MAXLINE) <= 0) return -1;
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
        if (strncmp("Connection:", buf, strlen("Connection:")) == 0) continue;
        if (strncmp("Host:", buf, strlen("Host:")) == 0) contain_host = 1;
        strcpy(content, buf);
        content += strlen(buf);
        if(!strcmp(buf, "\r\n")) break;
    }

    if(!contain_host){
        sprintf(buf, "Host: %s\r\n", host);
        strcpy(content, buf);
        content += strlen(buf);
    }

    sprintf(buf, "Connection: close\r\nProxy-Connection: close\r\n");
    strcpy(content, buf);
    content += strlen(buf);

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

void proxy_error(int fd){
    char buf[MAXLINE];
    sprintf(buf, "%s %s\r\n\r\n",my_version ,"502 Bad-Gateway");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<h1>502 Bad-Gateway<h1>\r\n");
}