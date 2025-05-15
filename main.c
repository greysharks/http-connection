#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char* argv[]) {
  const unsigned int IP = (127 << 24) + 1; // for ip 127.0.0.1
  const unsigned short int PORT = 80;
  const unsigned short int MAX_CONNECTIONS = 10;

  const unsigned int BUFF_SIZE = 512; // max size of bytes of receive buffer
  char buff[BUFF_SIZE]; // for receive buffer
  unsigned short int bytes_received = 0;

  // structure that holds server's IP, port number and address family
  // (it can be used either to create a server with these values or
  // either to connect to the server with these values)
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = htonl(IP);

  int sock; // for socket

  // initialize socket for either client or server
  sock = socket(addr.sin_family, SOCK_STREAM, 0);

  if (sock < 0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  if (argc > 1) {
    // if program is used as server
    if (strcmp(argv[1], "-s") == 0) {
      int conns[MAX_CONNECTIONS]; // for connections
      unsigned short int N = 0; // N is number of connections
      FILE* response; // server's http response
      long int file_size;

      // open file with server's http response in read mode
      response = fopen("response.txt", "r");

      // calculate response file size
      if (fseek(response, 0, SEEK_END) != 0) {
        close(sock);
        perror("fseek()");
        exit(EXIT_FAILURE);
      }
      file_size = ftell(response);
      rewind(response);

      // read response file
      char message[file_size];
      fread(message, 1, file_size, response);
      fclose(response);

      // used only for accept(...) function
      unsigned int addr_len = sizeof (addr);

      // mark unexisting connections as -1
      for (unsigned short int i = 0; i < MAX_CONNECTIONS; i++) {
        conns[i] = -1;
      }

      // bind server socket with server's IP address, port number and address family
      if (bind(sock, (struct sockaddr*) &addr, sizeof (addr)) < 0) {
        close(sock);
        perror("bind()");
        exit(EXIT_FAILURE);
      }

      // make the server listen to new connections until they reach MAX_CONNECTIONS
      if (listen(sock, MAX_CONNECTIONS) < 0) {
        close(sock);
        perror("listen()");
        exit(EXIT_FAILURE);
      }

      printf("server is listening on port %hu\n", PORT);

      while (1) {
        // add new connections to the array that holds all connections (as they occur)
        if (N < MAX_CONNECTIONS) {
          int conn; // for connection
          conn = accept(sock, (struct sockaddr*) &addr, &addr_len);

          if (conn >= 0) {
            conns[N] = conn;
            N += 1;
          }
        }

        // go through all connections (skipping unexisting ones)
        for (unsigned short int i = 0; i < MAX_CONNECTIONS; i++) {
          if (conns[i] == -1) {
            continue;
          }

          puts("client's request:\n");

          // read client's request and print it out in console
          bytes_received = recv(conns[i], buff, BUFF_SIZE, 0);

          // these variables will be used to check whether the received bytes from client
          // contain starting parts or ending parts of http request
          char* http_method;
          char* http_version;
          char* end_of_request;

          while (bytes_received > 0) {
            http_method = strstr(buff, "GET");
            http_version = strstr(buff, "HTTP/1.1");
            end_of_request = strstr(buff, "\n\n");

            if (http_method != NULL && http_version != NULL) {
              // send server's response to client
              if (send(conns[i], message, sizeof (message), 0) < 0) {
                close(conns[i]);
                conns[i] = -1;
                N -= 1;
              }

              // close the write option for connetion socket
              if (shutdown(conns[i], SHUT_WR) < 0) {
                close(conns[i]);
                conns[i] = -1;
                N -= 1;
              }
            }

            // display request message in console
            for (unsigned short int i = 0; i < bytes_received; i++) {
              putchar(buff[i]);
            }

            if (end_of_request != NULL) {
              // close the read option for connection socket
              if (shutdown(conns[i], SHUT_RD) < 0) {
                close(conns[i]);
                conns[i] = -1;
                N -= 1;
              }

              break;
            }

            bytes_received = recv(conns[i], buff, BUFF_SIZE, 0);
          }

          // check if any error occured while trying to receive request from client
          if (bytes_received < 0) {
            close(conns[i]);
            conns[i] = -1;
            N -= 1;
          }

          // if current connection is valid, then close it after sending server's message
          // (because in this implementation server sends only one message
          // and doesn't receive any messages from the client)
          if (conns[i] != -1) {
            close(conns[i]);
            conns[i] = -1;
            N -= 1;
          }
        }
      }
    }

    // if program is used as client
    else if (strcmp(argv[1], "-c") == 0) {
      FILE* request; // client's http request
      long int file_size;

      // open file with client's http request in read mode
      request = fopen("request.txt", "r");

      // calculate request file size
      if (fseek(request, 0, SEEK_END) != 0) {
        close(sock);
        perror("fseek()");
        exit(EXIT_FAILURE);
      }
      file_size = ftell(request);
      rewind(request);

      // read request file
      char message[file_size];
      fread(message, 1, file_size, request);
      fclose(request);

      // connect to the server using its port number and IP
      if (connect(sock, (struct sockaddr*) &addr, sizeof (addr)) < 0) {
        close(sock);
        perror("connect()");
        exit(EXIT_FAILURE);
      }

      // send client's request to server
      if (send(sock, message, sizeof (message), 0) < 0) {
        close(sock);
        perror("send()");
        exit(EXIT_FAILURE);
      }

      if (shutdown(sock, SHUT_WR) < 0) {
        close(sock);
        perror("send()");
        exit(EXIT_FAILURE);
      }

      puts("server's response:\n");

      // try to recevie the first portion of bytes of the server's response
      bytes_received = recv(sock, buff, BUFF_SIZE, 0);

      // keep printing all bytes of data that client recevies from the server
      while (bytes_received > 0) {
        for (unsigned short int i = 0; i < bytes_received; i++) {
          putchar(buff[i]);
        }

        bytes_received = recv(sock, buff, BUFF_SIZE, 0);
      }

      // check if any error occured while trying to receive data from server
      // (in other case the client successfully received all the data)
      if (bytes_received < 0) {
        close(sock);
        perror("recv()");
        exit(EXIT_FAILURE);
      }

      shutdown(sock, SHUT_RD);
      close(sock);
      exit(EXIT_SUCCESS);
    }

    // otherwise wrong arguments were passed to program
    else {
      close(sock);
      perror("command-line arguments");
      exit(EXIT_FAILURE);
    }
  }

  close(sock);

  return 0;
}
