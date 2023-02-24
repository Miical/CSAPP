#include "csapp.h"
#include "tiny/csapp.h"
#include <stdio.h>
#include <string.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void parse_uri(char *uri, char *hostname, char *filename, char *port);
void send_to_server(int fd, char *hostname, char *filename);
void forward(rio_t *server_rio, int client_fd);
void clienterror(int fd, char *cause, char *errnum,
		 char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connect from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
    return 0;
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

    /* Parse URI from GET request */
    parse_uri(uri, hostname, filename, port);

    /* Send to server */
    int server_fd = Open_clientfd(hostname, port);
    Rio_readinitb(&server_rio, server_fd);
    send_to_server(server_fd, hostname, filename);
    forward(&server_rio, fd);
    Close(server_fd);


    /*
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found",
		    "Tiny couldn't find this file");
        return;
    }

    if (is_static) { // Serve static content
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
	    clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't read the file");
	    return;
	}
	serve_static(fd, filename, sbuf.st_size);
    }
    else { // Serve dynamic content
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
	    clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't run the CGI program");
	    return;
	}
	serve_dynamic(fd, filename, cgiargs);
    }
    */
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

void forward(rio_t *server_rio, int client_fd) {
    char buf[MAXLINE];
    ssize_t len;
    while ((len = Rio_readlineb(server_rio, buf, MAXLINE)) != 0)
        Rio_writen(client_fd, buf, len);
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

