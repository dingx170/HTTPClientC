/**
 * A simple HTTP client sends a request to a server then prints 
 * out the request hearder and response hearder, and either 
 * prints out the response body or saves it in a given file
 * @author Tong (Debby) Ding
 * @version 1.0
 * @see CPSC 5510 Spring 2020, Seattle University
 */
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#define PORT_DEF 80
#define BUF_SIZE 0xfff
#define ARR_SIZE 100

/**
 * Print an error message and exit the program
 * @param msg customized error message
 */ 
void checkErr(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

/**
 * Send a request to a server and print out both request and response
 * @param argc number of arguments entered by user
 * @param argv[] the content of arguments
 */ 
int main(int argc, char *argv[]) {
    // server URI, HTTP port
    char *port_num, *path_adr;
    char host[ARR_SIZE], path[ARR_SIZE]; 
    int port = PORT_DEF;

    // flags
    bool url_parsed = false, addr_connected = false, is_text = false;

    // header info
    const char *requestLineFmt = "GET /%s HTTP/1.1\r\n";
    const char *headerFmt = "Host: %s\r\n";
    const char *connFmt = "Connection: %s\r\n";
    const char *CRLF = "\r\n";

    // socket & msg related
    int sockfd, err, bytes;
    struct addrinfo hints, *svr_addr, *addr; // getaddrinfo() result

    // request & response holder
    char request[BUF_SIZE], response[BUF_SIZE];
    char header_buf[BUF_SIZE];
    char *buffer1, *buffer2, *buffer3;
    size_t bufLen;
    char *header_end, *ctnt_type_start, *ctnt_type_end;
    char header[BUF_SIZE], body[BUF_SIZE], ctnt_type[ARR_SIZE];
    int len, tmplen;

    // check user input
    if (argc < 2)
        checkErr("user input");
    
    // parse URL
    if (sscanf(argv[1], "http://%99[^:]:%99d/%99[^\n]", host, &port, path) == 3) 
        url_parsed = true;
    else if (sscanf(argv[1], "http://%99[^/]/%99[^\n]", host, path) == 2) 
        url_parsed = true;
    else if (sscanf(argv[1], "http://%99[^:]:%99d[^\n]", host, &port) == 2) 
        url_parsed = true;
    else if (sscanf(argv[1], "http://%99[^\n]", host) == 1) 
        url_parsed = true;

    if (url_parsed == false)
        checkErr("parse URL");

    // get port num and path
    asprintf(&port_num, "%d", port);
    asprintf(&path_adr, "%s", path);
    
    // get IP
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6
    hints.ai_flags = AI_NUMERICSERV; // treat port as number
    hints.ai_protocol = 0; 

    err = getaddrinfo(host, port_num, &hints, &svr_addr);
    if (err != 0)
        checkErr((char *)gai_strerror(err));
    
    // construct request
    strcpy(request, "");
    // request line
    bufLen = strlen(requestLineFmt) + strlen(path_adr) + 1; // plus 1 for '\0'
    buffer1 = (char *)malloc(bufLen);
    snprintf(buffer1, bufLen, requestLineFmt, path_adr);
    // host
    bufLen = strlen(headerFmt) + strlen(host) + 1;
    buffer2 = (char *)malloc(bufLen);
    snprintf(buffer2, bufLen, headerFmt, host);
    // connection
    bufLen = strlen(connFmt) + strlen("close") + 1;
    buffer3 = (char *)malloc(bufLen);
    snprintf(buffer3, bufLen, connFmt, "close");
    
    strcat(request, buffer1);
    strcat(request, buffer2);
    strcat(request, buffer3);
    strcat(request, CRLF);

    // send request, loop breaks if 1 addr works
    for (addr = svr_addr; addr != NULL; addr = addr->ai_next) {

        // construct socket
        sockfd = socket(addr->ai_family, addr->ai_socktype, 0);

        // connect server
        err = connect(sockfd, svr_addr->ai_addr, svr_addr->ai_addrlen);
        if (err < 0)
            checkErr("connect server");
        else
            addr_connected = true;
        
        fprintf(stderr, "--------------\nRequest:\n--------------\n%s\n", request);

        // send request
        err = send(sockfd, request, strlen(request), 0);
        if (err < 0)
            checkErr("send request");
        
        // recv the first response to get header
        err = recv(sockfd, header_buf, BUF_SIZE, 0);
        if (err < 0)
            checkErr("receive response");

        // get header
        header_end = strstr(header_buf, "\r\n\r\n");
        len = header_end - header_buf;
        strncpy(header, header_buf, len); // memcpy(tmp, header_buf, len);

        // get initial part of body
        len = BUF_SIZE - len - 4;     
        tmplen = len;   
        strncpy(body, header_end + sizeof("\r\n\r\n") - 1, len);

        // check content length, may be useful later
        // char ctnt_len[ARR_SIZE];
        // if (strstr(header_buf, "Content-Length:") != nullptr) {
        //     char *ctnt_len_start = strstr(header_buf, "Content-Length:") + sizeof("Content-Length:");
        //     char *ctnt_len_end = strstr(ctnt_len_start, "\n");
        //     len = ctnt_len_end - ctnt_len_start;
        //     strncpy(ctnt_len, ctnt_len_start, len); 
        // }

        // get content type
        ctnt_type_start = strstr(header_buf, "Content-Type:") + sizeof("Content-Type:");
        ctnt_type_end = strstr(ctnt_type_start, "\n");
        len = ctnt_type_end - ctnt_type_start;
        strncpy(ctnt_type, ctnt_type_start, len); 

        fprintf(stderr, "--------------\nResponse:\n--------------\n%s\n", header);

        // adjust print aproach based on content type
        if (strstr(ctnt_type, "text")) {
            printf(body);
            is_text = true;
        } else {
            write(fileno(stdout), header_end + sizeof("\r\n\r\n") - 1, tmplen);
        }

        // continue content receiving
        do {
            bytes = recv(sockfd, response, BUF_SIZE, 0);
            if (bytes < 0)
                checkErr("ERROR reading response from socket");

            if (is_text == true)
                printf(response);
            else
                write(fileno(stdout), response, bytes);
                
            memset(response, 0, BUF_SIZE);
        } while (bytes != 0);

        if (addr_connected)
            break;

    }
    // free ptr memory
    free(buffer1);
    buffer1 = NULL;
    free(buffer2);
    buffer2 = NULL;
    free(buffer3);
    buffer3 = NULL;

    freeaddrinfo(svr_addr);
    svr_addr = NULL;

    close(sockfd);
    
    return 0;
}   