#pragma once

#include <netinet/in.h>

int RunClient(struct sockaddr_in* socket_address, int socket);