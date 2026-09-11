#pragma once

#include <netinet/in.h>

struct server_file {
  char* file_content;
  char* url_path;
  long int size_of_file;
};

// return the array of opened files (their content) along with their location
// paths (A.K.A. server paths)
struct server_file *OpenServerFiles(char* server_paths, unsigned long int* number_of_server_paths);
// runs server based on information provided by sockaddr_in structure (which IP and port number server should use) as well as using socket fid
int RunServer(struct sockaddr_in* socket_address, int socket);

// receive HTTP message (either client's request or server's response)
char* ReceiveMessage(int connection, unsigned long int* http_message_length);
// form server's response based on client's request 
char* FormResponse(char* request_content, unsigned long int request_length, struct server_file *server_files, unsigned long int number_of_files); 
