#pragma once

// this function returns a string, containing the content of requested file
char* ReadFile(char* file_name, long int* file_size); 
// this function excerpts information from a string, pointed out by the given substring and the given separator (start and the end of each portion of information)
char** SearchAfter(char* substring, char* string, char* separator, unsigned long int* number_of_results); 
