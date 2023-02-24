#include "csapp.h"
#include "tiny/csapp.h"
#include <stdio.h>
#include <string.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int readcnt;
sem_t mutex, w;
void doit(int fd);
void read_requesthdrs(rio_t *rp);
void parse_uri(char *uri, char *hostname, char *filename, char *port);
void send_to_server(int fd, char *hostname, char *filename);
int send_from_cache(char *uri, int client_fd);
void insert_into_cache(char* uri, char* data, int len);
void forward(rio_t *server_rio, int client_fd, char *uri);
void clienterror(int fd, char *cause, char *errnum,
		 char *shortmsg, char *longmsg);
void *thread(void *vargp);

int main(int argc, char **argv) {
    int listenfd, *connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    Sem_init(&mutex, 0, 1);
    Sem_init(&w, 0, 1);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname,
            MAXLINE, port, MAXLINE, 0);
        printf("Accepted connect from (%s, %s)\n",hostname, port);
        Pthread_create(&tid, NULL, thread, connfdp);
    }
    return 0;
}

void *thread(void *vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}

void doit(int fd) {
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], hostname[MAXLINE], port[MAXLINE];
    rio_t client_rio, server_rio;

    /* Read request line and headers */
    Rio_readinitb(&client_rio, fd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE))
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return;
    }
    read_requesthdrs(&client_rio);

    if (!send_from_cache(uri, fd)) {
        parse_uri(uri, hostname, filename, port);

        int server_fd = Open_clientfd(hostname, port);
        Rio_readinitb(&server_rio, server_fd);
        send_to_server(server_fd, hostname, filename);
        forward(&server_rio, fd, uri);
        Close(server_fd);
    }
}

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */


void parse_uri(char *uri, char *hostname, char *filename, char *port) {
    char uri_copy[MAXLINE];
    strcpy(uri_copy, uri);
    uri = uri_copy;
    char *ptr;
    if ((ptr = strstr(uri, "//")) != NULL)
        uri = ptr + 2;
    ptr = index(uri, '/');
    strcpy(filename, ptr);
    *ptr = '\0';
    ptr = index(uri, ':');
    if (ptr == NULL) {
        strcpy(hostname, uri);
        strcpy(port, "80");
    } else {
        *ptr = '\0';
        strcpy(hostname, uri);
        strcpy(port, ptr + 1);
    }
}

void send_to_server(int fd, char *hostname, char *filename) {
    char buf[MAXLINE];
    sprintf(buf, "GET %s HTTP/1.0\r\n", filename);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Host: %s\r\n", hostname);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s", user_agent_hdr);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Connection: close\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Proxy-Connection: close\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));
}

void forward(rio_t *server_rio, int client_fd, char *uri) {
    char buf[MAXLINE], cache[MAX_OBJECT_SIZE];
    int sz = 0;
    ssize_t len;
    while ((len = Rio_readlineb(server_rio, buf, MAXLINE)) != 0) {
        Rio_writen(client_fd, buf, len);
        if (sz + len <= MAX_OBJECT_SIZE)
            memcpy(cache + sz, buf, len);
        sz += len;
    }
    if (sz <= MAX_OBJECT_SIZE)
        insert_into_cache(uri, cache, sz);
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */

/* Cache */
#define BLOCK_N (MAX_CACHE_SIZE / MAX_OBJECT_SIZE)
char cache[BLOCK_N][MAX_OBJECT_SIZE], uris[BLOCK_N][MAXLINE];
int lens[BLOCK_N], n;
int send_from_cache(char *uri, int client_fd) {
    P(&mutex);
    readcnt++;
    if (readcnt == 1)
        P(&w);
    V(&mutex);

    for (int i = 0; i < n; i++) {
        if (!strcmp(uris[i], uri)) {
            Rio_writen(client_fd, cache[i], lens[i]);
            return 1;
        }
    }

    P(&mutex);
    readcnt--;
    if (readcnt == 0)
        V(&w);
    V(&mutex);
    return 0;
}

void insert_into_cache(char* uri, char* data, int len) {
    P(&w);

    int pos;
    if (n < BLOCK_N) pos = n++;
    else pos = len % n;
    lens[pos] = len;
    memcpy(cache[pos], data, len);
    strcpy(uris[pos], uri);

    V(&w);
}
