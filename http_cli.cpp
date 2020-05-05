    
/**
 * A simple HTTP client in IPv4 or IPv6 internet domain using 
 * TCP to send messages to a server and receive the responses. 
 * The hostname and port number are passed as arguments.
 * @author Tong (Debby) Ding
 * @version 1.0
 * @see CPSC 5510 Spring 2020, Seattle University
 */
#include <netdb.h>
#include <unistd.h> // close
// #include <cstring> 

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
 * Send messages to server through TCP socket and print the response
 * @param argc number of arguments entered by user
 * @param argv[] the content of arguments
 */ 
int main(int argc, char *argv[]) {
    // server URI, HTTP port
    char *host_adr, *port_num, *path_adr;

    char host[ARR_SIZE], path[ARR_SIZE]; 
    int port = PORT_DEF, succ_parsing = 0;

    char request[BUF_SIZE], response[BUF_SIZE];
    char header_buf[BUF_SIZE];

    const char *requestLineFmt = "GET /%s HTTP/1.1\r\n";
    const char *headerFmt = "Host: %s\r\n";
    const char *connFmt = "Connection: %s\r\n";
    const char *CRLF = "\r\n";

    int sockfd, err; // socket file descriptor and error status
    struct addrinfo hints, *svr_addr, *addr; // getaddrinfo() result

    // check user input
    if (argc < 2)
        checkErr("user input");
    

    // FIX-ME --> switch stmt
    if (sscanf(argv[1], "http://%99[^:]:%99d/%99[^\n]", host, &port, path) == 3) 
    { fprintf(stderr, "\ntest --- 1\n\n"); succ_parsing = 1;}
    else if (sscanf(argv[1], "http://%99[^/]/%99[^\n]", host, path) == 2) 
    { fprintf(stderr, "\ntest --- 2\n\n"); succ_parsing = 1;}
    else if (sscanf(argv[1], "http://%99[^:]:%99d[^\n]", host, &port) == 2) 
    { fprintf(stderr, "\ntest --- 3\n\n"); succ_parsing = 1;}
    else if (sscanf(argv[1], "http://%99[^\n]", host) == 1) 
    { fprintf(stderr, "\ntest --- 4\n\n"); succ_parsing = 1;}

    if (succ_parsing) {
        if ((path != NULL) && (strlen(path) > 0)) 
            asprintf(&host_adr, "%s/%s", host, path);
        else 
            asprintf(&host_adr, "%s", host);
    }
    // get port num and path
    asprintf(&port_num, "%d", port);
    asprintf(&path_adr, "%s", path);
    
    // printf("host = \"%s\"\n", host);
    // printf("path = \"%s\"\n", path);
    // printf("host_adr = \"%s\"\n", host_adr);
    // printf("port_num = \"%s\"\n", port_num);
    // printf("path_adr = \"%s\"\n", path_adr);

    // get IP
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6
    hints.ai_flags = AI_NUMERICSERV; // treat port as number
    hints.ai_protocol = 0; 

    err = getaddrinfo(host, port_num, &hints, &svr_addr);
    if (err != 0)
        checkErr((char *)gai_strerror(err));
    

    // FIX-ME --> reuse
    // requestLine buffer
    size_t bufLen1 = strlen(requestLineFmt) + strlen(path_adr) + 1; // plus 1 for '\0'
    char *buffer1 = (char *)malloc(bufLen1);

    // header buffer
    size_t bufLen2 = strlen(headerFmt) + strlen(host) + 1; // plus 1 for '\0'
    char *buffer2 = (char *)malloc(bufLen2);

    // conn buffer
    size_t bufLen3 = strlen(connFmt) + strlen("close") + 1; // plus 1 for '\0'
    char *buffer3 = (char *)malloc(bufLen3);
    
    // construct request
    strcpy(request, "");
    snprintf(buffer1, bufLen1, requestLineFmt, path_adr);
    snprintf(buffer2, bufLen2, headerFmt, host);
    snprintf(buffer3, bufLen3, connFmt, "close");
    
    strcat(request, buffer1);
    strcat(request, buffer2);
    strcat(request, buffer3);
    strcat(request, CRLF);

    bool success = false;

    for (addr = svr_addr; addr !=NULL; addr = addr->ai_next) {

         // construct socket
        sockfd = socket(addr->ai_family, addr->ai_socktype, 0);

        // connect server
        err = connect(sockfd, svr_addr->ai_addr, svr_addr->ai_addrlen);
        if (err < 0)
            checkErr("connect server");
        else
            success = true;
        
        
        fprintf(stderr, "--------------\nRequest:\n--------------\n%s\n", request);


        // send request
        err = send(sockfd, request, strlen(request), 0);
        if (err < 0)
            checkErr("send request");
        
        // recv response
        err = recv(sockfd, header_buf, BUF_SIZE, 0);
        if (err < 0)
            checkErr("receive response");

        char *header_end = strstr(header_buf, "\r\n\r\n");
        int len;

        // get header
        char header[BUF_SIZE], body[BUF_SIZE], ctnt_len[100], ctnt_type[100];
        len = header_end - header_buf;
        strncpy(header, header_buf, len); // memcpy(tmp, header_buf, len);

        // get body
        len = BUF_SIZE - len - 4;     
        int tmplen = len;   
        strncpy(body, header_end + sizeof("\r\n\r\n") - 1, len);
        fprintf(stderr, "\nfirst_len: %d\n", len);

        char *ctnt_len_start = strstr(header_buf, "Content-Length:") + sizeof("Content-Length:");
        char *ctnt_len_end = strstr(ctnt_len_start, "\n");
        len = ctnt_len_end - ctnt_len_start;
        strncpy(ctnt_len, ctnt_len_start, len); 

        char *ctnt_type_start = strstr(header_buf, "Content-Type:") + sizeof("Content-Type:");
        char *ctnt_type_end = strstr(ctnt_type_start, "\n");
        len = ctnt_type_end - ctnt_type_start;
        strncpy(ctnt_type, ctnt_type_start, len); 

        fprintf(stderr, "--------------\nResponse:\n--------------\n%s\n", header);
        fprintf(stderr, "\n\nctnt_len: %s\n", ctnt_len);
        fprintf(stderr, "ctnt_type: %s\n\n", ctnt_type);

        bool text = false;
        if (strstr(ctnt_type, "text")) {
            fprintf(stderr, "\nTEXT\n");
            printf(body);
            text = true;
        }
        else
        {
            fprintf(stderr, "\nNOT TEXT\n");
            write(fileno(stdout), header_end + sizeof("\r\n\r\n") - 1, tmplen);
            // fwrite(body, sizeof(char), sizeof(body), fileno(stdout));
        }
   
        
        int bytes, rcvd = 0;
        do {
            // bytes = recv(sockfd, response + rcvd, BUF_SIZE - rcvd, 0);
            bytes = recv(sockfd, response, BUF_SIZE, 0);
            if (bytes < 0)
                checkErr("ERROR reading response from socket");

            if (text == true)
                printf(response);
            else
                write(fileno(stdout), response, sizeof(response));
                

            rcvd += bytes;
            memset(response, 0, BUF_SIZE);
        } while (bytes != 0);
       
        fprintf(stderr, "\nREST LEN %d\n", rcvd);

        if (success)
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