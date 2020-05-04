    
/**
 * A simple HTTP client in IPv4 or IPv6 internet domain using 
 * TCP to send messages to a server and receive the responses. 
 * The hostname and port number are passed as arguments.
 * @author Tong (Debby) Ding
 * @version 1.0
 * @see CPSC 5510 Spring 2020, Seattle University
 */
#include <netdb.h>
// #include <unistd.h>
// #include <cstring>

#include <stdio.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>


#define PORT 80
#define BUF_SIZE 0xfff


/**
 * Print an error message and exit the program
 * @param msg customized error message
 */ 
void checkErr(const char *msg) {
    fprintf(stderr, "Error: %s", msg);
    exit(1);
}

/**
 * Send messages to server through TCP socket and print the response
 * @param argc number of arguments entered by user
 * @param argv[] the content of arguments
 */ 
int main(int argc, char *argv[]) {
  
    char *host, *port = PORT; // server URI, HTTP port

    char request[BUF_SIZE], response[BUF_SIZE];
    char *requestLine = "GET / HTTP/1.0\r\n";
    char *headerFmt = "Host: %s\r\n";
    char *CRLF = "\r\n";

    int sockfd, err; // socket file descriptor and error status
    struct addrinfo hints, *sev_addr; // getaddrinfo() result
    // int addrStatus; // getaddrinfo status

    // check user input
    if (argc < 2)
        checkErr("user input");
    
    // FIX-ME if (argc )

    host = argv[1];

    // header buffer
    size_t bufLen = strlen(headerFmt) + strlen(host) + 1; // plus 1 for '\0'
    char *buffer = (char *)malloc(bufLen);

    // construct request
    strcpy(request, requestLine);
    snprintf(buffer, bufLen, headerFmt, host);
    strcat(request, buffer);
    strcat(request, CRLF);

    // get IP
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // use TCP
    hints.ai_flags = AI_NUMERICSERV; // treat port as number

    err = getaddrinfo(host, port, &hints, &sev_addr);
    if (err != 0)
        checkErr((char *)gai_strerror(err));

    // construct socket
    sockfd = socket(sev_addr->ai_family, sev_addr->ai_socktype, 0);

    // connect server
    err = connect(sockfd, sev_addr->ai_addr, sev_addr->ai_addrlen);
    if (err < 0)
        checkErr("connect server");
    
    printf("----------\nRequest:\n----------\n%s\n", request);

    // send request
    err = send(sockfd, request, strlen(request), 0);
    if (err < 0)
        checkErr("send request");
    
    // recv response
    err = recv(sockfd, response, BUF_SIZE, 0);
    if (err < 0)
        checkErr("receive response");

    printf("----------\nResponse:\n----------\n%s\n", response);

    // free ptr memory
    free(buffer);
    buffer = NULL;
    freeaddrinfo(sev_addr);
    sev_addr = NULL;

    close(sockfd);
    
    return 0;
}   