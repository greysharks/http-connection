#include "main.h"
#include "client.h"
#include "server.h"
#include "tools.h"

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
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
      int result = RunServer(&addr, sock);
      HandleRuntimeErrors(result, sock);
    }

    // if program is used as client
    else if (strcmp(argv[1], "-c") == 0) {
      const unsigned int BUFF_SIZE = 512; // max size of bytes of receive buffer
      char buff[BUFF_SIZE];               // for receive buffer

      ssize_t bytes_received = 0;

      long int request_fsize;

      // read request file
      char *message = ReadFile("request.txt", &request_fsize);

      if (message == NULL) {
        close(sock);
        perror("ReadFile()");
        exit(EXIT_FAILURE);
      }

      // connect to the server using its port number and IP
      if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        perror("connect()");
        exit(EXIT_FAILURE);
      }

      // send client's request to server
      if (send(sock, message, (size_t)(request_fsize + 1), 0) < 0) {
        close(sock);
        perror("send()");
        exit(EXIT_FAILURE);
      }

      if (shutdown(sock, SHUT_WR) < 0) {
        close(sock);
        perror("shutdown()");
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

      free(message);
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