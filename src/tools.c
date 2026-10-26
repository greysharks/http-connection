#include "tools.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef true 
#define true 1
#endif

#ifndef false
#define false 0
#endif

char *ReadFile(char *file_name, long int *file_size) {
  FILE *file;

  // open file
  file = fopen(file_name, "r");

  if (file == NULL) {
    return NULL;
  }

  // get file size
  if (fseek(file, 0, SEEK_END) != 0) {
    return NULL;
  }

  *file_size = ftell(file);

  if (*file_size == -1L) {
    return NULL;
  }

  rewind(file);

  // read file content
  char *file_content = (char *)malloc((size_t)(*file_size + 1));

  // malloc(...) can return NULL instead of allocating memory
  if (file_content == NULL) {
    fclose(file);
    return NULL;
  }

  // if file was not read successfully
  if (fread(file_content, 1, *file_size, file) != *file_size) {
    free(file_content);
    fclose(file);
    return NULL;
  }

  file_content[*file_size] = '\0';

  // close file
  fclose(file);

  return file_content;
}

size_t CountNumberOfSubstrings(char *substring, char *string) {
  size_t string_length = strlen(string);
  size_t substring_length = strlen(substring);

  size_t counter = 0;

  if (string_length < substring_length) return counter; // return 0

  for (size_t string_index = 0; string_index < string_length; string_index++) {

    // possibile occurence
    if (string[string_index] == substring[0]) {
      size_t substring_index = 0;

      // loop over string characters, until they matches with substring characters
      while (string[string_index + substring_index] == substring[substring_index]) {
        substring_index += 1;

        // considering that substring_length < string_length, at some point
        // the substring will end (if all characters match) and it will signify that the
        // content of substring was found in the string, so the loop should be
        // finished

        if (substring_index == substring_length) {
          counter += 1;
          break;
        }
      }

      // skip unsuccessful search (or successful!)
      string_index += substring_index - 1; // (substring_index - 1), because there's string_index++ in the end of the loop cycle anyway
    }
  }

  return counter;
}

char** ExtractResults(char *string, char *begin, char *end, size_t number_of_results) {
  char **results = (char **)malloc((sizeof(char *)) * (number_of_results));

  if (results == NULL) {
    return NULL;
  }

  size_t string_length = strlen(string);
  size_t begin_length = strlen(begin);
  size_t end_length = strlen(end);

  if (string_length < begin_length || string_length < end_length) {
    free(results);
    return NULL;
  }

  if (end_length == 0) {
    // if there was an empty string instead of custom 'end' separator, 
    // then we'll use 'begin' separator as 'end' separator
    end = begin;
    // recalculate length
    end_length = strlen(end);
  }

  // index for each result in results array
  size_t results_index = 0;

  // get all substrings after given substring but before the given separator
  for (size_t string_index = 0; string_index < string_length; string_index++) {
    // potential occurence of 'begin' separator in string
    if (string[string_index] == begin[0]) {
      size_t substring_index = 0;

      // search stops if characters stop to match
      while (string[string_index + substring_index] == begin[substring_index]) {
        substring_index += 1;

        // found full occurence of 'begin' separator in string
        if (substring_index == begin_length) {
          // move to the part of the string that starts right after the occurence
          string_index += substring_index;

          size_t length = 0;
          int finish_search_cycle = false;

          // get the length of searched substring
          while (string[string_index + length] != '\0') {
            // start search for occurence of 'end' separator in string
            if (string[string_index + length] == end[0]) {
              substring_index = 0;

              // search stops if characters stop matching
              while (string[string_index + length + substring_index] == end[substring_index]) {
                substring_index += 1;

                // this should be self explaining
                if (substring_index == end_length) {
                  finish_search_cycle = true;
                  break;
                }
              }
            }

            // exit outer while loop if full occurence of 'end' separator was found in the string
            if (finish_search_cycle == true) {
              finish_search_cycle = false;
              break;
            }

            // increment length otherwise
            length += 1;
          }

          char *result_found = (char *)malloc(sizeof(char) * (length + 1));

          if (result_found == NULL) {
            free(results);
            return NULL;
          }

          result_found[length] = '\0';
          substring_index = 0;

          // copy substring found to result_found variable
          while (substring_index != length) {
            result_found[substring_index] = string[string_index + substring_index];
            substring_index += 1;
          }

          results[results_index] = result_found;
          string_index += length - 1;// (length - 1), because there's string_index++ in the end of the loop cycle anyway
          results_index += 1;
          break;
        }
      }
    }
  }

  // exit function while freeing memory if the number of results added by this function
  // is different from number_of_results provided as argument to this function
  if (results_index != number_of_results) {
    for (size_t i = 0; i < results_index; i++) {
      free(results[i]);
    }
    free(results);
    return NULL;
  }

  return results;
}

char **SearchAfter(char *string, char *begin, char *end, size_t *number_of_results) {
  // count number of substrings placed in the string
  *number_of_results = CountNumberOfSubstrings(begin, string);

  if (*number_of_results == 0) {
    return NULL;
  }

  return ExtractResults(string, begin, end, *number_of_results);
}

void HandleRuntimeErrors(int error, int socket) {
  if (error < 0) {
    close(socket);
    if (error == ERRORS_READ_FILE) {
      perror("ReadFile()");
    } else if (error == ERRORS_OPEN_SERVER_FILES) {
      perror("OpenServerFiles()");
    } else if (error == ERRORS_SERVER_BIND) {
      perror("bind()");
    } else if (error == ERRORS_SERVER_LISTEN) {
      perror("listen()");
    } else if (error == ERRORS_EPOLL_CREATE) {
      perror("epoll_create1()");
    } else if (error == ERRORS_EPOLL_CTL) {
      perror("epoll_ctl()");
    } else if (error == ERRORS_EPOLL_WAIT) {
      perror("epoll_wait()");
    } else if (error == ERRORS_CLIENT_CONNECT) {
      perror("connect()");
    } else if (error == ERRORS_SOCKET_SEND) {
      perror("send()");
    } else if (error == ERRORS_SOCKET_SHUTDOWN) {
      perror("shutdown()");
    } else if (error == ERRORS_SOCKET_RECV) {
      perror("recv()");
    }
    exit(EXIT_FAILURE);
  }
}
