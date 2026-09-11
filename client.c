#include "client.h"
#include "tools.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int RunClient(struct sockaddr_in* socket_address, int socket) {
  const unsigned int BUFF_SIZE = 512; // max size of bytes of receive buffer
      char buff[BUFF_SIZE];               // for receive buffer

      ssize_t bytes_received = 0;

      long int request_fsize;

      // read request file
      char *message = ReadFile("request.txt", &request_fsize);

      if (message == NULL) {
        return ERRORS_READ_FILE;
      }

      // connect to the server using its port number and IP
      if (connect(socket, (struct sockaddr *)socket_address, sizeof((*socket_address))) < 0) {
        return ERRORS_CLIENT_CONNECT;
      }

      // send client's request to server
      if (send(socket, message, (size_t)(request_fsize + 1), 0) < 0) {
        return ERRORS_SOCKET_SEND;
      }

      if (shutdown(socket, SHUT_WR) < 0) {
        return ERRORS_SOCKET_SHUTDOWN;
      }

      puts("server's response:\n");

      // try to recevie the first portion of bytes of the server's response
      bytes_received = recv(socket, buff, BUFF_SIZE, 0);

      // keep printing all bytes of data that client recevies from the server
      while (bytes_received > 0) {
        for (unsigned short int i = 0; i < bytes_received; i++) {
          putchar(buff[i]);
        }

        bytes_received = recv(socket, buff, BUFF_SIZE, 0);
      }

      // check if any error occured while trying to receive data from server
      // (in other case the client successfully received all the data)
      if (bytes_received < 0) {
        return ERRORS_SOCKET_RECV;
      }

      free(message);
      shutdown(socket, SHUT_RD);
      return 0;
}