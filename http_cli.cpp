    
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
    const char *requestLineFmt = "GET /%s HTTP/1.0\r\n";
    const char *headerFmt = "Host: %s\r\n";
    const char *CRLF = "\r\n";

    int sockfd, err; // socket file descriptor and error status
    struct addrinfo hints, *svr_addr, *addr; // getaddrinfo() result

    // check user input
    if (argc < 2)
        checkErr("user input");
    

    // FIX-ME --> switch stmt
    if (sscanf(argv[1], "http://%99[^:]:%99d/%99[^\n]", host, &port, path) == 3) 
    { printf("test --- 1\n"); succ_parsing = 1;}
    else if (sscanf(argv[1], "http://%99[^/]/%99[^\n]", host, path) == 2) 
    { printf("test --- 2\n"); succ_parsing = 1;}
    else if (sscanf(argv[1], "http://%99[^:]:%99d[^\n]", host, &port) == 2) 
    { printf("test --- 3\n"); succ_parsing = 1;}
    else if (sscanf(argv[1], "http://%99[^\n]", host) == 1) 
    { printf("test --- 4\n"); succ_parsing = 1;}

    if (succ_parsing) {
        if ((path != NULL) && (strlen(path) > 0)) 
            asprintf(&host_adr, "%s/%s", host, path);
        else 
            asprintf(&host_adr, "%s", host);
    }
    // get port num and path
    asprintf(&port_num, "%d", port);
    asprintf(&path_adr, "%s", path);
    

    printf("host = \"%s\"\n", host);
    printf("path = \"%s\"\n", path);
    printf("host_adr = \"%s\"\n", host_adr);
    printf("port_num = \"%s\"\n", port_num);
    printf("path_adr = \"%s\"\n", path_adr);

    // get IP
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6
    // hints.ai_socktype = SOCK_STREAM; // use TCP
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

    printf("check 3\n");

    // construct request
    strcpy(request, "");
    snprintf(buffer1, bufLen1, requestLineFmt, path_adr);
    snprintf(buffer2, bufLen2, headerFmt, host);
    strcat(request, buffer1);
    strcat(request, buffer2);
    strcat(request, CRLF);

    printf("check 4\n"); // printf("check \n");


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

        if (success)
            break;

    }


    // free ptr memory
    free(buffer1);
    buffer1 = NULL;
    free(buffer2);
    buffer2 = NULL;

    freeaddrinfo(svr_addr);
    svr_addr = NULL;

    close(sockfd);
    
    return 0;
}   