#include "tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *ReadFile(char *file_name, long int *file_size) {
  FILE *file;

  // open file
  file = fopen(file_name, "r");

  // get file size
  fseek(file, 0, SEEK_END);
  *file_size = ftell(file);
  rewind(file);

  // read file content
  char *file_content = (char *)malloc((size_t)(*file_size + 1));

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

char **SearchAfter(char *substring, char *string, char *separator,
                   unsigned long int *number_of_results) {
  char **results = NULL;

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

        // as at some moment substring[j] will be '\0' (considering that
        // substring_length < string_length) and it will signify that the
        // content of substring was found in the string and the loop would be
        // finished, unless string[i + j] would be the end of the string as well
        // and so it would also be '\0', so the next iteration of the loop would
        // happen and the program would try to access the values outside the
        // given arrays (resulting in error or unpredictable behaviour)
        if (string[i + j] == '\0') {
          break;
        }
      }

      // that means that every characters of substring was in the string (in
      // same sequence)
      if (j == substring_length) {
        // (j - 1) moves index to the position of the final character of the
        // string, so when the index gets incremented (i++) in the end of for
        // loop iteration -- the index gets positioned right after the the end
        // of substring
        i += j - 1;
        *number_of_results += 1;
      }
    }
  }

  if (number_of_results == 0) {
    return NULL;
  }

  results = (char **)malloc((sizeof(char *)) * (*number_of_results));

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

      // and if every characters of substring was in the string (in same
      // sequence)
      if (j == substring_length) {
        // move to the part of the string that starts right after the given
        // substring
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

            // that means that the algorithm reached the separator for the
            // string search (if this block will be removed, then the length
            // would be incremented until the algorithm reaches the end of the
            // string)
            if (k == separator_length) {
              break;
            }
          }

          length += 1;
        }

        char *result_found = (char *)malloc(sizeof(char) * (length + 1));
        result_found[length] = '\0';

        j = 0;

        while (j != length) {
          result_found[j] = string[i];

          j += 1;
          i += 1;
        }

        results[count] = result_found;

        // this expression lets index skip the found result of search as well as
        // skip the separator's substring to continue the search for the next
        // occurrence of the given substring in the string (decrement for the
        // same reason as in the loop for counting number of substrings in the
        // string)
        i += (length - 1) + (separator_length - 1);

        count += 1;
      }
    }
  }

  return results;
}

void HandleRuntimeErrors(int error, int socket) {
  if (error == -1) {
    close(socket);
    perror("ReadFile()");
    exit(EXIT_FAILURE);
  } else if (error == -2) {
    close(socket);
    perror("OpenServerFiles()");
    exit(EXIT_FAILURE);
  } else if (error == -3) {
    close(socket);
    perror("bind()");
    exit(EXIT_FAILURE);
  } else if (error == -4) {
    close(socket);
    perror("listen()");
    exit(EXIT_FAILURE);
  } else if (error == -5) {
    close(socket);
    perror("epoll_create1()");
    exit(EXIT_FAILURE);
  } else if (error == -6) {
    close(socket);
    perror("epoll_ctl()");
    exit(EXIT_FAILURE);
  } else if (error == -7) {
    close(socket);
    perror("epoll_wait()");
    exit(EXIT_FAILURE);
  } else if (error == -8) {

  }
}