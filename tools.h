#pragma once

enum Errors {
  ERRORS_READ_FILE = -1,
  ERRORS_OPEN_SERVER_FILES = -2,
  ERRORS_SERVER_BIND = -3,
  ERRORS_SERVER_LISTEN = -4,
  ERRORS_EPOLL_CREATE = -5,
  ERRORS_EPOLL_CTL = -6,
  ERRORS_EPOLL_WAIT = -7,
  ERRORS_CLIENT_CONNECT = -8,
  ERRORS_SOCKET_SEND = -9,
  ERRORS_SOCKET_SHUTDOWN = -10,
  ERRORS_SOCKET_RECV = -11
};

// this function returns a string, containing the content of requested file
char* ReadFile(char* file_name, long int* file_size); 
// this function excerpts information from a string, pointed out by the given substring and the given separator (start and the end of each portion of information)
char** SearchAfter(char* substring, char* string, char* separator, unsigned long int* number_of_results); 

void HandleRuntimeErrors(int error, int socket);