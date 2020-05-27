/**
 * A simple HTTP server that receives a request from a client 
 * and then responses with the file requested, if it exists
 * @author Tong (Debby) Ding
 * @version 1.0
 * @see CPSC 5510 Spring 2020, Seattle University
 */
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
#include <iostream>

int handle_client(int sock);
int send_msg(int sock, char* buffer, unsigned long num_bytes);
int recv_msg(int sock, char* buffer, unsigned long num_bytes);
unsigned short get_port(int argc, char* argv[]);
void error_exit(const char* step);
char* parse_request(const char* request, char* flag);
void get_file_type(const char* path, char* flag, char* file_type);
char* read_file(const char* path, char* flag, size_t file_size);
void prep_header(char* header, char* mod_time, const char* file_type, const char* flag, const char* len_buf, const char* date_buf);
void prep_header_err(char* header, const char* flag);

#define BACKLOG 5
#define BUF_SIZE 8190

/**
 * Initiate a server to receive request from clients
 * @param argc number of arguments entered by user
 * @param argv[] the content of arguments
 */ 
int main(int argc, char* argv[]) {

    // set up listening socket
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

        // handle requests from clients
        int client_sock = accept(listening_sock, (struct sockaddr*) &cli_addr, &cli_size);
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

/**
 * Handle communication with one client connection
 * @param sock the socket fd connected to the client
 * @return an error code (0 success, -1 error)
 */ 
int handle_client(int sock) {

    char flag[100] = "200 OK";  // error status code
    char header[BUF_SIZE];  // response header

    // get request from client
    char request_header[BUF_SIZE];
    int retval = recv_msg(sock, request_header, BUF_SIZE);
    if (retval < 0) {
        fprintf(stderr, "ERROR: failed to receive message\n");
        return -1;
    }    
    char* header_endptr = strstr(request_header, "\r\n\r\n");
    if (header_endptr == nullptr) 
        strcpy(flag, "413 Entity Too Large");
        
    printf("\n\n---------- Received request ----------\n");
    printf(request_header);

    try {
        printf("* parse request\n");
        char* path = parse_request(request_header, flag);

        printf("* check file status\n");
        struct stat sb;
        if (stat(path, &sb) == -1)
            perror("stat");
        size_t file_size = (size_t) sb.st_size;
        char len_buf[20];
        sprintf(len_buf, "%ld", sb.st_size);
        char* mod_time = ctime(&sb.st_mtime);

        printf("* prepare response msg\n");
        // get file type
        char file_type[20];
        get_file_type(path, flag, file_type);
        // get file
        char* file_buf = read_file(path, flag, file_size);
        // get date
        time_t now = time(0);
        struct tm tm = *gmtime(&now);
        char date_buf[1000];
        strftime(date_buf, sizeof date_buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);

        // prepare header
        if (strncmp(flag, "200 OK", 6) != 0)
            prep_header_err(header, flag);
        else
            prep_header(header, mod_time, file_type, flag, len_buf, date_buf);

        // prepare response
        char* response = (char*)malloc(strlen(header) + file_size);
        strcpy(response, header);
        memcpy(response + strlen(header), file_buf, file_size);

        printf("\n---------- Sending response ----------\n");
        printf(response);

        // send response
        retval = send_msg(sock, response, strlen(header) + file_size);
        if (retval < 0) {
            fprintf(stderr, "ERROR: failed to send message\n");
            return -1;
        }
        free(path);
        free(file_buf);
        free(response);

    } catch (...) {
        strcpy(flag, "500 Internal Server Error");
        prep_header_err(header, flag);
        retval = send_msg(sock, header, strlen(header));
        if (retval < 0) {
            fprintf(stderr, "ERROR: failed to send message\n");
            return -1;
        }
    }

    return 0;
}

/**
 * Get the type of a file at a given path
 * @param path the path to locate the file
 * @param flag indicate error status code
 * @param file_type the type found
 */ 
void get_file_type(const char* path, char* flag, char* file_type) {
    const char *type_start = strchr(path, '.') + 1;
    const char *type_end = strchr(path, '\0');
    size_t type_len = type_end - type_start;
    char* typestr = (char*)malloc((type_len+1) * sizeof(char));
    strncpy(typestr, type_start, type_len);
    typestr[strlen(typestr)] = '\0';

    if (strncmp(typestr, "txt", 3) == 0)
        strcpy(file_type, "text/plain");
    else if (strncmp(typestr, "html", 4) == 0)
        strcpy(file_type, "text/html");
    else if (strncmp(typestr, "htm", 3) == 0)
        strcpy(file_type, "text/htm");
    else if (strncmp(typestr, "css", 3) == 0)
        strcpy(file_type, "text/css");
    else if (strncmp(typestr, "jpg", 3) == 0)
        strcpy(file_type, "image/jpeg");
    else if (strncmp(typestr, "jpeg", 4) == 0)
        strcpy(file_type, "image/jpeg");
    else if (strncmp(typestr, "png", 3) == 0)
        strcpy(file_type, "image/jpeg");
    else
        strcpy(flag, "400 Bad Request");
    
    free(typestr);
}

/**
 * Parse a given request to find the file path
 * @param request the request to parse
 * @param flag indicate error status code
 * @return the path found
 */
char* parse_request(const char* request, char* flag) {
    // check if not GET
    if (strncmp(request, "GET", 3) != 0) {
        strcpy(flag, "501 Not Implemented");
        return nullptr;
    }
    const char *path_start = strchr(request, ' ') + 1;
    const char *path_end = strchr(path_start, ' ');
    size_t path_len = path_end - path_start;
    size_t root_len = strlen("web_root");

    char* path = (char*)malloc((root_len + path_len) * sizeof(char));
    strcpy(path, "web_root");
    strncpy(path + root_len, path_start, path_len);
    path[strlen(path)] = '\0';

    return path;
}

/**
 * Get the content of a file at a given path
 * @param path the path to locate a file
 * @param flag indicate error status code
 * @param file_size the size of file
 * @return the content of the file
 */ 
char* read_file(const char* path, char* flag, size_t file_size) {
    char* file_buf = (char*)malloc((file_size + 1) * sizeof(char));
    FILE* file_stream = fopen(path, "rb");
    if (file_stream == NULL) {
        strcpy(flag, "404 Not Found");
    }
    if (file_stream != nullptr) {
        fread(file_buf, 1, file_size, file_stream);
        fclose(file_stream);
    }
    return file_buf;
}

/**
 * Generate a normal header with given information
 * @param header the header to form
 * @param mod_time last file modification time
 * @param file_type the type of file
 * @param flag indicate error status code 
 * @param len_buf the size of file
 * @param date_buf the current date
 */
void prep_header(char* header, char* mod_time, const char* file_type, const char* flag, const char* len_buf, const char* date_buf) {
    // header info format
    const char *stat_fmt = "HTTP/1.1 %s\r\n";
    const char *conn_fmt = "Connection: %s\r\n";
    const char *type_fmt = "Content-Type: %s\r\n";
    const char *clen_fmt = "Content-Length: %s\r\n";   
    const char *modi_fmt = "Last-Modified: %s";
    const char *date_fmt = "Date: %s\r\n"; 
    const char *CRLF = "\r\n";

    size_t tmp_len;
    tmp_len = strlen(stat_fmt) + strlen(flag) + 1;
    char *buffer1 = (char *)malloc(tmp_len);
    snprintf(buffer1, tmp_len, stat_fmt, flag);

    tmp_len = strlen(conn_fmt) + strlen("close") + 1;
    char *buffer2 = (char *)malloc(tmp_len);
    snprintf(buffer2, tmp_len, conn_fmt, "close");

    tmp_len = strlen(type_fmt) + strlen(file_type) + 1;
    char *buffer3 = (char *)malloc(tmp_len);
    snprintf(buffer3, tmp_len, type_fmt, file_type);

    tmp_len = strlen(clen_fmt) + strlen(len_buf) + 1;
    char *buffer4 = (char *)malloc(tmp_len);
    snprintf(buffer4, tmp_len, clen_fmt, len_buf);
    
    tmp_len = strlen(modi_fmt) + strlen(mod_time) + 1;
    char *buffer5 = (char *)malloc(tmp_len);
    snprintf(buffer5, tmp_len, modi_fmt, mod_time);

    tmp_len = strlen(date_fmt) + strlen(date_buf) + 1;
    char *buffer6 = (char *)malloc(tmp_len);
    snprintf(buffer6, tmp_len, date_fmt, date_buf);
    
    strcpy(header, "");
    strcat(header, buffer1);
    strcat(header, buffer2);
    strcat(header, buffer3);
    strcat(header, buffer4);
    strcat(header, buffer5);
    strcat(header, buffer6);
    strcat(header, CRLF);
    free(buffer1);
    free(buffer2);
    free(buffer3);
    free(buffer4);
    free(buffer5);
    free(buffer6);
}

/**
 * Generate a short header with given information
 * @param header the header to form
 * @param flag indicate error status code 
 */
void prep_header_err(char* header, const char* flag) {
    // header info format
    const char *stat_fmt = "HTTP/1.1 %s\r\n";
    const char *conn_fmt = "Connection: %s\r\n";
    const char *CRLF = "\r\n";

    size_t tmp_len;
    tmp_len = strlen(stat_fmt) + strlen(flag) + 1;
    char *buffer1 = (char *)malloc(tmp_len);
    snprintf(buffer1, tmp_len, stat_fmt, flag);
    
    tmp_len = strlen(conn_fmt) + strlen("close") + 1;
    char *buffer2 = (char *)malloc(tmp_len);
    snprintf(buffer2, tmp_len, conn_fmt, "close");
    
    strcpy(header, "");
    strcat(header, buffer1);
    strcat(header, buffer2);
    strcat(header, CRLF);
    free(buffer1);
    free(buffer2);
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
        buffer += retval;
        num_bytes -= retval;
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

/**
 * Print out an error and then exit the program
 * @param step the step at which an error occurs
 */ 
void error_exit(const char* step) {
    perror(step);
    exit(1);
}