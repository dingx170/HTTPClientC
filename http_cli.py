import socket
import sys
import os

# CONN_TIMEOUT = 5
CHUNK_SIZE = 1024
CRLF = '\r\n'
DIR_NAME = 'files'
FILE_TYPE = {'.txt' : 'text/plain',
             '.html' : 'text/html',
             '.htm' : 'text/htm',
             '.css' : 'text/css',
             '.jpg' : 'image/jpeg',
             '.jpeg' : 'image/jpeg',
             '.png' : 'image/jpeg'}

# check user input
if len(sys.argv) != 2:
    print('Usage: python http_cli.py PORT')
    exit(1)

port = int(sys.argv[1])


def parse_header(header):
    lines = header.split(CRLF)

    print("---- test split header ----") # DEL
    for line in lines:
        print(line)
    
    # code = header_fields.pop(0).split(' ')[1]
    items = {}
    for line in lines:
        key, val = line.split(':', 1)
        items[key.lower()] = val
    
    return items

# create socket
svr_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
try:
    # bind to the given port
    svr_sock.bind('', port)

    # start listening
    svr_sock.listen(5)

    while True:
        # accept connection
        cli_sock, addr = svr_sock.accept()
        print('Connection accepted: ', addr)

        data = ''
        # get client message
        while True:

            if not data:
                break
        
            data += cli_sock.recv(CHUNK_SIZE)
            # data.decode('utf-8')
        
        # get header
        header, body = data.split(CRLF + CRLF, 1)

        fields = parse_header(header)
        # fields[]
        # https://stackoverflow.com/questions/42815084/write-a-basic-http-server-using-socket-api
        # TO-DO: get file or identify error
        file_path = ''
        file_info = os.stat(file_path)
        file_size = file_info.st_size
        file_time = file_info.st_mtime

        # TO-DO: create header
        req_type = ''
        file_type = FILE_TYPE[req_type]

        header_msg = ''
        status = '200 OK'

        header_msg = CRLF.join(["HTTP/1.1 {}".format(status), 
                                "Content-Type: {}".format(file_type),
                                "Connection: Close\r\n\r\n"])

        # request_text = (
        #     b'GET /who/ken/trust.html HTTP/1.1\r\n'
        #     b'Host: cm.bell-labs.com\r\n'
        #     b'Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n'
        #     b'Accept: text/html;q=0.9,text/plain\r\n'
        #     b'\r\n'
        # )

        # print({ k:v.strip() for k,v in [line.split(":",1) 
        #         for line in request_text.decode().splitlines() if ":" in line]})

        cli_sock.sendall(header_msg + data)
        cli_sock.shutdown(SHUT_WR)
        cli_sock.close()
    except Exception as e:
        print("Error: {}\n".format(e))

svr_sock.close()

# https://stackoverflow.com/questions/5755507/creating-a-raw-http-request-with-sockets
# https://stackoverflow.com/questions/47658584/implementing-http-client-with-sockets-without-http-libraries-with-python