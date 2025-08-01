#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct server_file {
  char* file_content;
  char* url_path;
  long int size_of_file;
};

char* ReadFile(char* file_name, long int* file_size); // return a string, containing the content of requested file
char** SearchAfter(char* substring, char* string, char* separator, unsigned long int* number_of_results); // excerpt information from a string, pointed out by the given substring and the given separator (start and the end of each portion of information)
struct server_file* OpenServerFiles(char* server_paths, unsigned long int* number_of_server_paths); // return the array of opened files (their content) along with their location paths (A.K.A. server paths)
char* ReceiveMessage(int connection, unsigned long int* http_message_length); // receive HTTP message (either client's request or server's response)
char* FormResponse(char* request_content, unsigned long int request_length, struct server_file* server_files, unsigned long int number_of_files); // form server response based on client's request

int main(int argc, char* argv[]) {
  const unsigned int IP = (127 << 24) + 1; // for ip 127.0.0.1
  const unsigned short int PORT = 80;

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
      const unsigned short int MAX_CONNECTIONS = 10;
      int conns[MAX_CONNECTIONS]; // for connections
      unsigned short int N = 0; // N is number of connections
      // long int response_fsize; // fsize is file size
      long int server_paths_fsize;

      // used only for accept(...) function
      unsigned int addr_len = sizeof (addr);

      char* response;
      char* clients_request;
      char* server_paths;

      struct server_file* server_files;

      unsigned long int clients_request_length;
      unsigned long int number_of_resources;

      // mark unexisting connections as -1
      for (unsigned short int i = 0; i < MAX_CONNECTIONS; i++) {
        conns[i] = -1;
      }

      // read response file
      // = ReadFile("response.txt", &response_fsize);

      // if (message == NULL) {
      //   close(sock);
      //   perror("ReadFile()");
      //   exit(EXIT_FAILURE);
      // }

      // read a file, containing all url paths available in this server (in form of list)
      server_paths = ReadFile("paths.txt", &server_paths_fsize);

      if (server_paths == NULL) {
        close(sock);
        perror("ReadFile()");
        exit(EXIT_FAILURE);
      }

      // open server files, using the contents of file, that contains server (available) paths
      server_files = OpenServerFiles(server_paths, &number_of_resources);

      if (server_files == NULL) {
        close(sock);
        perror("OpenServerFiles()");
        exit(EXIT_FAILURE);
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

          // receive request from client
          clients_request = ReceiveMessage(conns[i], &clients_request_length);

          // check if any error occured while trying to receive request from client
          if (clients_request == NULL) {
            close(conns[i]);
            conns[i] = -1;
            N -= 1;
            continue;
          }

          // display request message in console
          for (unsigned long int j = 0; j < clients_request_length; j++) {
            putchar(clients_request[j]);
          }

          response = FormResponse(clients_request, clients_request_length, server_files, number_of_resources);

          // check if any error occurred while forming server's response
          if (response == NULL) {
            free(clients_request);
            close(conns[i]);
            conns[i] = -1;
            N -= 1;
            continue;
          }

          send(conns[i], response, (size_t) strlen(response), 0);

          free(clients_request);
          free(response);

          shutdown(conns[i], SHUT_WR);

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

      for (unsigned long int i = 0; i < number_of_resources; i++) {
        free(server_files[i].file_content);
        free(server_files[i].url_path);
      }

      free(server_files);
      free(server_paths);
    }

    // if program is used as client
    else if (strcmp(argv[1], "-c") == 0) {
      const unsigned int BUFF_SIZE = 512; // max size of bytes of receive buffer
      char buff[BUFF_SIZE]; // for receive buffer

      ssize_t bytes_received = 0;

      long int request_fsize;

      // read request file
      char* message = ReadFile("request.txt", &request_fsize);

      if (message == NULL) {
        close(sock);
        perror("ReadFile()");
        exit(EXIT_FAILURE);
      }

      // connect to the server using its port number and IP
      if (connect(sock, (struct sockaddr*) &addr, sizeof (addr)) < 0) {
        close(sock);
        perror("connect()");
        exit(EXIT_FAILURE);
      }

      // send client's request to server
      if (send(sock, message, (size_t) (request_fsize + 1), 0) < 0) {
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

char* ReadFile(char* file_name, long int* file_size) {
  FILE* file;

  // open file
  file = fopen(file_name, "r");

  // get file size
  fseek(file, 0, SEEK_END);
  *file_size = ftell(file);
  rewind(file);

  // read file content
  char* file_content = (char*) malloc((size_t) (*file_size + 1));

  // malloc(...) can return NULL instead of allocating memory
  if (file_content == NULL) {
    return NULL;
  }

  // if file was not read successfully
  if (fread(file_content, 1, *file_size, file) != *file_size) {
    return NULL;
  }

  file_content[*file_size] = '\0';

  // close file
  fclose(file);

  return file_content;
}

char** SearchAfter(char* substring, char* string, char* separator, unsigned long int* number_of_results) {
  char** results = NULL;

  size_t string_length = strlen(string);
  size_t substring_length = strlen(substring);
  size_t separator_length = strlen(separator);

  *number_of_results = 0;

  // count number of substrings placed in the string
  for (size_t i = 0; i < string_length; i++) {
    if (string[i] == substring[0]) {
      size_t j = 0;

      // check if the substring is in the string
      while (string[i + j] == substring[j]) {
        j += 1;

        // as at some moment substring[j] will be '\0' (considering that substring_length < string_length)
        // and it will signify that the content of substring was found in the string and the loop would be finished,
        // unless string[i + j] would be the end of the string as well and so it would also be '\0',
        // so the next iteration of the loop would happen and the program would try to access the values
        // outside the given arrays (resulting in error or unpredictable behaviour)
        if (string[i + j] == '\0') {
          break;
        }
      }

      // that means that every characters of substring was in the string (in same sequence)
      if (j == substring_length) {
        // (j - 1) moves index to the position of the final character of the string, so when the index gets incremented (i++)
        // in the end of for loop iteration -- the index gets positioned right after the the end of substring
        i += j - 1;
        *number_of_results += 1;
      }
    }
  }

  if (number_of_results == 0) {
    return NULL;
  }

  results = (char**) malloc((sizeof (char*)) * (*number_of_results));

  unsigned int count = 0;

  // get all substrings after given substring but before the given separator
  for (size_t i = 0; i < string_length; i++) {
    // find again each place where substring is in the string
    if (string[i] == substring[0]) {
      size_t j = 0;

      while (string[i + j] == substring[j]) {
        j += 1;

        // explanations are above in this function
        if (string[i + j] == '\0') {
          break;
        }
      }

      // and if every characters of substring was in the string (in same sequence)
      if (j == substring_length) {
        // move to the part of the string that starts right after the given substring
        i += j;

        size_t length = 0;

        j = 0;

        // get the length of searched substring
        while (string[i + length] != '\0') {
          if (string[i + length] == separator[0]) {
            size_t k = 0;

            while (string[i + length + k] == separator[k]) {
              k += 1;

              // similar to cases before (explanations in function above)
              if (string[i + length + k] == '\0') {
                break;
              }
            }

            // that means that the algorithm reached the separator for the string search
            // (if this block will be removed, then the length would be incremented until
            // the algorithm reaches the end of the string)
            if (k == separator_length) {
              break;
            }
          }

          length += 1;
        }

        char* result_found = (char*) malloc(sizeof(char) * (length + 1));
        result_found[length] = '\0';

        j = 0;

        while (j != length) {
          result_found[j] = string[i];

          j += 1;
          i += 1;
        }

        results[count] = result_found;

        // this expression lets index skip the found result of search as well as skip the separator's substring
        // to continue the search for the next occurrence of the given substring in the string (decrement for the
        // same reason as in the loop for counting number of substrings in the string)
        i += (length - 1) + (separator_length - 1);

        count += 1;
      }
    }
  }

  return results;
}


struct server_file* OpenServerFiles(char* server_paths, unsigned long int* number_of_server_paths) {
  char** available_resources;
  struct server_file* server_files;
  char* file_path;
  size_t file_path_length;

  char* index_file_name = "index.html";

  // get the list of server's URL paths, that are available for GET requests
  available_resources = SearchAfter("GET ", server_paths, "\n", number_of_server_paths);

  if (available_resources == NULL) {
    return NULL;
  }

  // there would be the same number of objects of server_file structure as the number of
  // server's URL paths available for GET requests, even though two URL paths may point
  // to the same file (if the URL path ends with '/', that means that this URL points
  // to index.html file), so the actual number of server's files available for GET
  // requests may be LESS or equal to the number of available server's URL paths
  server_files = (struct server_file*) malloc(sizeof (struct server_file) * (*number_of_server_paths));

  // if memory was not allocated for structure objetcs
  if (server_files == NULL) {
    for (unsigned long int i = 0; i < *number_of_server_paths; i++) {
      // then free the memory allocated for list of server's URL paths
      free(available_resources[i]);
    }

    free(available_resources);

    return NULL;
  }

  for (unsigned long int i = 0; i < *number_of_server_paths; i++) {
    // assign this to NULL initially to be aware that this member of structure
    // was not assigned any value yet
    server_files[i].file_content = NULL;
    server_files[i].url_path = available_resources[i];
  }

  // from this moment the list of available server's URL paths is no longer needed
  free(available_resources);

  for (unsigned long int i = 0; i < *number_of_server_paths; i++) {
    if (server_files[i].file_content != NULL) {
      // skip the URL paths that already have been linked with their file
      continue;
    }

    size_t url_path_length = strlen(server_files[i].url_path);

    // check if current URL path ends with forward slash
    if (server_files[i].url_path[url_path_length - 1] == '/') {
      size_t index_url_path_length;
      char* index_url_path;

      // URL path for index file will contatin current URL path plus index.html
      index_url_path_length = url_path_length + strlen(index_file_name);
      index_url_path = (char*) malloc((sizeof (char)) * (index_url_path_length + 1));

      index_url_path[index_url_path_length] = '\0';

      // file path will point to the index file in the current directory
      // (e.g. for the "index.html" file in current directory it is "./index.html")
      file_path_length = index_url_path_length + 1;
      file_path = (char*) malloc((sizeof (char)) * (file_path_length + 1));

      file_path[0] = '.';
      file_path[file_path_length] = '\0';

      size_t j = 0;

      // adding current URL path to the URL and file path of index file (index.html)
      for (; j < url_path_length; j++) {
        index_url_path[j] = server_files[i].url_path[j];

        // looping over file path sould be (j + 1), because the first character of
        // file path is always a dot ('.'), but URL paths always start with forward slash ('/')
        file_path[j + 1] = server_files[i].url_path[j];
      }

      // adding "index.html" to the URL and file path of index file (index.html)
      for (size_t k = 0; k < strlen(index_file_name); k++) {
        index_url_path[j + k] = index_file_name[k];
        file_path[j + 1 + k] = index_file_name[k];
      }

      // reading file content by given file path
      server_files[i].file_content = ReadFile(file_path, &server_files[i].size_of_file);

      if (server_files[i].file_content == NULL) {
        free(file_path);
        return NULL;
      }

      // now go through the rest of available URL paths to look for the URL path
      // of the index file in the same server directory
      j = i;

      for (; j < *number_of_server_paths; j++) {
        if (strstr(server_files[j].url_path, index_url_path) != NULL) {
          // if current URL path is the same as the URL path for index file
          // in the same server's directory, then point this URL path to
          // the same index file
          server_files[j].file_content = server_files[i].file_content;
          // and then exit the loop, because there could be only one
          // index file (index.html) for server's directory
          break;
        }
      }
    }

    // check if current URL path contains index.html
    else if (strstr(server_files[i].url_path, index_file_name) != NULL) {
      size_t directory_url_path_length;
      char* directory_url_path;

      // URL path for the same directory where index file is located
      directory_url_path_length = url_path_length - strlen(index_file_name);
      directory_url_path = (char*) malloc((sizeof (char)) * (directory_url_path_length + 1));

      directory_url_path[directory_url_path_length] = '\0';

      // file path will point to the index file in the current directory
      // (e.g. for the "index.html" file in current directory it is "./index.html")
      file_path_length = url_path_length + 1;
      file_path = (char*) malloc((sizeof (char)) * (file_path_length + 1));

      file_path[0] = '.';
      file_path[file_path_length] = '\0';

      size_t j = 0;

      // adding current URL path to the URL and file path of index file (index.html)
      for (; j < directory_url_path_length; j++) {
        directory_url_path[j] = server_files[i].url_path[j];

        // looping over file path sould be (j + 1), because the first character of
        // file path is always a dot ('.'), but URL paths always start with forward slash ('/')
        file_path[j + 1] = server_files[i].url_path[j];
      }

      // adding "index.html" to the file path of index file (index.html)
      // without adding it to the URL path of current directory
      for (; j < url_path_length; j++) {
        file_path[j + 1] = server_files[i].url_path[j];
      }

      // reading file content by given file path
      server_files[i].file_content = ReadFile(file_path, &server_files[i].size_of_file);

      if (server_files[i].file_content == NULL) {
        free(file_path);
        return NULL;
      }

      // now go through the rest of available URL paths to look for the URL path
      // of the same server directory as the URL path for current index file
      j = i;

      for (; j < *number_of_server_paths; j++) {
        if (strstr(server_files[j].url_path, directory_url_path) != NULL) {
          // point this URL path to the same index file
          server_files[j].file_content = server_files[i].file_content;
          // and then exit the loop, because there could be only one
          // index file (index.html) for server's directory
          break;
        }
      }
    }

    else {
      // file path will point to the index file in the current directory
      // (e.g. for the "index.html" file in current directory it is "./index.html")
      file_path_length = url_path_length + 1;
      file_path = (char*) malloc((sizeof (char)) * (file_path_length + 1));

      file_path[0] = '.';
      file_path[file_path_length] = '\0';

      // looping over file path sould be (j + 1), because the first character of
      // file path is always a dot ('.')
      for (size_t j = 0; j < url_path_length; j++) {
        file_path[j + 1] = server_files[i].url_path[j];
      }

      // reading file content by given file path
      server_files[i].file_content = ReadFile(file_path, &server_files[i].size_of_file);

      if (server_files[i].file_content == NULL) {
        free(file_path);
        return NULL;
      }
    }

    free(file_path);
  }

  return server_files;
}

char* ReceiveMessage(int connection, size_t *http_message_length) {
  const unsigned int BUFF_SIZE = 512; // max size of bytes of receive buffer
  const unsigned short int MAX_NUMBER_OF_ATTEMPTS = 100;
  char buff[BUFF_SIZE]; // for receive buffer

  const char* START_OF_HTTP_HEADER = "HTTP/1.1";
  const char* END_OF_HTTP_HEADER = "\n\n";

  char* temp = NULL;
  char* http_message = NULL;
  char* http_header = NULL;
  char* http_body = NULL;

  size_t http_header_length = 0;
  size_t http_body_length = 0;

  ssize_t bytes_received = 0;

  unsigned short int number_of_attempts = 0;

  *http_message_length = 0;

  // intial receive
  bytes_received = recv(connection, buff, BUFF_SIZE, 0);

  // start making attempts of receiving data until they become successfull or until they reach the limit
  while (number_of_attempts < MAX_NUMBER_OF_ATTEMPTS) {
    if (bytes_received > 0) {
      // check if there is start of HTTP message section in the current receive buffer
      if (strstr(buff, START_OF_HTTP_HEADER) != NULL) {
        char* end_of_header = NULL;

        // printf("1. Start receiving HTTP header\n");

        // start collecting bytes of HTTP header section until substring, that signifies the end of this section occurs
        while (strstr(buff, END_OF_HTTP_HEADER) == NULL) {
          http_header_length += (size_t) bytes_received;

          temp = (char*) malloc(sizeof(char) * (http_header_length));

          if (temp == NULL) {
            free(http_header);
            return NULL;
          }

          //// copy previously received data to new block of memory
          for (size_t i = 0; i < (http_header_length - (size_t) bytes_received); i++) {
            temp[i] = http_header[i];
          }

          //// add all characters from the receive buff to the new block of memory of received data
          for (ssize_t i = 0; i < bytes_received; i++) {
            temp[http_header_length - (size_t) bytes_received + (size_t) i] = buff[i];
          }

          //// free old block of memory
          free(http_header);
          http_header = temp;
          bytes_received = recv(connection, buff, BUFF_SIZE, 0);
        }

        // printf("2. Receive the remaining part of HTTP header\n");

        // get the position of the substring, that signifies the end of HTTP header section
        end_of_header = strstr(buff, END_OF_HTTP_HEADER);

        size_t last_header_length = 0; // length of last part of HTTP header section (part from buffer that comes before "\n\n")

        // get the length of data that comes before the substring (that signifies the end of HTTP header section)
        while (&(buff[last_header_length]) != end_of_header) {
          last_header_length += 1;
        }

        // collect the last data from HTTP header section
        if (last_header_length > 0) {
          http_header_length += last_header_length;
          temp = (char*) malloc(sizeof(char) * (http_header_length));

          if (temp == NULL) {
            free(http_header);
            return NULL;
          }

          //// copy previously received data to new block of memory
          for (size_t i = 0; i < (http_header_length - last_header_length); i++) {
            temp[i] = http_header[i];
          }

          //// add all characters from the receive buff to the new block of memory of received data
          for (size_t i = 0; i < last_header_length; i++) {
            temp[http_header_length - last_header_length + i] = buff[i];
          }

          //// free old block of memory
          free(http_header);
          http_header = temp;
        }

        // printf("3. Add null terminator in the end of string, containing HTTP header section\n");

        // add null terminator character in the end of collected HTTP header section
        temp = (char*) malloc(sizeof(char) * (http_header_length + 1));

        if (temp == NULL) {
          free(http_header);
          return NULL;
        }

        //// copy previously received data to new block of memory
        for (size_t i = 0; i < http_header_length; i++) {
          temp[i] = http_header[i];
        }

        temp[http_header_length] = '\0';

        //// free old block of memory
        free(http_header);
        http_header = temp;

        // printf("header_length: %lu\n", http_header_length);

        // printf("4. Get the number of bytes for HTTP body section, if Content-Length header is inside HTTP header section\n");

        // check if there is Content_Length header in HTTP header section
        if (strstr(http_header, "Content-Length") != NULL) {
          char** content_length_bytes;
          unsigned long int number_of_results = 0;
          unsigned long int number_of_bytes;
          unsigned long int length_of_number = 0;

          // get the number of bytes of data in HTTP body section, specified by the Content_Length header
          content_length_bytes = SearchAfter("Content-Length: ", http_header, "\n", &number_of_results);

          if (content_length_bytes == NULL) {
            free(http_header);
            return NULL;
          }

          if (number_of_results != 1) {
            for (unsigned long int i = 0; i < number_of_results; i++) {
              free(content_length_bytes[i]);
            }

            free(content_length_bytes);
            free(http_header);
            return NULL;
          }

          // some of the type conversion..
          number_of_bytes = strtoul(content_length_bytes[0], NULL, 10);

          for (unsigned long int i = 0; i < number_of_results; i++) {
            free(content_length_bytes[i]);
          }

          free(content_length_bytes);

          // printf("5. Receive first part of HTTP body section\n");

          // check if there is data for HTTP body section in current receive buffer
          if (last_header_length != (bytes_received - strlen(END_OF_HTTP_HEADER))) {
            http_body_length += bytes_received - strlen(END_OF_HTTP_HEADER) - last_header_length;
            http_body = (char*) malloc(sizeof(char) * (http_body_length));

            if (http_body == NULL) {
              free(http_header);
              return NULL;
            }

            // get the first data of HTTP body section
            for (size_t i = 0; i < http_body_length; i++) {
              http_body[i] = buff[last_header_length + strlen(END_OF_HTTP_HEADER) + i];
            }

            number_of_bytes -= http_body_length;
          }

          // printf("6. Start receiving HTTP body section, until all bytes are received\n");

          // start collecting data for HTTP body section until all bytes of data (specified by Content-Length header) are collected
          bytes_received = recv(connection, buff, BUFF_SIZE, 0);

          while (number_of_bytes > 0) {
            http_body_length += (unsigned long int) bytes_received;
            temp = (char*) malloc(sizeof(char) * (http_body_length));

            if (temp == NULL) {
              free(http_header);
              free(http_body);
              return NULL;
            }

            //// copy previously received data to new block of memory
            for (size_t i = 0; i < (http_body_length - (size_t) bytes_received); i++) {
              temp[i] = http_body[i];
            }

            //// add all characters from the receive buff to the new block of memory of received data
            for (ssize_t i = 0; i < bytes_received; i++) {
              temp[http_body_length - (size_t) bytes_received + (size_t) i] = buff[i];
            }

            //// free old block of memory
            free(http_body);
            http_body = temp;
            number_of_bytes -= bytes_received;
          }
        }

        // printf("7. Add null terminator in the end of string, containing HTTP body section\n");

        // add null terminator character in the end of collected HTTP body section
        temp = (char*) malloc(sizeof(char) * (http_body_length + 1));

        if (temp == NULL) {
          free(http_header);
          free(http_body);
          return NULL;
        }

        //// copy previously received data to new block of memory
        for (size_t i = 0; i < http_body_length; i++) {
          temp[i] = http_body[i];
        }

        temp[http_body_length] = '\0';

        //// free old block of memory
        free(http_body);
        http_body = temp;

        // printf("body_length: %lu\n", http_body_length);

        // printf("8. Form HTTP message from HTTP header and HTTP body sections\n");

        // combine HTTP header and HTTP body sections to form HTTP message
        *http_message_length = http_header_length + http_body_length + strlen(END_OF_HTTP_HEADER);
        http_message = (char*) malloc(sizeof(char) * (*http_message_length + 1));
        http_message[*http_message_length] = '\0';

        if (http_message == NULL) {
          free(http_header);
          free(http_body);
          return NULL;
        }

        // copy all data from collected HTTP header section
        for (size_t i = 0; i < http_header_length; i++) {
          http_message[i] = http_header[i];
        }

        for (size_t i = 0; i < strlen(END_OF_HTTP_HEADER); i++) {
         http_message[i + http_header_length] = END_OF_HTTP_HEADER[i];
        }

        // copy all data from collected HTTP body section
        for (size_t i = 0; i < http_body_length; i++) {
          http_message[i + http_header_length + strlen(END_OF_HTTP_HEADER)] = http_body[i];
        }

        free(http_header);
        free(http_body);

        // printf("message_length: %lu\n", *http_message_length);

        break;
      }
    }

    else { // make another attempt if previous one was unsuccessfull
      bytes_received = recv(connection, buff, BUFF_SIZE, 0);
      number_of_attempts += 1;
    }
  }

  return http_message;
}

char* FormResponse(char* request_content, size_t request_length, struct server_file* server_files, unsigned long int number_of_files) {
  char* http_message = NULL;
  char* http_header = NULL;
  char* http_body = NULL;

  size_t http_message_length;
  size_t http_header_length;
  size_t http_body_length;

  if (strstr(request_content, "GET") != NULL) {
    char** search_result;
    unsigned long int number_of_results;

    char* url_path;

    search_result = SearchAfter("GET ", request_content, " HTTP/1.1\n", &number_of_results);

    if (search_result == NULL) {
      return NULL;
    }

    if (number_of_results != 1) {
      for (unsigned long int i = 0; i < number_of_results; i++) {
        free(search_result[i]);
      }

      free(search_result);
      return NULL;
    }

    url_path = search_result[0];
    free(search_result);

    unsigned long int i = 0;

    for (; i < number_of_files; i++) {
      if (strstr(server_files[i].url_path, url_path) != NULL) {
        // return requested file with code 200
        http_header = "HTTP/1.1 200 OK";
        http_body = server_files[i].file_content;

        free(url_path);
        url_path = NULL;

        break;
      }
    }

    // if requested file was not in the server files
    if (url_path != NULL) {
      // return 404 error (file not found)
      http_header = "HTTP/1.1 404 Not Found";
      http_body = "<!DOCTYPE html>\n<html>\n<head>\n</head>\n<body>\n<h1>\n404 Not Found\n</h1>\n</body>\n</html>";

      free(url_path);
      url_path = NULL;
    }

    http_header_length = strlen(http_header);
    http_body_length = strlen(http_body);

    // printf("http_header_length: %lu\n", http_header_length);
    // printf("http_body_length: %lu\n", http_body_length);

    http_message_length = http_header_length + strlen("\n\n") + http_body_length;

    // printf("http_message_length: %lu\n", http_message_length);

    http_message = (char*) malloc(sizeof(char) * (http_message_length + 1));
    http_message[http_message_length] = '\0';

    size_t j = 0;
    for (; j < http_header_length; j++) {
      http_message[j] = http_header[j];
    }

    for (; (j - http_header_length) < 2; j++) { // (j - http_header_length) < 2, because strlen("\n\n") results in 2
      http_message[j] = '\n';
    }

    for (; (j - http_header_length - strlen("\n\n")) < http_message_length; j++) {
      http_message[j] = server_files[i].file_content[j - http_header_length - strlen("\n\n")];
    }
  }

  return http_message;
}
