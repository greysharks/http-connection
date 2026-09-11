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
			int result = RunClient(&addr, sock);
			HandleRuntimeErrors(result, sock);
    }

    // otherwise wrong arguments were passed to program
    else {
      close(sock);
      perror("command-line arguments");
      exit(EXIT_FAILURE);
    }
  }

  close(sock);
	exit(EXIT_SUCCESS);
	
  return 0;
}