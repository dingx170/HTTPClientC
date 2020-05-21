#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <iostream>
#include <ostream>
// #include <string>


int handle_client(int sock);
int send_msg(int sock, char* buffer, unsigned long num_bytes);
int recv_msg(int sock, char* buffer, unsigned long num_bytes);
unsigned short get_port(int argc, char* argv[]);
void error_exit(const char* step);
void parse_request(const char* request, char* path, char* flag);


#define BACKLOG 5
#define BUF_SIZE 8190

int main(int argc, char* argv[]) {

    unsigned short listening_port = get_port(argc, argv);

    struct sockaddr_in listening_sockaddr;
    memset(&listening_sockaddr, 0, sizeof(listening_sockaddr));
    listening_sockaddr.sin_family = AF_INET;
    listening_sockaddr.sin_port = htons(listening_port);
    listening_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int listening_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_sock == -1)
        error_exit("create listening socket");

    int retval = bind(listening_sock,
                    (struct sockaddr*) &listening_sockaddr,
                    sizeof(listening_sockaddr));
    if (retval < 0)
        error_exit("bind listening socket");

    printf("* start listening\n");

    retval = listen(listening_sock, BACKLOG);
    if (retval)
        error_exit("listen");
    
    while (true) {
        struct sockaddr_in cli_addr;
        socklen_t cli_size; 

        int client_sock = accept(listening_sock, (struct sockaddr *) &cli_addr, &cli_size);
        if (client_sock == -1) {
            perror("accept client connection");
        } else {
            handle_client(client_sock);
            close(client_sock);
        }
    }
    close(listening_sock);
    return 0;
}

void error_exit(const char* step) {
    perror(step);
    exit(1);
}

/**
 * Handles communication with one client connection
 * PRE:  the socket (sock param) is connected
 * POST: the socket (sock param) is connected (the caller must close it)
 * 
 * @param sock the socket fd connected to the client
 * @return an error code (0 success, -1 error)
 */ 
int handle_client(int sock) {
    char header_buf[BUF_SIZE];
    int retval = recv_msg(sock, header_buf, BUF_SIZE);  // update buf size ???
    
    if (retval < 0) {
        fprintf(stderr, "ERROR: failed to receive message\n");
        return -1;
    }

    char flag[100] = "200 OK";

    char *header_endptr = strstr(header_buf, "\r\n\r\n");
    if (header_endptr == nullptr)
        strcpy(flag, "413 Entity Too Large");
    
    printf("* receive request\n------------------\n");
    printf(header_buf);

    printf("* parse request\n");
    char path[BUF_SIZE];
    parse_request(header_buf, path, flag);

    // check if not ok
    if (strncmp (flag, "200 OK", 6) != 0)
        printf("\nNOT OK\n");
    
    printf("* get data to send\n");
    struct stat sb;
    if (stat(path, &sb) == -1)
        perror("stat");
    printf("out: ");
    printf("File size: %lld bytes\n", (long long) sb.st_size);
    printf("Last file modification: %s", ctime(&sb.st_mtime));
    

    printf("* prepare msg\n");
    // header info
    const char *stat_fmt = "HTTP/1.1 %s\r\n";
    const char *conn_fmt = "Connection: %s\r\n";
    const char *type_fmt = "Content-type: %s\r\n";
    // const char *clen_fmt = "Content-length: %s\r\n";   
    const char *modi_fmt = "Last-modified: %s\r\n";
    // const char *date_fmt = "Date: %s\r\n"; 
    const char *CRLF = "\r\n";
    size_t tmp_len;
    tmp_len = strlen(stat_fmt) + strlen(flag) + 1;
    char *buffer1 = (char *)malloc(tmp_len);
    snprintf(buffer1, tmp_len, stat_fmt, flag);
    
    tmp_len = strlen(conn_fmt) + strlen("close") + 1;
    char *buffer2 = (char *)malloc(tmp_len);
    snprintf(buffer2, tmp_len, conn_fmt, "close");
    //
    printf("check 1");
    tmp_len = strlen(type_fmt) + strlen("text") + 1;
    printf("check 2");
    char *buffer3 = (char *)malloc(tmp_len);
    printf("check 3");
    snprintf(buffer3, tmp_len, stat_fmt, "text");
    printf("check 4");
    printf("%ld", sb.st_size);
    printf("check 1");

    // tmp_len = strlen(clen_fmt) + (size_t) sb.st_size + 1;
    // char *buffer4 = (char *)malloc(tmp_len);
    // snprintf(buffer4, tmp_len, clen_fmt, sb.st_size);
    
    tmp_len = strlen(modi_fmt) + strlen(ctime(&sb.st_mtime)) + 1;
    char *buffer5 = (char *)malloc(tmp_len);
    snprintf(buffer5, tmp_len, modi_fmt, ctime(&sb.st_mtime));

    // tmp_len = strlen(date_fmt) + strlen(flag) + 1;
    // char *buffer6 = (char *)malloc(tmp_len);
    // snprintf(buffer6, tmp_len, date_fmt, flag);





    // std::string msg = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello world!";
    // int len = msg.size();
    // char *hello = new char[len + 1];
    // std::copy(msg.begin(), msg.end(), hello);
    // hello[len] = '\0';
    
    char request[0xfff];
    strcpy(request, "");
    strcat(request, buffer1);
    strcat(request, buffer2);
    strcat(request, buffer3);
    // strcat(request, buffer4);
    strcat(request, buffer5);
    // strcat(request, buffer6);
    strcat(request, CRLF);

    printf(request);

    // send
    retval = send_msg(sock, request, strlen(request));

    // free(request);
    free(buffer1);
    buffer1 = NULL;
    free(buffer2);
    buffer2 = NULL;
    free(buffer3);
    buffer3 = NULL;
    // free(buffer4);
    // buffer4 = NULL;
        free(buffer5);
    buffer5 = NULL;
    //     free(buffer6);
    // buffer6 = NULL;

    if (retval < 0) {
        fprintf(stderr, "ERROR: failed to send message\n");
        return -1;
    }
    return 0;
}

void parse_request(const char* request, char* path, char* flag) {
    // check if not GET
    if (strncmp (request, "GET", 3) != 0)
        strcpy(flag, "501 Not Implemented");

    const char *start = strchr(request, '/') + 1;
    const char *end = strchr(start, ' ');
    strncpy(path, start, end - start);
    path[end - start + 1] = '\0';

    // printf("in: %s\n", path);
    // printf("in: %d\n", (int)sizeof(path));

    // struct stat sb;
    // if (stat(path, &sb) == -1)
    //     perror("stat");
    // printf("in: ");
    // printf("File size: %lld bytes\n", (long long) sb.st_size);
    // printf("Last file modification: %s", ctime(&sb.st_mtime));
}


/**
 * Send exactly num_bytes from the buffer
 * @param sock the socket to send to
 * @param buffer the buffer to read and send the data from
 * @param num_bytes the number of bytes to send
 * @return an error code (0 success, -1 error)
 */ 
int send_msg(int sock, char* buffer, unsigned long num_bytes) {
    int retval;
    while (num_bytes > 0) {
        retval = send(sock, buffer, num_bytes, 0);

        if (retval < 0) {
            perror("send message");
            return -1;
        } else if (retval == 0) {
            fprintf(stderr, "ERROR: client closed the connection unexpectedly\n");
            return -1;
        }
        assert(retval > 0); // WHY
        buffer += retval;
        num_bytes -= retval;
        assert(num_bytes >= 0); // WHY
    }
    return 0;
}
 
/**
 * Receive exactly num_bytes and write them into buffer
 * @param sock the socket to receive from
 * @param buffer the buffer to write the recieved data to
 * @param num_bytes the number of bytes to receive
 * @return an error code (0 success, -1 error)
 */ 
int recv_msg(int sock, char* buffer, unsigned long num_bytes) {
    int retval; // return value

    retval = recv(sock, buffer, num_bytes, 0);

    if (retval < 0) {
        perror("recive data");
        return -1;
    } else if (retval == 0) {
        fprintf(stderr, "ERROR: client closed the connection unexpectedly\n");
        return -1;
    }

    return 0;
}

/**
 * Get the port number from the first command line argument
 * PRE: argv has argc elements
 * POST: argv has at least 1 element
 * 
 * @param argc number of command line arguments
 * @param argv array of command line arguments
 * @return the port number
 */
unsigned short get_port(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s port\n", argv[0]);
        exit(1);
    }
    char* endptr = NULL; // get the char in str after numerical value
    errno = 0; // check overflow
    unsigned long port = strtoul(argv[1], &endptr, 10);  // base 10
    
    if (endptr != NULL && *endptr != '\0') {
        printf("ERROR: %s is not a number\n", argv[1]);
        exit(1);
    } else if (port == ULONG_MAX && errno == ERANGE){
        printf("ERROR: %s: %s\n", strerror(ERANGE), argv[1]);
        exit(1);
    } else if (port > USHRT_MAX || port < 0) { // USHRT_MAX: 2^16-1
        printf("ERROR: %s: %lu is not a valid port number\n", strerror(ERANGE), port);
        exit(1);
    }
    
    return port;
}