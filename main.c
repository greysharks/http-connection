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

char* ReadFile(char* file_name, long int* file_size);
char** SearchAfter(char* substring, char* string, char* separator, unsigned int* number_of_results);
struct server_file* OpenServerFiles(char* server_paths, unsigned int* number_of_server_paths);

int main(int argc, char* argv[]) {
  const unsigned int IP = (127 << 24) + 1; // for ip 127.0.0.1
  const unsigned short int PORT = 80;

  const unsigned int BUFF_SIZE = 512; // max size of bytes of receive buffer
  char buff[BUFF_SIZE]; // for receive buffer
  ssize_t bytes_received = 0;

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
      long int response_fsize; // fsize is file size
      long int server_paths_fsize;

      // used only for accept(...) function
      unsigned int addr_len = sizeof (addr);

      char* message;
      char* server_paths;
      char* url_path;
      char* requested_method;

      struct server_file* server_files;

      unsigned int number_of_resources;
      int url_path_length;
      int requested_method_length;

      // mark unexisting connections as -1
      for (unsigned short int i = 0; i < MAX_CONNECTIONS; i++) {
        conns[i] = -1;
      }

      // read response file
      message = ReadFile("response.txt", &response_fsize);

      if (message == NULL) {
        close(sock);
        perror("ReadFile()");
        exit(EXIT_FAILURE);
      }

      // read a file containing all url paths available in this server (in form of list)
      server_paths = ReadFile("paths.txt", &server_paths_fsize);

      if (server_paths == NULL) {
        close(sock);
        perror("ReadFile()");
        exit(EXIT_FAILURE);
      }

      number_of_resources = 0;

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

          // read client's request and print it out in console
          bytes_received = recv(conns[i], buff, BUFF_SIZE, 0);

          // these variables will be used to check whether the received bytes from client
          // contain starting parts or ending parts of http request
          char* start_of_request = NULL;
          char* end_of_request = NULL;

          while (bytes_received > 0) {
            start_of_request = strstr(buff, "HTTP/1.1");
            end_of_request = strstr(buff, "\n\n");

            if (start_of_request != NULL) {
              int end_of_first_line = 0;

              // locate the index of the first end line character (aka the end
              // of first line of http message) in the string that holds the
              // first part of http request (in other words, find where the
              // first line ends in that string)
              while (buff[end_of_first_line] != '\n') {
                end_of_first_line += 1;
              }

              int space_count = 1;

              int spaces[2];

              for (int j = 0; j < end_of_first_line; j++) {
                if (buff[j] == ' ') {
                  // [space_count - 1] because space_count equals 1 initially
                  // and not 0 (but 0 is the index of first element in arrays
                  // so it should be used to access the first element of spaces
                  // array)
                  spaces[space_count - 1] = j;
                  space_count += 1;
                }
              }

              // the number of spaces in the frist line of the http request
              // should be exactly 2: the first space after name of the http
              // method (GET, POST, PUT, etc) and the second space after the
              // url path to the requested resource. after that goes the version
              // of http protocol and the end of line character
              if (space_count != 2) {
                close(conns[i]);
                conns[i] = -1;
                N -= 1;
                break;
              }

              // (spaces[1] - spaces[0] - 1) is because the string is located
              // between those indexes and so it excludes them (the first
              // character of the string is located at (spaces[0] + 1) and the
              // last character of the string is located at (spaces[1] - 1))
              url_path_length = spaces[1] - spaces[0] - 1;
              url_path = (char*) malloc(sizeof(char) * ((size_t) (url_path_length + 1)));
              url_path[url_path_length] = '\0';

              // (spaces[0] + 1) to exclude the first space character from
              // the url path string (in other words, start indexing the string
              // right after the index of first space of that string)
              for (int j = spaces[0] + 1; j < url_path_length; j++) {
                url_path[j] = buff[j];
              }

              // check if the requested path to resource on server exists in the list of avilable server's paths
              if (strstr(server_paths, url_path) == NULL) {
                close(conns[i]);
                conns[i] = -1;
                N -= 1;
                break;
              }

              requested_method_length = spaces[0];
              requested_method = (char*) malloc(sizeof(char) * ((size_t) (requested_method_length + 1)));
              requested_method[requested_method_length] = '\0';

              for (int j = 0; j < requested_method_length; j++) {
                requested_method[j] = buff[j];
              }

              if (strstr(server_paths, requested_method) == NULL) {
                close(conns[i]);
                conns[i] = -1;
                N -= 1;
                break;
              }

              // send server's response to client
              if (send(conns[i], message, (size_t) (response_fsize + 1), 0) < 0) {
                close(conns[i]);
                conns[i] = -1;
                N -= 1;
                break;
              }

              // close the write option for connetion socket
              if (shutdown(conns[i], SHUT_WR) < 0) {
                close(conns[i]);
                conns[i] = -1;
                N -= 1;
                break;
              }

              free(url_path);
              free(requested_method);
            }

            // display request message in console
            for (ssize_t j = 0; j < bytes_received; j++) {
              putchar(buff[i]);
            }

            if (end_of_request != NULL) {
              // close the read option for connection socket
              if (shutdown(conns[i], SHUT_RD) < 0) {
                close(conns[i]);
                conns[i] = -1;
                N -= 1;
                break;
              }

              close(conns[i]);
              conns[i] = -1;
              N -= 1;
              break;
            }

            bytes_received = recv(conns[i], buff, BUFF_SIZE, 0);
          }

          // check if any error occured while trying to receive request from client
          if (bytes_received < 0) {
            close(conns[i]);
            conns[i] = -1;
            N -= 1;
            continue;
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

      for (unsigned int i = 0; i < number_of_resources; i++) {
        free(server_files[i].file_content);
        free(server_files[i].url_path);
      }

      free(server_files);

      free(message);
      free(server_paths);
    }

    // if program is used as client
    else if (strcmp(argv[1], "-c") == 0) {
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

char** SearchAfter(char* substring, char* string, char* separator, unsigned int* number_of_results) {
  char** results = NULL;

  unsigned long int string_length = strlen(string);
  unsigned long int substring_length = strlen(substring);
  unsigned long int separator_length = strlen(separator);

  int count = 0;

  // count number of substrings placed in the string
  for (unsigned long int i = 0; i < string_length; i++) {
    if (string[i] == substring[0]) {
      unsigned int j = 0;

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
        count += 1;
      }
    }
  }

  *number_of_results = count;

  if (number_of_results == 0) {
    return NULL;
  }

  results = (char**) malloc((sizeof (char*)) * ((size_t) count));

  count = 0;

  // get all substrings after given substring but before the given separator
  for (unsigned long int i = 0; i < string_length; i++) {
    // find again each place where substring is in the string
    if (string[i] == substring[0]) {
      unsigned int j = 0;

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

        unsigned long int length = 0;

        j = 0;

        // get the length of searched substring
        while (string[i + length] != '\0') {
          if (string[i + length] == separator[0]) {
            unsigned int k = 0;

            while (string[i + length + k] == separator[k]) {
              k += 1;

              // similar to cases before (explanations in function above)
              if (string[i + length + k] == '\0') {
                break;
              }
            }

            // that means that the algorithm reached the separator for the string search
            if (k == separator_length) {
              break;
            }
          }

          length += 1;
        }

        char* result_found = (char*) malloc(sizeof(char) * ((size_t)(length + 1)));
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


struct server_file* OpenServerFiles(char* server_paths, unsigned int* number_of_server_paths) {
  char** available_resources;
  struct server_file* server_files;
  char* file_path;
  unsigned int file_path_length;

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
    for (unsigned int i = 0; i < *number_of_server_paths; i++) {
      // then free the memory allocated for list of server's URL paths
      free(available_resources[i]);
    }

    free(available_resources);

    return NULL;
  }

  for (unsigned int i = 0; i < *number_of_server_paths; i++) {
    // assign this to NULL initially to be aware that this member of structure
    // was not assigned any value yet
    server_files[i].file_content = NULL;
    server_files[i].url_path = available_resources[i];
  }

  // from this moment the list of available server's URL paths is no longer needed
  free(available_resources);

  for (unsigned int i = 0; i < *number_of_server_paths; i++) {
    if (server_files[i].file_content != NULL) {
      // skip the URL paths that already have been linked with their file
      continue;
    }

    unsigned int url_path_length = strlen(server_files[i].url_path);

    // check if current URL path ends with forward slash
    if (server_files[i].url_path[url_path_length - 1] == '/') {
      unsigned int index_url_path_length;
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

      unsigned int j = 0;

      // adding current URL path to the URL and file path of index file (index.html)
      for (; j < url_path_length; j++) {
        index_url_path[j] = server_files[i].url_path[j];

        // looping over file path sould be (j + 1), because the first character of
        // file path is always a dot ('.'), but URL paths always start with forward slash ('/')
        file_path[j + 1] = server_files[i].url_path[j];
      }

      // adding "index.html" to the URL and file path of index file (index.html)
      for (unsigned int k = 0; k < strlen(index_file_name); k++) {
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
      unsigned int directory_url_path_length;
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

      unsigned int j = 0;

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
      for (unsigned int j = 0; j < url_path_length; j++) {
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
